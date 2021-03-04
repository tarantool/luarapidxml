#include <cstring>
#include <stdexcept>

#define RAPIDXML_STATIC_POOL_SIZE (32*1024)
#define RAPIDXML_DYNAMIC_POOL_SIZE (32*1024)
#include <rapidxml.hpp>

#include <lua.hpp>

extern "C" {
    int decode(lua_State *L);
    int encode(lua_State *L);
    LUA_API int luaopen_luarapidxml( lua_State *L );
}

#define NAME_KEY    "tag"
#define ATTR_KEY    "attr"

#define MAX_MSG_LEN 256
#define MARK_ERROR(x,note,what) memset(x, 0, MAX_MSG_LEN); snprintf( x,MAX_MSG_LEN,"%s: %s",note,what )

static char msg[MAX_MSG_LEN];

static int decode_string(lua_State *L, const char* str, size_t len, char* msg)
{
    int parts = 1;
    const char* end = str+len;

    for (const char* pos = str; pos <= end; pos++) {
        if (*pos != '&') continue;
        lua_pushlstring(L, str, pos-str);
        uint32_t codepoint = 0;
        int n = 0;
        if (false) ;
        else if (!strncmp(pos+1, "lt;",   3)) { lua_pushlstring(L, "<", 1);  pos += 3; }
        else if (!strncmp(pos+1, "gt;",   3)) { lua_pushlstring(L, ">", 1);  pos += 3; }
        else if (!strncmp(pos+1, "amp;",  4)) { lua_pushlstring(L, "&", 1);  pos += 4; }
        else if (!strncmp(pos+1, "apos;", 5)) { lua_pushlstring(L, "'", 1);  pos += 5; }
        else if (!strncmp(pos+1, "quot;", 5)) { lua_pushlstring(L, "\"", 1); pos += 5; }
        else if ( (sscanf(pos+1, "#%u;%n", &codepoint, &n) == 1) ||
                  (sscanf(pos+1, "#x%x;%n", &codepoint, &n) == 1) ) {
            if (codepoint <= 0x7f) {
                lua_pushfstring(L, "%c",
                    (char) codepoint & 0x7f
                );
            } else if (codepoint <= 0x7ff) {
                lua_pushfstring(L, "%c%c",
                    (char) (0xc0 | (codepoint >> 6)),
                    (char) (0x80 | (codepoint & 0x3f))
                );
            } else if (codepoint <= 0xffff) {
                lua_pushfstring(L, "%c%c%c",
                    (char) (0xe0 | (codepoint >> 12)),
                    (char) (0x80 | ((codepoint >> 6) & 0x3f)),
                    (char) (0x80 | (codepoint & 0x3f))
                );
            } else if (codepoint <= 0x1fffff) {
                lua_pushfstring(L, "%c%c%c%c",
                    (char) (0xf0 | (codepoint >> 18)),
                    (char) (0x80 | ((codepoint >> 12) & 0x3f)),
                    (char) (0x80 | ((codepoint >> 6) & 0x3f)),
                    (char) (0x80 | (codepoint & 0x3f))
                );
            } else {
                MARK_ERROR(msg, "xml decode", "invalid unicode codepoint");
                return -1;
            }
            pos += n;
        }
        else {
            MARK_ERROR(msg, "xml decode", "invalid escape sequence");
            return -1;
        }
        str = pos+1;
        parts+=2;
    }

    lua_pushlstring(L, str, end-str);
    lua_concat(L, parts);
    return 0;
}

static int decode_element(lua_State *L, rapidxml::xml_node<> *node, char* msg)
{
    if (!node || rapidxml::node_element != node->type())
    {
        MARK_ERROR(msg, "decode element", "not a xml element");
        return -1;
    }

    if ( !lua_checkstack( L,5 ) )
    {
        MARK_ERROR( msg,"decode element","xml decode out of stack" );
        return -1;
    }

    size_t narr = 0;
    for (rapidxml::xml_node<> *sub = node->first_node(); sub; sub = sub->next_sibling())
        ++ narr;
    lua_createtable(L, narr, 2 /* NAME_KEY, ATTR_KEY */);

    /* element name */
    lua_pushstring( L,NAME_KEY );
    lua_pushlstring( L,node->name(),node->name_size() );
    lua_rawset( L,-3 );

    /* element value */
    /* <oppn id="1" rk_min="2896" rk_max="2910"/> has no value */
    int index = 1;
    for (rapidxml::xml_node<> *sub = node->first_node(); sub; sub = sub->next_sibling())
    {
        if (sub->type() == rapidxml::node_element) {
            int ret = decode_element(L, sub, msg);
            if (ret < 0)
                return -1;
        } else if (sub->type()==rapidxml::node_data || sub->type()==rapidxml::node_cdata) {
            int ret = decode_string(L, sub->value(), sub->value_size(), msg);
            if (ret < 0)
                return -1;
        } else {
            MARK_ERROR(msg, "xml decode", "unsupported xml type");
            return -1;
        }
        lua_rawseti(L, -2, index);
        ++index;
    }

    /* attribute */
    rapidxml::xml_attribute<> *attr = node->first_attribute();
    if ( attr )
    {
        lua_pushstring(L, ATTR_KEY);
        lua_newtable(L);
        for ( ; attr; attr = attr->next_attribute() )
        {
            lua_pushlstring(L, attr->name(), attr->name_size());
            int ret = decode_string(L, attr->value(), attr->value_size(), msg);
            if (ret < 0)
                return -1;

            lua_rawset( L,-3 );
        }
        lua_rawset( L,-3 );
    }

    return 0;
}

int decode( lua_State *L )
{
    const char *str = luaL_checkstring( L,1 );

    int ret = 0;
    {
        rapidxml::xml_document<> doc;
        try
        {
            /* never modify str */
            doc.parse<rapidxml::parse_non_destructive>(const_cast<char*>(str));
            ret = decode_element(L, doc.first_node(), msg);
        }
        catch ( const std::runtime_error& e )
        {
            MARK_ERROR(msg, "xml decode fail", e.what());
            ret = -1;
        }
        catch (const rapidxml::parse_error& e)
        {
            MARK_ERROR(msg, "invalid xml string", e.what());
            ret = -1;
        }
        catch (const std::exception& e)
        {
            MARK_ERROR(msg, "xml decode fail", e.what());
            ret = -1;
        }
        catch (...)
        {
            MARK_ERROR(msg, "xml decode fail", "unknow error");
            ret = -1;
        }

        /* rapidxml static memory pool will never free,until you call clear */
        doc.clear();
    }

    if (ret < 0)
    {
        lua_pushnil(L),
        lua_pushstring(L, msg);
        return 2;
    }

    return 1;
}

struct buf {
    char *str;
    size_t len;
    size_t reserved;
};

int
buf_init(struct buf *buf)
{
    buf->reserved = 1024;
    buf->len = 0;
    buf->str = (char*)malloc(buf->reserved);
    if (buf->str == NULL) {
        return -1;
    }
    return 0;
}

int
buf_write(struct buf *buf, const char *str, size_t len)
{
    size_t reserved = buf->reserved;
    while (true) {
        if ((buf->len + len) >= reserved)
            reserved *= 2;
        else
            break;
    }

    if (buf->reserved != reserved) {
        buf->reserved = reserved;
        buf->str = (char*)realloc(buf->str, buf->reserved);
        if (buf->str == NULL)
            return -1;
    }

    memcpy(buf->str + buf->len, str, len);
    buf->len += len;
    return 0;
}

int
buf_put(struct buf *buf, char c)
{
    return buf_write(buf, &c, 1);
}

void
buf_destroy(struct buf *buf)
{
    free(buf->str);
}

static void encode_string(struct lua_State *L, struct buf *buf, int idx)
{
    size_t len = 0;
    const char* str = lua_tolstring(L, idx, &len);
    const char* end = str+len;

    for (const char* pos = str; pos <= end; pos++) {
        switch (*pos) {
        case '<':  buf_write(buf, str, pos-str); buf_write(buf, "&lt;", 4); str = pos+1; break;
        case '>':  buf_write(buf, str, pos-str); buf_write(buf, "&gt;", 4); str = pos+1; break;
        case '&':  buf_write(buf, str, pos-str); buf_write(buf, "&amp;", 5); str = pos+1; break;
        case '"':  buf_write(buf, str, pos-str); buf_write(buf, "&quot;", 6); str = pos+1; break;
        case '\'': buf_write(buf, str, pos-str); buf_write(buf, "&apos;", 6); str = pos+1; break;
        }
    }
    buf_write(buf, str, end-str);
}

static int encode_element(
    struct lua_State *L,
    struct buf *buf,
    int idx,
    char *msg)
{
    // soap lom object may be either a nil (it is omitted)
    // or a string, or a number (it is converted to string by lua_tolstring())
    // or a table:
    // soap_lom_object = {
    //         ["tag"] = "abc",
    //         ["attr"] = {
    //                 ["a1"] = "A1",
    //                 ["a2"] = "A2",
    //         },
    //         [1] = "inside tag abc",
    // },
    // which is transformed to the string
    // [[<abc a1="A1" a2="A2">inside tag abc</abc>]]

    int top = lua_gettop(L);

    lua_getfield(L, idx, NAME_KEY);
    // -1: lom.tag

    if (lua_type(L, -1) != LUA_TSTRING) {
        MARK_ERROR(msg, "encode element",
            "Invalid table format (`" NAME_KEY "' field must be a string)");
        return -1;
    }
    size_t tag_len;
    const char *tag = lua_tolstring(L, -1, &tag_len);
    buf_put(buf, '<');
    buf_write(buf, tag, tag_len);

    lua_getfield(L, idx, ATTR_KEY);
    // -1: lom.attr
    // -2: tag

    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        /* DO NOTHING */
        break;
    case LUA_TTABLE:
        for (lua_pushnil(L); lua_next(L,-2)!=0; lua_pop(L, 1)) {
            // -1: value
            // -2: key
            // -3: lom.attr
            // -4: tag

            if ( lua_type(L, -1) != LUA_TSTRING || lua_type(L, -2) != LUA_TSTRING )
            {
                MARK_ERROR(msg, "encode element",
                    "Invalid table format (`attr' table must have string keys and values)");
                return -1;
            }

            size_t key_len;
            const char* key = lua_tolstring(L, -2, &key_len);

            buf_put(buf, ' ');
            buf_write(buf, key, key_len);
            buf_write(buf, "=\"", 2);
            encode_string(L, buf, lua_gettop(L));
            buf_put(buf, '"');
        }
        break;
    default:
        MARK_ERROR(msg, "encode element",
            "Invalid table format (`attr' field must be a table)");
        return -1;
    }

    if (lua_objlen(L, idx) == 0) {
        buf_write(buf, "/>", 2);
        lua_settop(L, top);
        return 0;
    } else {
        buf_put(buf, '>');
    }

    for (int i=1; i<=lua_objlen(L, idx); i++) {
        lua_rawgeti(L, idx, i);
        // -1: lom[i]
        // -2: lom.attr
        // -3: tag

        switch (lua_type(L, -1)) {
        case LUA_TSTRING:
        {
            encode_string(L, buf, lua_gettop(L));
            break;
        }
        case LUA_TNUMBER:
        {
            size_t buf_len;
            const char* str_buf = lua_tolstring(L, -1, &buf_len);
            buf_write(buf, str_buf, buf_len);
            break;
        }
        case LUA_TTABLE: {
            int ret = encode_element(L, buf, lua_gettop(L), msg);
            if (ret < 0)
                return -1;
            else
                break;
        }
        default:
            MARK_ERROR(msg, "encode element",
                "Invalid table format (unknown content type)");
            return -1;
        }

        lua_pop(L, 1);
        // -1: lom.attr
        // -2: tag
    }

    buf_put(buf, '<');
    buf_put(buf, '/');
    buf_write(buf, tag, tag_len);
    buf_put(buf, '>');
    lua_settop(L, top);
    return 0;
}

int encode(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    struct buf buf = {NULL, 0, 0};
    if (buf_init(&buf) < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "Memory allocation error");
        return 2;
    }

    int rc = 1;
    int ret = encode_element(L, &buf, 1, msg);
    if (ret < 0) {
        lua_pushnil(L);
        lua_pushstring(L, msg);
        rc = 2;
        goto finalize;
   }

    lua_pushlstring(L, buf.str, buf.len);

finalize:
    buf_destroy(&buf);
    return rc;
}

/* ====================LIBRARY INITIALISATION FUNCTION======================= */

int luaopen_luarapidxml(lua_State *L)
{
    static const struct luaL_Reg lib [] = {
        {"encode", encode},
        {"decode", decode},
        {NULL, NULL}
    };
    luaL_newlib(L, lib);
    return 1;
}
