#include "layout.c"
#ifdef __cplusplus
extern "C" {
#endif
#include "lauxlib.h"
#ifdef __cplusplus
}
#endif

#if LAY_FLOAT == 1
#define LUALAY_CHECK_SCALAR luaL_checknumber
#define LUALAY_PUSH_SCALAR lua_pushnumber
#else
#define LUALAY_CHECK_SCALAR luaL_checkinteger
#define LUALAY_PUSH_SCALAR lua_pushinteger
#endif

int lualay_context_new(lua_State* L)
{
    lua_settop(L, 0);
    lay_context *ctx = (lay_context*)lua_newuserdata(L, sizeof(lay_context));
    lay_init_context(ctx);

    luaL_getmetatable(L, "Layout");
    lua_setmetatable(L, -2);
    return 1;
}

lay_context *lualay_context_check(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "Layout");
    luaL_argcheck(L, ud != NULL, 1, "`Layout` expected");
    return (lay_context*)ud;
}

lay_id lualay_id_check(lua_State *L, lay_context *ctx, int pos)
{
    int n = (int)luaL_checkinteger(L, pos);
    luaL_argcheck(L, n >= 0, pos, "Item id must be non-negative");
    luaL_argcheck(L, (lay_id)n <= ctx->count, pos, "Item id is not in valid range for layout context");
    return (lay_id)n;
}

lay_id lualay_id_check_notinserted(lua_State* L, lay_context *ctx, int pos)
{
    lay_id id = lualay_id_check(L, ctx, pos);
    const lay_item_t* pitem = lay_get_item(ctx, id);
    uint32_t inserted = pitem->flags & LAY_ITEM_INSERTED;
    luaL_argcheck(L, !inserted, pos, "Item has already been inserted");
    return id;
}

int lualay_context_gc(lua_State* L)
{
    lay_context* ctx = lualay_context_check(L);
    lay_destroy_context(ctx);
    return 0;
}

int lualay_context_capacity(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lua_settop(L, 0);
    lay_id cap = lay_items_capacity(ctx);
    if (cap > (lay_id)INT_MAX)
        lua_pushinteger(L, INT_MAX);
    else
        lua_pushinteger(L, (int)cap);
    return 1;
}

int lualay_reserve_items_capacity(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    int n = (int)luaL_checkinteger(L, 2);
    luaL_argcheck(L, n >= 0, 2, "Zero or positive integer capacity expected");
    lay_reserve_items_capacity(ctx, (lay_id)n);
    return 0;
}

int lualay_item_new(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lua_pushinteger(L, lay_item(ctx));
    return 1;
}

int lualay_run_context(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_run_context(ctx);
    return 0;
}

int lualay_reset_context(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_reset_context(ctx);
    return 0;
}

int lualay_item_insert(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id parent = lualay_id_check(L, ctx, 2);
    lay_id child = lualay_id_check_notinserted(L, ctx, 3);
    lay_insert(ctx, parent, child);
    return 0;
}

int lualay_item_append(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id earlier = lualay_id_check(L, ctx, 2);
    lay_id later = lualay_id_check_notinserted(L, ctx, 3);
    lay_append(ctx, earlier, later);
    return 0;
}

int lualay_item_push(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id parent = lualay_id_check(L, ctx, 2);
    lay_id child = lualay_id_check_notinserted(L, ctx, 3);
    lay_push(ctx, parent, child);
    return 0;
}

int lualay_item_size_set(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    lay_scalar w = (lay_scalar)LUALAY_CHECK_SCALAR(L, 3);
    lay_scalar h = (lay_scalar)LUALAY_CHECK_SCALAR(L, 4);
    lay_set_size_xy(ctx, item, w, h);
    return 0;
}

int lualay_item_margins_set(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    lay_scalar l = (lay_scalar)LUALAY_CHECK_SCALAR(L, 3);
    lay_scalar t = (lay_scalar)LUALAY_CHECK_SCALAR(L, 4);
    lay_scalar r = (lay_scalar)LUALAY_CHECK_SCALAR(L, 5);
    lay_scalar b = (lay_scalar)LUALAY_CHECK_SCALAR(L, 6);
    lay_set_margins_ltrb(ctx, item, l, t, r, b);
    return 0;
}

int lualay_get_rect(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    lay_vec4 rect = lay_get_rect(ctx, item);
    LUALAY_PUSH_SCALAR(L, rect[0]);
    LUALAY_PUSH_SCALAR(L, rect[1]);
    LUALAY_PUSH_SCALAR(L, rect[2]);
    LUALAY_PUSH_SCALAR(L, rect[3]);
    return 4;
}

// TODO make varargs

int lualay_item_contain_set(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    uint32_t flags = 0;
    const int n = lua_gettop(L);
    for (int i = 3; i <= n; ++i) {
        flags |= luaL_checkinteger(L, i);
        luaL_argcheck(L, (flags & LAY_ITEM_BOX_MASK) == flags, i, "Invalid container flag");
    }
    lay_set_contain(ctx, item, flags);
    return 0;
}
int lualay_item_behave_set(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    uint32_t flags = 0;
    const int n = lua_gettop(L);
    for (int i = 3; i <= n; ++i) {
        flags |= luaL_checkinteger(L, i);
        luaL_argcheck(L, (flags & LAY_ITEM_LAYOUT_MASK) == flags, i, "Invalid behavior flag");
    }
    lay_set_behave(ctx, item, flags);
    return 0;
}

int lualay_item_next_sibling(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    lay_id id_next = lay_next_sibling(ctx, item);
    lua_settop(L, 0);
    // lay_id is uint32_t by default, and LAY_INVALID_ID is uint32_t max, so we
    // need to check if it's out of range
    if (id_next >= (lay_id)INT_MAX)
        lua_pushinteger(L, -1);
    else
        lua_pushinteger(L, (lay_id)id_next);
    return 1;
}

int lualay_item_first_child(lua_State* L)
{
    lay_context *ctx = lualay_context_check(L);
    lay_id item = lualay_id_check(L, ctx, 2);
    lay_id id_child = lay_first_child(ctx, item);
    lua_settop(L, 0);
    if (id_child >= (lay_id)INT_MAX)
        lua_pushinteger(L, -1);
    else
        lua_pushinteger(L, (lay_id)id_child);
    return 1;
}

static const struct luaL_reg laylib[] = {
    {"new", lualay_context_new},
    {"run", lualay_run_context},
    {"reset", lualay_reset_context},
    {"capacity", lualay_context_capacity},
    {"reserve", lualay_reserve_items_capacity},
    {"item", lualay_item_new},
    {"insert", lualay_item_insert},
    {"append", lualay_item_append},
    {"push", lualay_item_push},
    {"set_size", lualay_item_size_set},
    {"set_margins", lualay_item_margins_set},
    {"set_contain", lualay_item_contain_set},
    {"set_behave", lualay_item_behave_set},
    {"rect", lualay_get_rect},
    {"next_sibling", lualay_item_next_sibling},
    {"first_child", lualay_item_first_child},
    {"__gc", lualay_context_gc},
    {NULL, NULL}
};

#ifdef __cplusplus
extern "C" __declspec(dllexport) int luaopen_layout(lua_State* L)
#else
__declspec(dllexport) int luaopen_layout(lua_State* L)
#endif
{
    // TODO update existing if hot reloading
    luaL_newmetatable(L, "Layout");
    lua_pushvalue(L, -1);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    luaL_openlib(L, NULL, laylib, 0);

    lua_setglobal(L, "Layout");

    // Set integer flag constants

#define LUALAY_VAL(flagsym, fieldname) \
    lua_pushinteger(L, flagsym); \
    lua_setfield(L, -2, "" #fieldname "");

    LUALAY_VAL(LAY_ROW, ROW);
    LUALAY_VAL(LAY_COLUMN, COLUMN);
    LUALAY_VAL(LAY_LAYOUT, OVERLAY);
    LUALAY_VAL(LAY_FLEX, FLEX);
    LUALAY_VAL(LAY_NOWRAP, NOWRAP);
    LUALAY_VAL(LAY_WRAP, WRAP);
    LUALAY_VAL(LAY_START, START);
    LUALAY_VAL(LAY_MIDDLE, MIDDLE);
    LUALAY_VAL(LAY_END, END);
    LUALAY_VAL(LAY_JUSTIFY, JUSTIFY);

    LUALAY_VAL(LAY_LEFT, LEFT);
    LUALAY_VAL(LAY_TOP, TOP);
    LUALAY_VAL(LAY_RIGHT, RIGHT);
    LUALAY_VAL(LAY_BOTTOM, BOTTOM);
    LUALAY_VAL(LAY_HFILL, HFILL);
    LUALAY_VAL(LAY_VFILL, VFILL);
    LUALAY_VAL(LAY_HCENTER, HCENTER);
    LUALAY_VAL(LAY_VCENTER, VCENTER);
    LUALAY_VAL(LAY_CENTER, CENTER);
    LUALAY_VAL(LAY_FILL, FILL);
    LUALAY_VAL(LAY_BREAK, BREAK);

#undef LUALAY_VAL

    // TODO flags for preventing being squished (HFIXED/VFIXED)

    return 0;
}
