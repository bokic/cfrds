#include <internal/wddx.h>
#include <internal/cfrds_buffer.h>
#include <internal/explicit_bzero.h>

#include <libxml/tree.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>


#define WDDX_MAX_ARRAY_LENGTH 10000

#define xmlDoc_defer(var) xmlDoc* var __attribute__((cleanup(xmlDoc_cleanup))) = NULL

struct WDDX_NODE {
    int type;
    int cnt;
    union {
        bool boolean;
        double number;
        void *items[1];
        char string[1];
    };
};

typedef struct {
    char *name;
    struct WDDX_NODE *value;
} WDDX_STRUCT_NODE;

struct WDDX {
    struct WDDX_NODE *header;
    struct WDDX_NODE *data;
    xmlBufferPtr str;
};

static void wddx_node_free(struct WDDX_NODE *value);

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

    if (len > 20) return false;

    for (size_t i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
            return false;
    }

    return true;
}

static struct WDDX_NODE *wddx_recursively_put(struct WDDX_NODE *node, const char *path, const char *value, enum wddx_type type)
{
    size_t path_len = strlen(path);
    size_t newsize = 0;
    bool created_node = false;

    if (path_len == 0)
    {
        struct WDDX_NODE *new_node = NULL;
        size_t value_len = strlen(value);

        size_t size = offsetof(struct WDDX_NODE, string) + value_len + 1;
        new_node = malloc(size);
        if (new_node == NULL) return NULL;

        new_node->type = type;
        memcpy(new_node->string, value, value_len + 1);

        return new_node;
    }

    const char *next = strchr(path, ',');
    size_t seg_len = (next == NULL) ? path_len : (size_t)(next - path);
    if (seg_len == 0) return NULL;

    char newkey[seg_len + 1];
    memcpy(newkey, path, seg_len);
    newkey[seg_len] = 0;
    if (next)
        path += seg_len + 1;
    else
        path += seg_len;

    if(is_string_numeric(newkey))
    {
        long parsed = strtol(newkey, NULL, 10);
        if (parsed < 0 || parsed >= WDDX_MAX_ARRAY_LENGTH) return NULL;
        int idx = (int)parsed + 1;
        if (idx < 1 || idx > WDDX_MAX_ARRAY_LENGTH) return NULL;

        if (node == NULL)
        {
            newsize = offsetof(struct WDDX_NODE, items) + (sizeof(void *) * (size_t)idx);
            node = malloc(newsize);
            if (node == NULL)
            {
                return NULL;
            }
            created_node = true;
            explicit_bzero(node, newsize);
            node->type = WDDX_ARRAY;
            node->cnt = idx;
        }

        switch (node->type)
        {
        case WDDX_NULL:
        case WDDX_BOOLEAN:
        case WDDX_NUMBER:
        case WDDX_STRING:
        case WDDX_STRUCT:
            if (created_node) wddx_node_free(node);
            return NULL;
        case WDDX_ARRAY:
        {
            if(node->cnt < idx)
            {
                newsize = offsetof(struct WDDX_NODE, items) + (sizeof(void *) * (size_t)idx);
                struct WDDX_NODE *pre_realloc_node = node;
                node = realloc(node, newsize);
                if (node == NULL) {
                    if (created_node) wddx_node_free(pre_realloc_node);
                    return NULL;
                }
                size_t oldsize = offsetof(struct WDDX_NODE, items) + (sizeof(void *) * (size_t)node->cnt);
                explicit_bzero((char *)node + oldsize, newsize - oldsize);
                node->cnt = idx;
            }

            struct WDDX_NODE *new_child = wddx_recursively_put(node->items[idx - 1], path, value, type);
            if (new_child == NULL && node->items[idx - 1] != NULL) {
                if (created_node) {
                    node->items[idx - 1] = NULL;
                    wddx_node_free(node);
                }
                return NULL;
            }
            node->items[idx - 1] = new_child;
            return node;
        }
        default:
            if (created_node) wddx_node_free(node);
            return NULL;
        }
    }
    else
    {
        if (node == NULL)
        {
            newsize = sizeof(struct WDDX_NODE);
            node = malloc(newsize);
            if (node == NULL)
                return NULL;
            explicit_bzero(node, newsize);
            node->type = WDDX_STRUCT;
            node->cnt = 0;
            created_node = true;
        }

        switch (node->type)
        {
        case WDDX_NULL:
        case WDDX_BOOLEAN:
        case WDDX_NUMBER:
        case WDDX_STRING:
        case WDDX_ARRAY:
            if (created_node) wddx_node_free(node);
            return NULL;
        case WDDX_STRUCT:
        {
            for(int c = 0; c < node->cnt; c++)
            {
                WDDX_STRUCT_NODE *child = node->items[c];
                if (child == NULL || child->name == NULL) continue;
                if (strcmp(child->name, newkey) == 0)
                {
                    struct WDDX_NODE *new_val = wddx_recursively_put(child->value, path, value, type);
                    if (new_val == NULL && child->value != NULL)
                    {
                        if (created_node) wddx_node_free(node);
                        return NULL;
                    }
                    child->value = new_val;
                    return node;
                }
            }

            newsize = offsetof(struct WDDX_NODE, items) + (sizeof(WDDX_STRUCT_NODE *) * (size_t)(node->cnt + 1));
            struct WDDX_NODE *pre_realloc_node = node;
            node = realloc(node, newsize);
            if (node == NULL) {
                if (created_node) wddx_node_free(pre_realloc_node);
                return NULL;
            }
            node->items[node->cnt] = malloc(sizeof(WDDX_STRUCT_NODE));
            if (node->items[node->cnt] == NULL) {
                if (created_node) wddx_node_free(node);
                return NULL;
            }
            char *tstr = strdup(newkey);
            if (tstr == NULL) {
                free(node->items[node->cnt]);
                if (created_node) wddx_node_free(node);
                return NULL;
            }
            struct WDDX_NODE *new_val = wddx_recursively_put(NULL, path, value, type);
            if (new_val == NULL) {
                free(tstr);
                free(node->items[node->cnt]);
                if (created_node) wddx_node_free(node);
                return NULL;
            }
            WDDX_STRUCT_NODE *sitem = node->items[node->cnt];
            sitem->name = tstr;
            sitem->value = new_val;
            node->cnt++;
            return node;
        }
        default:
            if (created_node) wddx_node_free(node);
            return NULL;
        }
    }

    if (created_node) wddx_node_free(node);
    return NULL;
}

WDDX *wddx_create(void)
{
    struct WDDX *ret = malloc(sizeof(struct WDDX));
    if (ret)
    {
        explicit_bzero(ret, sizeof(struct WDDX));
    }

    return ret;
}

static bool wddx_put(WDDX *dest, const char *path, const char *value, enum wddx_type type)
{
    if (dest == NULL) return false;

    struct WDDX_NODE *new_data = wddx_recursively_put(dest->data, path, value, type);
    if (new_data == NULL && dest->data != NULL) {
        return false;
    }

    dest->data = new_data;

    return dest->data != NULL;
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
    char valueStr[64];

    snprintf(valueStr, sizeof(valueStr), "%.16g", value);

    return wddx_put(dest, path, valueStr, WDDX_NUMBER);
}

bool wddx_put_string(WDDX *dest, const char *path, const char *value)
{
    return wddx_put(dest, path, value, WDDX_STRING);
}

static void wddx_to_xml_node(xmlNode *xml_node, const struct WDDX_NODE *node)
{
    xmlNode *child_node = NULL;
    char number_str[20];

    if ((xml_node == NULL)||(node == NULL)) return;

    switch(node->type)
    {
    case WDDX_NULL:
        xmlNewChild(xml_node, NULL, BAD_CAST "null", NULL);
        break;
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
            wddx_to_xml_node(child_node, node->items[c]);
        }
        break;
    case WDDX_STRUCT:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "struct", NULL);

        xmlNewProp(child_node, BAD_CAST "type", BAD_CAST "java.util.HashMap");

        for(int c = 0; c < node->cnt; c++)
        {
            const WDDX_STRUCT_NODE *var = node->items[c];
            if (var == NULL || var->name == NULL) continue;

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
    if (src->str)
    {
        xmlBufferFree(src->str);
        src->str = NULL;
    }

    xmlDocPtr doc = xmlNewDoc(NULL);
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "wddxPacket");
    xmlNewProp(root_node, BAD_CAST "version", BAD_CAST "1.0");
    xmlDocSetRootElement(doc, root_node);

    xmlNode *headerNode = xmlNewChild(root_node, NULL, BAD_CAST "header", NULL);
    wddx_to_xml_node(headerNode, src->header);

    xmlNode *dataNode = xmlNewChild(root_node, NULL, BAD_CAST "data", NULL);
    wddx_to_xml_node(dataNode, src->data);

    src->str = xmlBufferCreate();

    xmlNodeDump(src->str, doc, root_node, 0, 0);

    xmlFreeDoc(doc);

    return (const char *)xmlBufferContent(src->str);
}

static struct WDDX_NODE *wddx_from_xml_element(xmlNodePtr xml_node)
{
    struct WDDX_NODE *ret = NULL;
    size_t malloc_size = 0;

    if (xml_node == NULL) return NULL;
    if (xml_node->type != XML_ELEMENT_NODE) return NULL;

    const char *name = (const char *)xml_node->name;
    if (name == NULL) return NULL;

    if (strcmp(name, "null") == 0)
    {
        malloc_size = sizeof(struct WDDX_NODE);
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_NULL;
    }
    else if (strcmp(name, "boolean") == 0)
    {
        xmlChar *valueStr = xmlGetProp(xml_node, BAD_CAST "value");
        if (valueStr == NULL) return NULL;

        malloc_size = sizeof(struct WDDX_NODE);
        ret = malloc(malloc_size);
        if (ret == NULL) {
            xmlFree(valueStr);
            return NULL;
        }

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_BOOLEAN;

        if (strcmp((const char *)valueStr, "false") == 0)
            ret->boolean = false;
        else
            ret->boolean = true;

        xmlFree(valueStr);
    }
    else if (strcmp(name, "number") == 0)
    {
        if (xml_node->children == NULL) return NULL;
        if (xml_node->children->content == NULL) return NULL;

        malloc_size = sizeof(struct WDDX_NODE);
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

        malloc_size = offsetof(struct WDDX_NODE, string) + str_size + 1;
        ret = malloc(malloc_size);
        if (ret == NULL) return NULL;

        explicit_bzero(ret, malloc_size);

        ret->type = WDDX_STRING;

        if (str_size > 0) {
            strncpy(ret->string, (const char *)xml_node->children->content, str_size);
            ret->string[str_size] = '\0';
        }
    }
    else if (strcmp(name, "array") == 0)
    {
        xmlChar *lengthStr = xmlGetProp(xml_node, BAD_CAST "length");
        if (lengthStr == NULL) return NULL;

        long parsed_len = strtol((char *)lengthStr, NULL, 10);
        xmlFree(lengthStr); lengthStr = NULL;
        if (parsed_len <= 0 || parsed_len > WDDX_MAX_ARRAY_LENGTH) return NULL;
        int length = (int)parsed_len;

        malloc_size = offsetof(struct WDDX_NODE, items) + (size_t)length * sizeof(void *);
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

        malloc_size = offsetof(struct WDDX_NODE, items) + (size_t)length * sizeof(void *);
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
                wddx_node_free(ret);
                return NULL;
            }

            xmlChar *key = xmlGetProp(child_node, BAD_CAST "name");
            if (key == NULL)
            {
                wddx_node_free(ret);
                return NULL;
            }

            malloc_size = sizeof(WDDX_STRUCT_NODE);
            WDDX_STRUCT_NODE *item = malloc(malloc_size);
            if (item == NULL)
            {
                xmlFree(key);
                wddx_node_free(ret);
                return NULL;
            }

            explicit_bzero(item, malloc_size);

            item->name = strdup((char *)key);
            xmlFree(key); key = NULL;
            if (item->name == NULL)
            {
                free(item);
                wddx_node_free(ret);
                return NULL;
            }
            item->value = wddx_from_xml_element(child_node->children);

            ret->items[idx++] = item;
        }
    }

    return ret;
}

#ifndef NDEBUG
static void silentErrorHandler(void *ctx, const char *msg, ...) {
    (void)ctx;
    (void)msg;
    // Do nothing - silencing output
}
#endif

WDDX *wddx_from_xml(const char *xml)
{
    xmlDoc_defer(doc);
    size_t xml_len = 0;

    if (xml == NULL) return NULL;

    xml_len = strlen(xml);

    if (xml_len == 0) return NULL;

#ifndef NDEBUG
    xmlSetGenericErrorFunc(NULL, silentErrorHandler);
#endif

    if (xml_len > INT_MAX)
        return NULL;

    doc = xmlParseMemory(xml, (int)xml_len);

    xmlNodePtr rootEl = xmlDocGetRootElement(doc);
    if (rootEl == NULL) return NULL;

    while(rootEl->type == XML_TEXT_NODE)
    {
        if (rootEl->next == NULL) return NULL;
        rootEl = rootEl->next;
    }

    if (rootEl->type != XML_ELEMENT_NODE) return NULL;

    if (strcmp((const char *)rootEl->name, "wddxPacket") != 0) return NULL;

    xmlNodePtr headerEl = rootEl->children;
    if (headerEl == NULL) return NULL;

    while(headerEl->type == XML_TEXT_NODE)
    {
        if (headerEl->next == NULL) return NULL;
        headerEl = headerEl->next;
    }

    if (headerEl->type != XML_ELEMENT_NODE) return NULL;
    if (strcmp((const char *)headerEl->name, "header") != 0) return NULL;

    xmlNodePtr dataEl = headerEl->next;
    if (dataEl == NULL) return NULL;

    while(dataEl->type == XML_TEXT_NODE)
    {
        if (dataEl->next == NULL) return NULL;
        dataEl = dataEl->next;
    }

    if (dataEl->type != XML_ELEMENT_NODE) return NULL;
    if (strcmp((const char *)dataEl->name, "data") != 0) return NULL;

    struct WDDX *ret = wddx_create();
    if (ret == NULL) return NULL;

    if (headerEl->children) ret->header = wddx_from_xml_element(headerEl->children);
    if (dataEl->children) ret->data   = wddx_from_xml_element(dataEl->children);

    return ret;
}


static const struct WDDX_NODE *wddx_recursively_get(const struct WDDX_NODE *node, const char *path)
{
    if (node == NULL)
        return NULL;

    if (strlen(path) == 0)
    {
        return node;
    }

    const char *next = strchr(path, ',');
    if (next == NULL)
    {
        if(is_string_numeric(path))
        {
            long parsed_idx = strtol(path, NULL, 10);
            if (parsed_idx < 0 || parsed_idx >= node->cnt) return NULL;
            int idx = (int)parsed_idx;

            return node->items[idx];
        }
        else
        {
            if (node->type != WDDX_STRUCT) return NULL;

            for(int c = 0; c < node->cnt; c++)
            {
                const WDDX_STRUCT_NODE *item = node->items[c];
                if (item == NULL || item->name == NULL) continue;
                if (strcmp(item->name, path) == 0)
                    return item->value;
            }

            return NULL;
        }
    }
    else
    {
        size_t seg_len = (size_t)(next - path);
        char tmp_path[seg_len + 1];
        memcpy(tmp_path, path, seg_len);
        tmp_path[seg_len] = 0;

        if(is_string_numeric(tmp_path))
        {
            long parsed_idx = strtol(tmp_path, NULL, 10);
            if (parsed_idx < 0 || parsed_idx >= node->cnt) return NULL;
            int idx = (int)parsed_idx;

            return wddx_recursively_get(node->items[idx], next + 1);
        }
        else
        {
            if (node->type != WDDX_STRUCT) return NULL;

            for(int c = 0; c < node->cnt; c++)
            {
                const WDDX_STRUCT_NODE *item = node->items[c];
                if (item == NULL || item->name == NULL) continue;
                if (strcmp(item->name, tmp_path) == 0)
                    return wddx_recursively_get(item->value, next + 1);
            }

            return NULL;
        }
    }
}

static __attribute__((unused)) const WDDX_NODE *wddx_header(const WDDX *src)
{
    if (src == NULL)
        return NULL;

    return src->header;
}

const WDDX_NODE *wddx_data(const void *src_ptr)
{
    const struct WDDX *src = src_ptr;
    if (src == NULL)
        return NULL;

    return src->data;
}

int wddx_node_type(const void *value_ptr)
{
    const struct WDDX_NODE *value = value_ptr;
    if (value == NULL)
        return WDDX_NULL;

    return value->type;
}

static __attribute__((unused)) bool wddx_node_bool(const WDDX_NODE *value)
{
    if (value == NULL)
        return false;

    return value->boolean;
}

static __attribute__((unused)) double wddx_node_number(const WDDX_NODE *value)
{
    if (value == NULL)
        return NAN;

    return value->number;
}

const char *wddx_node_string(const WDDX_NODE *value)
{
    if (value == NULL)
        return NULL;

    return value->string;
}

int wddx_node_array_size(const void *value_ptr)
{
    const struct WDDX_NODE *value = value_ptr;
    if (value == NULL)
        return 0;

    return value->cnt;
}

const WDDX_NODE *wddx_node_array_at(const void *value_ptr, size_t cnt)
{
    const struct WDDX_NODE *value = value_ptr;
    if (value == NULL)
        return NULL;

    if (cnt >= (size_t)value->cnt)
        return NULL;

    return value->items[cnt];
}

int wddx_node_struct_size(const void *value_ptr)
{
    const struct WDDX_NODE *value = value_ptr;
    if (value == NULL)
        return 0;

    return value->cnt;
}

const WDDX_NODE *wddx_node_struct_at(const void *value_ptr, size_t cnt, const char **name)
{
    const struct WDDX_NODE *value = value_ptr;
    if (value == NULL)
        return NULL;

    if (cnt >= (size_t)value->cnt)
        return NULL;

    const WDDX_STRUCT_NODE *child = value->items[cnt];

    if (child == NULL)
        return NULL;

    if (name) *name = child->name;

    return child->value;
}

static __attribute__((unused)) bool wddx_get_bool(const void *src_ptr, const char *path, bool *ok)
{
    const struct WDDX *src = src_ptr;
    if (src == NULL)
    {
        if (ok) *ok = false;
        return false;
    }

    const struct WDDX_NODE *node = wddx_recursively_get(src->data, path);

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

double wddx_get_number(const void *src_ptr, const char *path, bool *ok)
{
    const struct WDDX *src = src_ptr;
    if (src == NULL)
    {
        if (ok) *ok = false;
        return 0.;
    }

    const struct WDDX_NODE *node = wddx_recursively_get(src->data, path);

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

const char *wddx_get_string(const void *src_ptr, const char *path)
{
    const struct WDDX *src = src_ptr;
    if (src == NULL)
    {
        return NULL;
    }

    const struct WDDX_NODE *node = wddx_recursively_get(src->data, path);

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

const WDDX_NODE *wddx_get_var(const void *src_ptr, const char *path)
{
    const struct WDDX *src = src_ptr;
    if (src == NULL)
    {
        return NULL;
    }

    return wddx_recursively_get(src->data, path);
}

static void wddx_node_recursively(xmlNodePtr xml, const struct WDDX_NODE *wddx)
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
            const WDDX_STRUCT_NODE *struct_node = wddx->items[c];
            if (struct_node == NULL || struct_node->name == NULL) continue;
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

static __attribute__((unused)) char *wddx_node_to_xml(const WDDX_NODE *node)
{
    char *ret = NULL;

    if (node == NULL) return NULL;

    xmlDocPtr doc = xmlNewDoc(NULL);
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "var");
    xmlDocSetRootElement(doc, root_node);

    wddx_node_recursively(root_node, node);

    xmlBuffer *xml_str = xmlBufferCreate();

    if (xml_str == NULL) {
        xmlFreeDoc(doc);
        return NULL;
    }

    xmlNodeDump(xml_str, doc, root_node, 0, 0);

    xmlFreeDoc(doc);

    ret = strdup((const char *)xmlBufferContent(xml_str));

    xmlBufferFree(xml_str);

    return ret;
}

static void wddx_node_free(struct WDDX_NODE *value)
{
    if (value == NULL) return;

    switch(value->type)
    {
    case WDDX_ARRAY:
        for(int c = 0;c < value->cnt; c++)
        {
            struct WDDX_NODE *child = value->items[c];
            wddx_node_free(child);
            child = NULL;
        }

        break;
    case WDDX_STRUCT:
        for(int c = 0;c < value->cnt; c++)
        {
            WDDX_STRUCT_NODE *child = value->items[c];
            if (child == NULL) continue;
            free(child->name);
            wddx_node_free(child->value);
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

void wddx_cleanup(void *value_ptr)
{
    if (value_ptr && *(struct WDDX **)value_ptr)
    {
        struct WDDX **v = value_ptr;
        if ((*v)->header)
        {
            wddx_node_free((*v)->header);
            (*v)->header = NULL;
        }

        if ((*v)->data)
        {
            wddx_node_free((*v)->data);
            (*v)->data = NULL;
        }

        if ((*v)->str)
        {
            xmlBufferFree((*v)->str);
            (*v)->str = NULL;
        }

        free(*v);
        *v = NULL;
    }
}
