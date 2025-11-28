#include <internal/wddx.h>

#include <libxml/tree.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>


enum wddx_type {
    WDDX_BOOLEAN,
    WDDX_NUMBER,
    WDDX_STRING,
    WDDX_ARRAY,
    WDDX_MAP
};

typedef struct {
    int type;
    int cnt;
    union {
        void *items[];
        char string[];
    };
} WDDX_node;

typedef struct {
    char *name;
    WDDX_node *value;
} WDDX_map_node;

typedef struct  {
    WDDX_node *header;
    WDDX_node *data;
    xmlBufferPtr str;
} WDDX_int;

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

static WDDX_node *wddx_recursively_put(WDDX_node *node, const char *path, const char *value, enum wddx_type type)
{
    int value_len = strlen(value);
    int path_len = strlen(path);
    int newsize = 0;

    if (path_len == 0)
    {
        WDDX_node *new_node = nullptr;

        int size = offsetof(WDDX_node, string) + value_len + 1;
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
            newsize = offsetof(WDDX_node, items) + (sizeof(WDDX_node) * idx);
            node = (WDDX_node *)malloc(newsize);
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
                newsize = offsetof(WDDX_node, items) + (sizeof(WDDX_node) * idx);
                node = (WDDX_node *)realloc(node, newsize);
                if (node == nullptr) return nullptr;
                node->cnt = idx;
            }

            node->items[idx - 1] = wddx_recursively_put(node->items[idx - 1], path, value, type);
            return node;
        }
        case WDDX_MAP:
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
            newsize = offsetof(WDDX_node, items);
            node = (WDDX_node *)malloc(newsize);
            explicit_bzero(node, newsize);
            node->type = WDDX_MAP;
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
        case WDDX_MAP:
        {
            // TODO: Search for existing.
            newsize = offsetof(WDDX_node, items) + (sizeof(WDDX_map_node *) * (node->cnt + 1));
            node = (WDDX_node *)realloc(node, newsize);
            if (node == nullptr) return nullptr;
            node->items[node->cnt] = malloc(sizeof(WDDX_map_node));
            if (node->items[node->cnt] == nullptr) return nullptr;
            ((WDDX_map_node *)node->items[node->cnt])->name = strdup(newkey);
            ((WDDX_map_node *)node->items[node->cnt])->value = wddx_recursively_put(((WDDX_map_node *)node->items[node->cnt])->value, path, value, type);
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

    snprintf(valueStr, sizeof(valueStr), "%f", value);

    return wddx_put(dest, path, valueStr, WDDX_NUMBER);
}

bool wddx_put_string(WDDX *dest, const char *path, const char *value)
{
    return wddx_put(dest, path, value, WDDX_STRING);
}

static void wddx_to_xml_node(xmlNode *xml_node, const WDDX_node *wddx_node)
{
    xmlNode *child_node = nullptr;
    char number_str[20];

    if ((xml_node == nullptr)||(wddx_node == nullptr)) return;

    switch(wddx_node->type)
    {
    case WDDX_BOOLEAN:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "boolean", nullptr);
        xmlNewProp(child_node, BAD_CAST "value", BAD_CAST wddx_node->string);
        break;
    case WDDX_NUMBER:
        xmlNewChild(xml_node, NULL, BAD_CAST "number", BAD_CAST wddx_node->string);
        break;
    case WDDX_STRING:
        xmlNewChild(xml_node, NULL, BAD_CAST "string", BAD_CAST wddx_node->string);
        break;
    case WDDX_ARRAY:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "array", nullptr);

        snprintf(number_str, sizeof(number_str), "%d", wddx_node->cnt);

        xmlNewProp(child_node, BAD_CAST "length", BAD_CAST number_str);

        for(int c = 0; c < wddx_node->cnt; c++)
        {
            wddx_to_xml_node(child_node, (const WDDX_node *)wddx_node->items[c]);
        }
        break;
    case WDDX_MAP:
        child_node = xmlNewChild(xml_node, NULL, BAD_CAST "struct", nullptr);

        xmlNewProp(child_node, BAD_CAST "type", BAD_CAST "java.util.HashMap");

        for(int c = 0; c < wddx_node->cnt; c++)
        {
            WDDX_map_node *var = (WDDX_map_node *)wddx_node->items[c];

            xmlNode *var_node = xmlNewChild(child_node, NULL, BAD_CAST "var", nullptr);

            xmlNewProp(var_node, BAD_CAST "name", BAD_CAST var->name);

            wddx_to_xml_node(var_node, var->value);
        }

        break;
    default:
        break;
    }
}

const char *wddx_to_string(WDDX *src)
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

void wddx_cleanup(WDDX **value)
{
    if (value)
    {
        WDDX_int *value_int = (WDDX_int *)*value;

        // TODO free header and data members.

        if(value_int->str)
        {
            xmlBufferFree(value_int->str);
            value_int->str = nullptr;
        }
    }
}
