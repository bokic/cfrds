#include <wddx.h>

#include <libxml/tree.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>


#if defined(__APPLE__)
#define explicit_bzero bzero
#endif

#define xmlDoc_defer(var) xmlDoc* var __attribute__((cleanup(xmlDoc_cleanup))) = nullptr
#define xmlChar_defer(var) xmlChar* var __attribute__((cleanup(xmlChar_cleanup))) = nullptr

enum wddx_type {
    WDDX_BOOLEAN,
    WDDX_NUMBER,
    WDDX_STRING,
    WDDX_ARRAY,
    WDDX_STRUCT
};

typedef struct {
    int type;
    int cnt;
    union {
        bool boolean;
        int64_t integer;
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
        *value = nullptr;
    }
}

static void xmlChar_cleanup(xmlChar **value)
{
    if (value)
    {
        xmlFree(*value);
        *value = nullptr;
    }
}

static bool is_string_numeric(const char *str)
{
    if (str == nullptr || *str == '\0')
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
    int path_len = strlen(path);
    int newsize = 0;

    if (path_len == 0)
    {
        WDDX_NODE_int *new_node = nullptr;
        int value_len = strlen(value);

        int size = offsetof(WDDX_NODE_int, string) + value_len + 1;
        new_node = malloc(size);
        if (new_node == nullptr) return nullptr;

        new_node->type = type;
        memcpy(new_node->string, value, value_len + 1);

        return new_node;
    }

    const char *next = strchr(path, ',');
    if (next == nullptr)
        newsize = path_len;
    else
        newsize = next - path;
    if (newsize <= 0) return nullptr;

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

        if (node == nullptr)
        {
            newsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_NODE_int) * idx);
            node = (WDDX_NODE_int *)malloc(newsize);
            explicit_bzero(node, newsize);
            node->type = WDDX_ARRAY;
            node->cnt = idx;
        }

        switch (node->type)
        {
        case WDDX_BOOLEAN:
            return nullptr;
        case WDDX_NUMBER:
            return nullptr;
        case WDDX_STRING:
            return nullptr;
            break;
        case WDDX_ARRAY:
        {
            if(node->cnt < idx)
            {
                newsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_NODE_int) * idx);
                node = (WDDX_NODE_int *)realloc(node, newsize);
                if (node == nullptr) return nullptr;
                int oldsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_NODE_int) * node->cnt);
                explicit_bzero((void *)node + oldsize, newsize - oldsize);
                node->cnt = idx;
            }

            node->items[idx - 1] = wddx_recursively_put(node->items[idx - 1], path, value, type);
            return node;
        }
        case WDDX_STRUCT:
            return nullptr;
            break;
        default:
            return nullptr;
            break;
        }
    }
    else
    {
        if (node == nullptr)
        {
            newsize = offsetof(WDDX_NODE_int, items);
            node = (WDDX_NODE_int *)malloc(newsize);
            explicit_bzero(node, newsize);
            node->type = WDDX_STRUCT;
            node->cnt = 0;
        }

        switch (node->type)
        {
        case WDDX_BOOLEAN:
             return nullptr;
        case WDDX_NUMBER:
             return nullptr;
        case WDDX_STRING:
            return nullptr;
            break;
        case WDDX_ARRAY:
            return nullptr;
            break;
        case WDDX_STRUCT:
        {
            for(int c = 0; c < node->cnt; c++)
            {
                WDDX_STRUCT_NODE_int *child = (WDDX_STRUCT_NODE_int *)node->items[c];
                if (strcmp(child->name, newkey) == 0)
                {
                    if (child->value != nullptr) WDDX_NODE_int_free(child->value);
                    child->value = wddx_recursively_put(nullptr, path, value, type);
                    return node;
                }
            }

            newsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_STRUCT_NODE_int *) * (node->cnt + 1));
            node = (WDDX_NODE_int *)realloc(node, newsize);
            if (node == nullptr) return nullptr;
            int oldsize = offsetof(WDDX_NODE_int, items) + (sizeof(WDDX_STRUCT_NODE_int) * node->cnt);
            node->items[node->cnt] = malloc(sizeof(WDDX_STRUCT_NODE_int));
            if (node->items[node->cnt] == nullptr) return nullptr;
            ((WDDX_STRUCT_NODE_int *)node->items[node->cnt])->name = strdup(newkey);
            ((WDDX_STRUCT_NODE_int *)node->items[node->cnt])->value = wddx_recursively_put(nullptr, path, value, type);
            node->cnt++;
            return node;
        }
        default:
            return nullptr;
            break;
        }
    }

    return nullptr;
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

    if (value_int == nullptr) return false;

    value_int->data = wddx_recursively_put(value_int->data, path, value, type);

    return value_int->data != nullptr;
}

bool wddx_put_bool(WDDX *dest, const char *path, bool value)
{
    const char *valueStr = nullptr;

    if (value)
        valueStr = "true";
    else
        valueStr = "false";


    return wddx_put(dest, path, valueStr, WDDX_BOOLEAN);
}

bool wddx_put_number(WDDX *dest, const char *path, double value)
{
    char valueStr[20];

    snprintf(valueStr, sizeof(valueStr), "%.16g", value);

    return wddx_put(dest, path, valueStr, WDDX_NUMBER);
}

bool wddx_put_string(WDDX *dest, const char *path, const char *value)
{
    return wddx_put(dest, path, value, WDDX_STRING);
}

static void wddx_to_xml_node(xmlNode *xml_node, const WDDX_NODE_int *node)
{
    xmlNode *child_node = nullptr;
    char number_str[20];

    if ((xml_node == nullptr)||(node == nullptr)) return;

    switch(node->type)
    {
    case WDDX_BOOLEAN:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "boolean", nullptr);
        xmlNewProp(child_node, BAD_CAST "value", BAD_CAST node->string);
        break;
    case WDDX_NUMBER:
        xmlNewChild(xml_node, NULL, BAD_CAST "number", BAD_CAST node->string);
        break;
    case WDDX_STRING:
        xmlNewChild(xml_node, NULL, BAD_CAST "string", BAD_CAST node->string);
        break;
    case WDDX_ARRAY:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "array", nullptr);

        snprintf(number_str, sizeof(number_str), "%d", node->cnt);

        xmlNewProp(child_node, BAD_CAST "length", BAD_CAST number_str);

        for(int c = 0; c < node->cnt; c++)
        {
            wddx_to_xml_node(child_node, ((const WDDX_NODE_int *)node)->items[c]);
        }
        break;
    case WDDX_STRUCT:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "struct", nullptr);

        xmlNewProp(child_node, BAD_CAST "type", BAD_CAST "java.util.HashMap");

        for(int c = 0; c < node->cnt; c++)
        {
            WDDX_STRUCT_NODE_int *var = (WDDX_STRUCT_NODE_int *)node->items[c];

            xmlNode *var_node = xmlNewChild(child_node, NULL, BAD_CAST "var", nullptr);

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
        value_int->str = nullptr;
    }

    xmlDocPtr doc = xmlNewDoc(nullptr);
    xmlNodePtr root_node = xmlNewNode(nullptr, BAD_CAST "wddxPacket");
    xmlNewProp(root_node, BAD_CAST "version", BAD_CAST "1.0");
    xmlDocSetRootElement(doc, root_node);

    xmlNode *headerNode = xmlNewChild(root_node, nullptr, BAD_CAST "header", nullptr);
    wddx_to_xml_node(headerNode, value_int->header);

    xmlNode *dataNode = xmlNewChild(root_node, nullptr, BAD_CAST "data", nullptr);
    wddx_to_xml_node(dataNode, value_int->data);

    value_int->str = xmlBufferCreate();

    xmlNodeDump(value_int->str, doc, root_node, 0, 0);

    xmlFreeDoc(doc);

    return (const char *)xmlBufferContent(value_int->str);
}

static WDDX_NODE_int *wddx_from_xml_element(xmlNodePtr xml_node)
{
    WDDX_NODE_int *ret = nullptr;
    int malloc_size = 0;

    if (xml_node == nullptr) return nullptr;
    if (xml_node->type != XML_ELEMENT_NODE) return nullptr;
    if (xml_node->children == nullptr) return nullptr;

    const char *name = (const char *)xml_node->name;
    if (name == nullptr) return nullptr;

    if (strcmp(name, "boolean") == 0)
    {
        if (xml_node->children == nullptr) return nullptr;
        if (xml_node->children->content == nullptr) return nullptr;

        malloc_size = sizeof(WDDX_NODE_int);
        ret = malloc(malloc_size);
        if (ret == nullptr) return nullptr;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_BOOLEAN;

        if (strcmp((const char *)xml_node->children->content, "false") == 0)
            ret->boolean = false;
        else
            ret->boolean = true;
    }
    else if (strcmp(name, "number") == 0)
    {
        if (xml_node->children == nullptr) return nullptr;
        if (xml_node->children->content == nullptr) return nullptr;

        malloc_size = sizeof(WDDX_NODE_int);
        ret = malloc(malloc_size);
        if (ret == nullptr) return nullptr;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_NUMBER;

        ret->number = atof((const char *)xml_node->children->content);
    }
    else if (strcmp(name, "string") == 0)
    {
        if (xml_node->children == nullptr) return nullptr;
        if (xml_node->children->content == nullptr) return nullptr;

        malloc_size = offsetof(WDDX_NODE_int, string) + strlen((const char *)xml_node->children->content) + 1;
        ret = malloc(malloc_size);
        if (ret == nullptr) return nullptr;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_STRING;

        strcpy(ret->string, (const char *)xml_node->children->content);
    }
    else if (strcmp(name, "array") == 0)
    {
        xmlChar *lengthStr = xmlGetProp(xml_node, BAD_CAST "length");
        if (lengthStr == nullptr) return nullptr;

        int length = atoi((char *)lengthStr);
        xmlFree(lengthStr); lengthStr = nullptr;
        if (length <= 0) return nullptr;

        malloc_size = offsetof(WDDX_NODE_int, items) + length * sizeof(void *);
        ret = malloc(malloc_size);
        if (ret == nullptr) return nullptr;

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
        if (ret == nullptr) return nullptr;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_STRUCT;
        ret->cnt = length;

        int idx = 0;
        for(xmlNodePtr child_node = xml_node->children; child_node && idx < length; child_node = child_node->next)
        {
            if (child_node->type != XML_ELEMENT_NODE) continue;

            if (child_node->children == nullptr) return nullptr;

            malloc_size = sizeof(WDDX_STRUCT_NODE_int);
            WDDX_STRUCT_NODE_int *item = malloc(malloc_size);
            if (item == nullptr) return nullptr;

            explicit_bzero(item, malloc_size);

            xmlChar *key = xmlGetProp(child_node, BAD_CAST "name");
            if (key == nullptr) return nullptr;
            item->name = strdup((char *)key);
            xmlFree(key); key = nullptr;
            item->value = wddx_from_xml_element(child_node->children);

            ret->items[idx++] = item;
        }
    }

    return ret;
}

WDDX *wddx_from_xml(const char *xml)
{
    xmlDoc_defer(doc);
    doc = xmlParseMemory(xml, strlen(xml));

    xmlNodePtr rootEl = xmlDocGetRootElement(doc);
    if (rootEl == nullptr) return nullptr;
    if (rootEl->type != XML_ELEMENT_NODE) return nullptr;
    if (strcmp((const char *)rootEl->name, "wddxPacket") != 0) return nullptr;

    xmlNodePtr headerEl = rootEl->children;
    if (headerEl == nullptr) return nullptr;
    if (headerEl->type != XML_ELEMENT_NODE) return nullptr;
    if (strcmp((const char *)headerEl->name, "header") != 0) return nullptr;

    xmlNodePtr dataEl = headerEl->next;
    if (dataEl == nullptr) return nullptr;
    if (dataEl->type != XML_ELEMENT_NODE) return nullptr;
    if (strcmp((const char *)dataEl->name, "data") != 0) return nullptr;

    WDDX_int *ret = wddx_create();
    if (ret == nullptr) return nullptr;

    if (headerEl->children) ret->header = wddx_from_xml_element(headerEl->children);
    if (dataEl->children) ret->data   = wddx_from_xml_element(dataEl->children);

    return ret;
}


static const WDDX_NODE_int *wddx_recursively_get(const WDDX_NODE_int *node, const char *path)
{
    if (strlen(path) == 0)
    {
        return nullptr;
    }

    const char *next = strchr(path, ',');
    if (next == nullptr)
    {
        if(is_string_numeric(path))
        {
            if (node->type != WDDX_ARRAY) return nullptr;
            int idx = atoi(path);

            if (node->cnt <= idx) return nullptr;

            return node->items[idx];
        }
        else
        {
            if (node->type != WDDX_STRUCT) return nullptr;

            for(int c = 0; c < node->cnt; c++)
            {
                WDDX_STRUCT_NODE_int *item = (WDDX_STRUCT_NODE_int *)node->items[c];
                if (strcmp(item->name, path) == 0)
                    return item->value;
            }

            return nullptr;
        }
    }
    else
    {
        char tmp_path[next - path + 1];
        memcpy(tmp_path, path, next - path);
        tmp_path[next - path] = 0;

        if(is_string_numeric(tmp_path))
        {
            if (node->type != WDDX_ARRAY) return nullptr;
            int idx = atoi(tmp_path);

            if (node->cnt <= idx) return nullptr;

            return wddx_recursively_get(node->items[idx], next + 1);
        }
        else
        {
            if (node->type != WDDX_STRUCT) return nullptr;

            WDDX_STRUCT_NODE_int * items = (WDDX_STRUCT_NODE_int *)node->items;

            for(int c = 0; c < node->cnt; c++)
            {
                if (strcmp(items[c].name, tmp_path) == 0)
                    return wddx_recursively_get(items[c].value, next + 1);
            }

            return nullptr;
        }
    }

    return nullptr;
}

bool wddx_get_bool(const WDDX *src, const char *path, bool *ok)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    if (src_int == nullptr)
    {
        if (ok) *ok = false;
        return false;
    }

    const WDDX_NODE_int *node = wddx_recursively_get(src_int->data, path);

    if (node == nullptr)
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

    if (src_int == nullptr)
    {
        if (ok) *ok = false;
        return 0.;
    }

    const WDDX_NODE_int *node = wddx_recursively_get(src_int->data, path);

    if (node == nullptr)
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

    if (src_int == nullptr)
    {
        return nullptr;
    }

    const WDDX_NODE_int *node = wddx_recursively_get(src_int->data, path);

    if (node == nullptr)
    {
        return nullptr;
    }

    if (node->type != WDDX_STRING)
    {
        return nullptr;
    }

    return node->string;
}

const WDDX_NODE *wddx_get_var(const WDDX *src, const char *path)
{
    const WDDX_int *src_int = (const WDDX_int *)src;

    if (src_int == nullptr)
    {
        return nullptr;
    }

    return wddx_recursively_get(src_int->data, path);
}

static void wddx_node_recursively(xmlNodePtr xml, const WDDX_NODE_int *wddx)
{
    xmlNodePtr new_node = nullptr;
    xmlNodePtr text_node = nullptr;
    char number[32];

    if (wddx == nullptr)
        return;

    switch(wddx->type)
    {
    case WDDX_BOOLEAN:
        new_node = xmlNewNode(nullptr, BAD_CAST "bool");
        if (wddx->boolean == false)
            text_node = xmlNewText((const xmlChar *)"false");
        else
            text_node = xmlNewText((const xmlChar *)"true");
        xmlAddChild(new_node, text_node);
        xmlAddChild(xml, new_node);
        break;
    case WDDX_NUMBER:
        new_node = xmlNewNode(nullptr, BAD_CAST "number");
        snprintf(number, sizeof(number), "%.16g", wddx->number);
        text_node = xmlNewText((const xmlChar *)number);
        xmlAddChild(new_node, text_node);
        xmlAddChild(xml, new_node);
        break;
    case WDDX_STRING:
        new_node = xmlNewNode(nullptr, BAD_CAST "string");
        text_node = xmlNewText((const xmlChar *)wddx->string);
        xmlAddChild(new_node, text_node);
        xmlAddChild(xml, new_node);
        break;
    case WDDX_ARRAY:
        new_node = xmlNewNode(nullptr, BAD_CAST "array");
        snprintf(number, sizeof(number), "%d", wddx->cnt);
        xmlNewProp(new_node, BAD_CAST "length", BAD_CAST number);
        for(int c = 0; c < wddx->cnt; c++)
        {
            wddx_node_recursively(new_node, wddx->items[c]);
        }
        xmlAddChild(xml, new_node);
        break;
    case WDDX_STRUCT:
        new_node = xmlNewNode(nullptr, BAD_CAST "struct");
        for(int c = 0; c < wddx->cnt; c++)
        {
            const WDDX_STRUCT_NODE_int *struct_node = (const WDDX_STRUCT_NODE_int *)wddx->items[c];
              xmlNodePtr var_node = xmlNewNode(nullptr, BAD_CAST "var");
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
    char *ret = nullptr;
    const WDDX_NODE_int *src_int = (const WDDX_NODE_int *)node;

    if (src_int == nullptr) return nullptr;

    xmlDocPtr doc = xmlNewDoc(nullptr);
    xmlNodePtr root_node = xmlNewNode(nullptr, BAD_CAST "var");
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
    if (value == nullptr) return;

    switch(value->type)
    {
    case WDDX_ARRAY:
        for(int c = 0;c < value->cnt; c++)
        {
            WDDX_NODE_int *child = (WDDX_NODE_int *)value->items[c];
            WDDX_NODE_int_free(child);
            child = nullptr;
        }

        break;
    case WDDX_STRUCT:
        for(int c = 0;c < value->cnt; c++)
        {
            WDDX_STRUCT_NODE_int *child = (WDDX_STRUCT_NODE_int *)value->items[c];
            free(child->name);
            WDDX_NODE_int_free(child->value);
            child->name = nullptr;
            child->value = nullptr;
            free(child);
            child = nullptr;
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
            value_int->header = nullptr;
        }

        if (value_int->data)
        {
            WDDX_NODE_int_free(value_int->data);
            value_int->data = nullptr;
        }

        if(value_int->str)
        {
            xmlBufferFree(value_int->str);
            value_int->str = nullptr;
        }

        free(value_int);
    }
}
