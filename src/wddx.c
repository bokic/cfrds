#include <wddx.h>

#include <libxml/tree.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>


#if defined(__APPLE__) || defined(_WIN32)
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *ptr = (volatile unsigned char *)s;
    while (n--) {
        *ptr++ = 0;
    }
}
#endif

#define xmlDoc_defer(var) xmlDoc* var __attribute__((cleanup(xmlDoc_cleanup))) = NULL

typedef struct {
    int type;
    int cnt;
    union {
        bool boolean;
        double number;
        void *items[];
        char string[];
    };
} WDDX_NODE_int;

typedef struct {
    char *name;
    WDDX_NODE_int *value;
} WDDX_STRUCT_NODE_int;

typedef struct  {
    WDDX_NODE_int *header;
    WDDX_NODE_int *data;
    xmlBufferPtr str;
} WDDX_int;

void WDDX_NODE_int_free(WDDX_NODE_int *value);

static void xmlDoc_cleanup(xmlDoc **value)
{
    if (value)
    {
        xmlFreeDoc(*value);
        *value = NULL;
    }
}

static bool is_string_numeric(const char *str)
{
    if (str == NULL || *str == '\0')
        return false;

    size_t len = strlen(str);

    if (len > 10) return false;

    for (size_t i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
            return false;
    }

    return true;
}

static WDDX_NODE_int *wddx_recursively_put(WDDX_NODE_int *node, const char *path, const char *value, enum wddx_type type)
{
    size_t path_len = strlen(path);
    size_t newsize = 0;

    if (path_len == 0)
    {
        WDDX_NODE_int *new_node = NULL;
        size_t value_len = strlen(value);

        size_t size = offsetof(WDDX_NODE_int, string) + value_len + 1;
        new_node = malloc(size);
        if (new_node == NULL) return NULL;

        new_node->type = type;
        memcpy(new_node->string, value, value_len + 1);

        return new_node;
    }

    const char *next = strchr(path, ',');
    if (next == NULL)
        newsize = path_len;
    else
        newsize = next - path;
    if (newsize == 0) return NULL;

    char newkey[newsize + 1];
    memcpy(newkey, path, newsize);
    newkey[newsize] = 0;
    if (next)
        path += newsize + 1;
    else
        path += newsize;

    if(is_string_numeric(newkey))
    {
        int idx = atoi(newkey) + 1;
        if (idx < 1) return NULL;

        if (node == NULL)
        {
            newsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_NODE_int) * idx);
            node = (WDDX_NODE_int *)malloc(newsize);
            if (node == NULL)
            {
                return NULL;
            }
            explicit_bzero(node, newsize);
            node->type = WDDX_ARRAY;
            node->cnt = idx;
        }

        switch (node->type)
        {
        case WDDX_BOOLEAN:
            return NULL;
        case WDDX_NUMBER:
            return NULL;
        case WDDX_STRING:
            return NULL;
        case WDDX_ARRAY:
        {
            if(node->cnt < idx)
            {
                newsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_NODE_int) * idx);
                node = (WDDX_NODE_int *)realloc(node, newsize);
                if (node == NULL) return NULL;
                int oldsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_NODE_int) * node->cnt);
                explicit_bzero((char *)node + oldsize, newsize - oldsize);
                node->cnt = idx;
            }

            node->items[idx - 1] = wddx_recursively_put(node->items[idx - 1], path, value, type);
            return node;
        }
        case WDDX_STRUCT:
            return NULL;
        default:
            return NULL;
        }
    }
    else
    {
        if (node == NULL)
        {
            newsize = sizeof(WDDX_NODE_int);
            node = (WDDX_NODE_int *)malloc(newsize);
            if (node == NULL)
                return NULL;
            explicit_bzero(node, newsize);
            node->type = WDDX_STRUCT;
            node->cnt = 0;
        }

        switch (node->type)
        {
        case WDDX_BOOLEAN:
             return NULL;
        case WDDX_NUMBER:
             return NULL;
        case WDDX_STRING:
            return NULL;
        case WDDX_ARRAY:
            return NULL;
        case WDDX_STRUCT:
        {
            for(int c = 0; c < node->cnt; c++)
            {
                WDDX_STRUCT_NODE_int *child = (WDDX_STRUCT_NODE_int *)node->items[c];
                if (strcmp(child->name, newkey) == 0)
                {
                    if (child->value != NULL) WDDX_NODE_int_free(child->value);
                    child->value = wddx_recursively_put(NULL, path, value, type);
                    return node;
                }
            }

            newsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_STRUCT_NODE_int *) * (node->cnt + 1));
            WDDX_NODE_int *pre_realloc_node = node;
            node = (WDDX_NODE_int *)realloc(node, newsize);
            if (node == NULL) {
                WDDX_NODE_int_free(pre_realloc_node);
                return NULL;
            }
            node->items[node->cnt] = malloc(sizeof(WDDX_STRUCT_NODE_int));
            if (node->items[node->cnt] == NULL) {
                WDDX_NODE_int_free(node);
                return NULL;
            }
            ((WDDX_STRUCT_NODE_int *)node->items[node->cnt])->name = strdup(newkey);
            ((WDDX_STRUCT_NODE_int *)node->items[node->cnt])->value = wddx_recursively_put(NULL, path, value, type);
            node->cnt++;
            return node;
        }
        default:
            return NULL;
        }
    }

    return NULL;
}

WDDX *wddx_create()
{
    WDDX_int *ret = malloc(sizeof(WDDX_int));
    if (ret)
    {
        explicit_bzero(ret, sizeof(WDDX_int));
    }

    return ret;
}

static bool wddx_put(WDDX *dest, const char *path, const char *value, enum wddx_type type)
{
    WDDX_int *value_int = (WDDX_int *)dest;

    if (value_int == NULL) return false;

    value_int->data = wddx_recursively_put(value_int->data, path, value, type);

    return value_int->data != NULL;
}

bool wddx_put_bool(WDDX *dest, const char *path, bool value)
{
    const char *valueStr = NULL;

    if (value)
        valueStr = "true";
    else
        valueStr = "false";

    return wddx_put(dest, path, valueStr, WDDX_BOOLEAN);
}

bool wddx_put_number(WDDX *dest, const char *path, double value)
{
    char valueStr[32];

    snprintf(valueStr, sizeof(valueStr), "%.16g", value);

    return wddx_put(dest, path, valueStr, WDDX_NUMBER);
}

bool wddx_put_string(WDDX *dest, const char *path, const char *value)
{
    return wddx_put(dest, path, value, WDDX_STRING);
}

static void wddx_to_xml_node(xmlNode *xml_node, const WDDX_NODE_int *node)
{
    xmlNode *child_node = NULL;
    char number_str[20];

    if ((xml_node == NULL)||(node == NULL)) return;

    switch(node->type)
    {
    case WDDX_BOOLEAN:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "boolean", NULL);
        xmlNewProp(child_node, BAD_CAST "value", BAD_CAST node->string);
        break;
    case WDDX_NUMBER:
        xmlNewChild(xml_node, NULL, BAD_CAST "number", BAD_CAST node->string);
        break;
    case WDDX_STRING:
        xmlNewChild(xml_node, NULL, BAD_CAST "string", BAD_CAST node->string);
        break;
    case WDDX_ARRAY:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "array", NULL);

        snprintf(number_str, sizeof(number_str), "%d", node->cnt);

        xmlNewProp(child_node, BAD_CAST "length", BAD_CAST number_str);

        for(int c = 0; c < node->cnt; c++)
        {
            wddx_to_xml_node(child_node, ((const WDDX_NODE_int *)node)->items[c]);
        }
        break;
    case WDDX_STRUCT:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "struct", NULL);

        xmlNewProp(child_node, BAD_CAST "type", BAD_CAST "java.util.HashMap");

        for(int c = 0; c < node->cnt; c++)
        {
            WDDX_STRUCT_NODE_int *var = (WDDX_STRUCT_NODE_int *)node->items[c];

            xmlNode *var_node = xmlNewChild(child_node, NULL, BAD_CAST "var", NULL);

            xmlNewProp(var_node, BAD_CAST "name", BAD_CAST var->name);

            wddx_to_xml_node(var_node, var->value);
        }

        break;
    default:
        break;
    }
}

const char *wddx_to_xml(WDDX *src)
{
    WDDX_int *value_int = (WDDX_int *)src;

    if (value_int->str)
    {
        xmlBufferFree(value_int->str);
        value_int->str = NULL;
    }

    xmlDocPtr doc = xmlNewDoc(NULL);
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "wddxPacket");
    xmlNewProp(root_node, BAD_CAST "version", BAD_CAST "1.0");
    xmlDocSetRootElement(doc, root_node);

    xmlNode *headerNode = xmlNewChild(root_node, NULL, BAD_CAST "header", NULL);
    wddx_to_xml_node(headerNode, value_int->header);

    xmlNode *dataNode = xmlNewChild(root_node, NULL, BAD_CAST "data", NULL);
    wddx_to_xml_node(dataNode, value_int->data);

    value_int->str = xmlBufferCreate();

    xmlNodeDump(value_int->str, doc, root_node, 0, 0);

    xmlFreeDoc(doc);

    return (const char *)xmlBufferContent(value_int->str);
}

static WDDX_NODE_int *wddx_from_xml_element(xmlNodePtr xml_node)
{
    WDDX_NODE_int *ret = NULL;
    size_t malloc_size = 0;

    if (xml_node == NULL) return NULL;
    if (xml_node->type != XML_ELEMENT_NODE) return NULL;

    const char *name = (const char *)xml_node->name;
    if (name == NULL) return NULL;

    if (strcmp(name, "boolean") == 0)
    {
        if (xml_node->children == NULL) return NULL;
        if (xml_node->children->content == NULL) return NULL;

        malloc_size = sizeof(WDDX_NODE_int);
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_BOOLEAN;

        if (strcmp((const char *)xml_node->children->content, "false") == 0)
            ret->boolean = false;
        else
            ret->boolean = true;
    }
    else if (strcmp(name, "number") == 0)
    {
        if (xml_node->children == NULL) return NULL;
        if (xml_node->children->content == NULL) return NULL;

        malloc_size = sizeof(WDDX_NODE_int);
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_NUMBER;

        ret->number = atof((const char *)xml_node->children->content);
    }
    else if (strcmp(name, "string") == 0)
    {
        size_t str_size = 0;
        if ((xml_node->children != NULL)&&(xml_node->children->content != NULL))
            str_size = strlen((const char *)xml_node->children->content);

        malloc_size = offsetof(WDDX_NODE_int, string) + str_size + 1;
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_STRING;

        if (str_size > 0)
            strcpy(ret->string, (const char *)xml_node->children->content);
    }
    else if (strcmp(name, "array") == 0)
    {
        xmlChar *lengthStr = xmlGetProp(xml_node, BAD_CAST "length");
        if (lengthStr == NULL) return NULL;

        int length = atoi((char *)lengthStr);
        xmlFree(lengthStr); lengthStr = NULL;
        if (length <= 0) return NULL;

        malloc_size = offsetof(WDDX_NODE_int, items) + length * sizeof(void *);
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_ARRAY;
        ret->cnt = length;

        int idx = 0;
        for(xmlNodePtr child_node = xml_node->children; child_node && idx < length; child_node = child_node->next)
        {
            if (child_node->type != XML_ELEMENT_NODE) continue;

            ret->items[idx++] = wddx_from_xml_element(child_node);
        }
    }
    else if (strcmp(name, "struct") == 0)
    {
        int length = 0;

        for(xmlNodePtr child_node = xml_node->children; child_node; child_node = child_node->next)
        {
            if (child_node->type != XML_ELEMENT_NODE) continue;

            if (strcmp((const char *)child_node->name, "var") != 0) continue;

            length++;
        }

        malloc_size = offsetof(WDDX_NODE_int, items) + length * sizeof(void *);
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_STRUCT;
        ret->cnt = length;

        int idx = 0;
        for(xmlNodePtr child_node = xml_node->children; child_node && idx < length; child_node = child_node->next)
        {
            if (child_node->type != XML_ELEMENT_NODE) continue;

            if (child_node->children == NULL)
            {
                WDDX_NODE_int_free(ret);
                return NULL;
            }

            xmlChar *key = xmlGetProp(child_node, BAD_CAST "name");
            if (key == NULL)
            {
                WDDX_NODE_int_free(ret);
                return NULL;
            }

            malloc_size = sizeof(WDDX_STRUCT_NODE_int);
            WDDX_STRUCT_NODE_int *item = malloc(malloc_size);
            if (item == NULL)
            {
                WDDX_NODE_int_free(ret);
                return NULL;
            }

            explicit_bzero(item, malloc_size);

            item->name = strdup((char *)key);
            xmlFree(key); key = NULL;
            item->value = wddx_from_xml_element(child_node->children);

            ret->items[idx++] = item;
        }
    }

    return ret;
}

WDDX *wddx_from_xml(const char *xml)
{
    xmlDoc_defer(doc);
    doc = xmlParseMemory(xml, (int)strlen(xml));

    xmlNodePtr rootEl = xmlDocGetRootElement(doc);
    if (rootEl == NULL) return NULL;
    if (rootEl->type != XML_ELEMENT_NODE) return NULL;
    if (strcmp((const char *)rootEl->name, "wddxPacket") != 0) return NULL;

    xmlNodePtr headerEl = rootEl->children;
    if (headerEl == NULL) return NULL;
    if (headerEl->type != XML_ELEMENT_NODE) return NULL;
    if (strcmp((const char *)headerEl->name, "header") != 0) return NULL;

    xmlNodePtr dataEl = headerEl->next;
    if (dataEl == NULL) return NULL;
    if (dataEl->type != XML_ELEMENT_NODE) return NULL;
    if (strcmp((const char *)dataEl->name, "data") != 0) return NULL;

    WDDX_int *ret = wddx_create();
    if (ret == NULL) return NULL;

    if (headerEl->children) ret->header = wddx_from_xml_element(headerEl->children);
    if (dataEl->children) ret->data   = wddx_from_xml_element(dataEl->children);

    return ret;
}


static const WDDX_NODE_int *wddx_recursively_get(const WDDX_NODE_int *node, const char *path)
{
    if (strlen(path) == 0)
    {
        return NULL;
    }

    const char *next = strchr(path, ',');
    if (next == NULL)
    {
        if(is_string_numeric(path))
        {
            if (node->type != WDDX_ARRAY) return NULL;
            int idx = atoi(path);

            if (node->cnt <= idx) return NULL;

            return node->items[idx];
        }
        else
        {
            if (node->type != WDDX_STRUCT) return NULL;

            for(int c = 0; c < node->cnt; c++)
            {
                WDDX_STRUCT_NODE_int *item = (WDDX_STRUCT_NODE_int *)node->items[c];
                if (strcmp(item->name, path) == 0)
                    return item->value;
            }

            return NULL;
        }
    }
    else
    {
        char tmp_path[next - path + 1];
        memcpy(tmp_path, path, next - path);
        tmp_path[next - path] = 0;

        if(is_string_numeric(tmp_path))
        {
            if (node->type != WDDX_ARRAY) return NULL;
            int idx = atoi(tmp_path);

            if (node->cnt <= idx) return NULL;

            return wddx_recursively_get(node->items[idx], next + 1);
        }
        else
        {
            if (node->type != WDDX_STRUCT) return NULL;

            const WDDX_STRUCT_NODE_int *items = (WDDX_STRUCT_NODE_int *)node->items;

            for(int c = 0; c < node->cnt; c++)
            {
                if (strcmp(items[c].name, tmp_path) == 0)
                    return wddx_recursively_get(items[c].value, next + 1);
            }

            return NULL;
        }
    }

    return NULL;
}

const WDDX_NODE *wddx_header(const WDDX *src)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    return src_int->header;
}

const WDDX_NODE *wddx_data(const WDDX *src)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    return src_int->data;
}

int wddx_node_type(const WDDX_NODE *value)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->type;
}

bool wddx_node_bool(const WDDX_NODE *value)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->boolean;
}

double wddx_node_number(const WDDX_NODE *value)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->number;
}

const char *wddx_node_string(const WDDX_NODE *value)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->string;
}

int wddx_node_array_size(const WDDX_NODE *value)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->cnt;
}

const WDDX_NODE *wddx_node_array_at(const WDDX_NODE *value, int cnt)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->items[cnt];
}

int wddx_node_struct_size(const WDDX_NODE *value)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    return value_int->cnt;
}

const WDDX_NODE *wddx_node_struct_at(const WDDX_NODE *value, int cnt, const char **name)
{
    const WDDX_NODE_int *value_int = (const WDDX_NODE_int *)value;

    const WDDX_STRUCT_NODE_int *childs = (const WDDX_STRUCT_NODE_int *)&value_int->items;

    *name = childs[cnt].name;

    return childs[cnt].value;
}

bool wddx_get_bool(const WDDX *src, const char *path, bool *ok)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    if (src_int == NULL)
    {
        if (ok) *ok = false;
        return false;
    }

    const WDDX_NODE_int *node = wddx_recursively_get(src_int->data, path);

    if (node == NULL)
    {
        if (ok) *ok = false;
        return false;
    }

    if (node->type != WDDX_BOOLEAN)
    {
        if (ok) *ok = false;
        return false;
    }

    if (ok) *ok = true;
    return node->boolean;
}

double wddx_get_number(const WDDX *src, const char *path, bool *ok)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    if (src_int == NULL)
    {
        if (ok) *ok = false;
        return 0.;
    }

    const WDDX_NODE_int *node = wddx_recursively_get(src_int->data, path);

    if (node == NULL)
    {
        if (ok) *ok = false;
        return 0.;
    }

    if (node->type != WDDX_NUMBER)
    {
        if (ok) *ok = false;
        return 0.;
    }

    if (ok) *ok = true;
    return node->number;
}

const char *wddx_get_string(const WDDX *src, const char *path)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    if (src_int == NULL)
    {
        return NULL;
    }

    const WDDX_NODE_int *node = wddx_recursively_get(src_int->data, path);

    if (node == NULL)
    {
        return NULL;
    }

    if (node->type != WDDX_STRING)
    {
        return NULL;
    }

    return node->string;
}

const WDDX_NODE *wddx_get_var(const WDDX *src, const char *path)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    if (src_int == NULL)
    {
        return NULL;
    }

    return wddx_recursively_get(src_int->data, path);
}

static void wddx_node_recursively(xmlNodePtr xml, const WDDX_NODE_int *wddx)
{
    xmlNodePtr new_node = NULL;
    xmlNodePtr text_node = NULL;
    char number[32];

    if (wddx == NULL)
        return;

    switch(wddx->type)
    {
    case WDDX_BOOLEAN:
        new_node = xmlNewNode(NULL, BAD_CAST "bool");
        if (wddx->boolean == false)
            text_node = xmlNewText((const xmlChar *)"false");
        else
            text_node = xmlNewText((const xmlChar *)"true");
        xmlAddChild(new_node, text_node);
        xmlAddChild(xml, new_node);
        break;
    case WDDX_NUMBER:
        new_node = xmlNewNode(NULL, BAD_CAST "number");
        snprintf(number, sizeof(number), "%.16g", wddx->number);
        text_node = xmlNewText((const xmlChar *)number);
        xmlAddChild(new_node, text_node);
        xmlAddChild(xml, new_node);
        break;
    case WDDX_STRING:
        new_node = xmlNewNode(NULL, BAD_CAST "string");
        text_node = xmlNewText((const xmlChar *)wddx->string);
        xmlAddChild(new_node, text_node);
        xmlAddChild(xml, new_node);
        break;
    case WDDX_ARRAY:
        new_node = xmlNewNode(NULL, BAD_CAST "array");
        snprintf(number, sizeof(number), "%d", wddx->cnt);
        xmlNewProp(new_node, BAD_CAST "length", BAD_CAST number);
        for(int c = 0; c < wddx->cnt; c++)
        {
            wddx_node_recursively(new_node, wddx->items[c]);
        }
        xmlAddChild(xml, new_node);
        break;
    case WDDX_STRUCT:
        new_node = xmlNewNode(NULL, BAD_CAST "struct");
        for(int c = 0; c < wddx->cnt; c++)
        {
            const WDDX_STRUCT_NODE_int *struct_node = (const WDDX_STRUCT_NODE_int *)wddx->items[c];
              xmlNodePtr var_node = xmlNewNode(NULL, BAD_CAST "var");
              xmlNewProp(var_node, BAD_CAST "key", BAD_CAST struct_node->name);
              wddx_node_recursively(var_node, struct_node->value);
              xmlAddChild(new_node, var_node);
        }
        xmlAddChild(xml, new_node);
        break;
    default:
        break;
    }
}

char *wddx_node_to_xml(const WDDX_NODE *node)
{
    char *ret = NULL;
    const WDDX_NODE_int *src_int = (const WDDX_NODE_int *)node;

    if (src_int == NULL) return NULL;

    xmlDocPtr doc = xmlNewDoc(NULL);
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "var");
    xmlDocSetRootElement(doc, root_node);

    wddx_node_recursively(root_node, src_int);

    xmlBuffer *xml_str = xmlBufferCreate();

    xmlNodeDump(xml_str, doc, root_node, 0, 0);

    xmlFreeDoc(doc);

    ret = strdup((const char *)xmlBufferContent(xml_str));

    xmlBufferFree(xml_str);

    return ret;
}

void WDDX_NODE_int_free(WDDX_NODE_int *value)
{
    if (value == NULL) return;

    switch(value->type)
    {
    case WDDX_ARRAY:
        for(int c = 0;c < value->cnt; c++)
        {
            WDDX_NODE_int *child = (WDDX_NODE_int *)value->items[c];
            WDDX_NODE_int_free(child);
            child = NULL;
        }

        break;
    case WDDX_STRUCT:
        for(int c = 0;c < value->cnt; c++)
        {
            WDDX_STRUCT_NODE_int *child = (WDDX_STRUCT_NODE_int *)value->items[c];
            free(child->name);
            WDDX_NODE_int_free(child->value);
            child->name = NULL;
            child->value = NULL;
            free(child);
            child = NULL;
        }
        break;
    default:
        break;
    }

    free(value);
}

void wddx_cleanup(WDDX **value)
{
    if (value)
    {
        WDDX_int *value_int = (WDDX_int *)*value;

        if (value_int->header)
        {
            WDDX_NODE_int_free(value_int->header);
            value_int->header = NULL;
        }

        if (value_int->data)
        {
            WDDX_NODE_int_free(value_int->data);
            value_int->data = NULL;
        }

        if(value_int->str)
        {
            xmlBufferFree(value_int->str);
            value_int->str = NULL;
        }

        free(value_int);
        *value = NULL;
    }
}
