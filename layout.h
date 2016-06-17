#pragma once

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#ifndef LAY_EXPORT
#define LAY_EXPORT extern
#endif

// 'static inline' for things we always want inlined -- the compiler should not
// even have to consider not inlining these.
#if defined(__GNUC__) || defined(__clang__)
#define LAY_STATIC_INLINE __attribute__((always_inline)) static inline
#elif defined(_MSC_VER)
#define LAY_STATIC_INLINE __forceinline static
#else
#define LAY_STATIC_INLINE inline static
#endif

#define LAY_ASSERT assert

typedef uint32_t lay_id;
#if LAY_FLOAT == 1
typedef float lay_scalar;
#else
typedef int16_t lay_scalar;
#endif

#if defined(__GNUC__) || defined(__clang__)
#if LAY_FLOAT == 1
typedef float lay_vec4 __attribute__ ((__vector_size__ (16), aligned(16)));
typedef float lay_vec2 __attribute__ ((__vector_size__ (8), aligned(8)));
#else
typedef int16_t lay_vec4 __attribute__ ((__vector_size__ (8), aligned(8)));
typedef int16_t lay_vec2 __attribute__ ((__vector_size__ (4), aligned(4)));
#endif // LAY_FLOAT
#elif defined(_MSC_VER)
struct lay_vec4 {
    lay_scalar xyzw[4];
    const lay_scalar& operator[](int index) const
    { return xyzw[index]; }
    lay_scalar& operator[](int index)
    { return xyzw[index]; }
};
struct lay_vec2 {
    lay_scalar xy[2];
    const lay_scalar& operator[](int index) const
    { return xy[index]; }
    lay_scalar& operator[](int index)
    { return xy[index]; }
};
#endif // __GNUC__ or _MSC_VER

#ifndef LAY_INVALID_ID
#define LAY_INVALID_ID UINT32_MAX
#endif

typedef struct lay_item_t {
    uint32_t flags;
    lay_id first_child;
    lay_id next_sibling;
    lay_vec4 margins;
    lay_vec2 size;
#if defined(__GNUC__) || defined(__clang__)
} __attribute__ ((aligned (16))) lay_item_t;
#else
} lay_item_t;
#endif

typedef struct lay_context {
    lay_item_t *items;
    lay_vec4 *rects;
    lay_id capacity;
    lay_id count;
} lay_context;

// container flags to pass to uiSetBox()
typedef enum lay_box_flags {
    // flex-direction (bit 0+1)

    // left to right
    LAY_ROW = 0x002,
    // top to bottom
    LAY_COLUMN = 0x003,

    // model (bit 1)

    // free layout
    LAY_LAYOUT = 0x000,
    // flex model
    LAY_FLEX = 0x002,

    // flex-wrap (bit 2)

    // single-line
    LAY_NOWRAP = 0x000,
    // multi-line, wrap left to right
    LAY_WRAP = 0x004,


    // justify-content (start, end, center, space-between)
    // at start of row/column
    LAY_START = 0x008,
    // at center of row/column
    LAY_MIDDLE = 0x000,
    // at end of row/column
    LAY_END = 0x010,
    // insert spacing to stretch across whole row/column
    LAY_JUSTIFY = 0x018,

    // align-items
    // can be implemented by putting a flex container in a layout container,
    // then using LAY_TOP, LAY_BOTTOM, LAY_VFILL, LAY_VCENTER, etc.
    // FILL is equivalent to stretch/grow

    // align-content (start, end, center, stretch)
    // can be implemented by putting a flex container in a layout container,
    // then using LAY_TOP, LAY_BOTTOM, LAY_VFILL, LAY_VCENTER, etc.
    // FILL is equivalent to stretch; space-between is not supported.
} lay_box_flags;

// child layout flags to pass to uiSetLayout()
typedef enum lay_layout_flags {
    // attachments (bit 5-8)
    // fully valid when parent uses LAY_LAYOUT model
    // partially valid when in LAY_FLEX model

    // anchor to left item or left side of parent
    LAY_LEFT = 0x020,
    // anchor to top item or top side of parent
    LAY_TOP = 0x040,
    // anchor to right item or right side of parent
    LAY_RIGHT = 0x080,
    // anchor to bottom item or bottom side of parent
    LAY_BOTTOM = 0x100,
    // anchor to both left and right item or parent borders
    LAY_HFILL = 0x0a0,
    // anchor to both top and bottom item or parent borders
    LAY_VFILL = 0x140,
    // center horizontally, with left margin as offset
    LAY_HCENTER = 0x000,
    // center vertically, with top margin as offset
    LAY_VCENTER = 0x000,
    // center in both directions, with left/top margin as offset
    LAY_CENTER = 0x000,
    // anchor to all four directions
    LAY_FILL = 0x1e0,
    // when wrapping, put this element on a new line
    // wrapping layout code auto-inserts LAY_BREAK flags,
    // drawing routines can read them with uiGetLayout()
    LAY_BREAK = 0x200
} lay_layout_flags;

enum {
    // these bits, starting at bit 16, can be safely assigned by the
    // application, e.g. as item types, other event types, drop targets, etc.
    // they can be set and queried using uiSetFlags() and uiGetFlags()
    //
    // (In reality we have more free bits than this, see what gets used in the
    // masks)
    //
    // TODO fix int/unsigned size mismatch (clang issues warning for this),
    // should be all bits as 1 instead of INT_MAX
    LAY_USERMASK = 0x7fff0000,

    // a special mask passed to uiFindItem()
    LAY_ANY = 0x7fffffff,
};

enum {
    // extra item flags

    // bit 0-2
    LAY_ITEM_BOX_MODEL_MASK = 0x000007,
    // bit 0-4
    LAY_ITEM_BOX_MASK       = 0x00001F,
    // bit 5-9
    LAY_ITEM_LAYOUT_MASK = 0x0003E0,
    // item has been inserted (bit 10)
    LAY_ITEM_INSERTED   = 0x400,
    // horizontal size has been explicitly set (bit 11)
    LAY_ITEM_HFIXED      = 0x800,
    // vertical size has been explicitly set (bit 12)
    LAY_ITEM_VFIXED      = 0x1000,
    // bit 11-12
    LAY_ITEM_FIXED_MASK  = LAY_ITEM_HFIXED | LAY_ITEM_VFIXED,

    // which flag bits will be compared
    LAY_ITEM_COMPARE_MASK = LAY_ITEM_BOX_MODEL_MASK
        | (LAY_ITEM_LAYOUT_MASK & ~LAY_BREAK)
        | LAY_USERMASK,
};

LAY_STATIC_INLINE lay_vec4 lay_vec4_xyzw(lay_scalar x, lay_scalar y, lay_scalar z, lay_scalar w)
{
#if defined(__GNUC__) || defined(__clang__)
    return (lay_vec4){x, y, z, w};
#elif defined(_MSC_VER)
    lay_vec4 result;
    result[0] = x;
    result[1] = y;
    result[2] = z;
    result[3] = w;
    return result;
#endif
}

LAY_EXPORT void lay_init_context(lay_context *ctx);
LAY_EXPORT void lay_reserve_items_capacity(lay_context *ctx, lay_id count);
LAY_EXPORT void lay_destroy_context(lay_context *ctx);

LAY_EXPORT void lay_reset_context(lay_context *ctx);
LAY_EXPORT void lay_run_context(lay_context *ctx);

LAY_EXPORT lay_id lay_items_count(lay_context *ctx);
LAY_EXPORT lay_id lay_items_capacity(lay_context *ctx);

LAY_EXPORT lay_id lay_item(lay_context *ctx);
LAY_EXPORT void lay_append(lay_context *ctx, lay_id earlier, lay_id later);
LAY_EXPORT void lay_insert(lay_context *ctx, lay_id parent, lay_id child);
LAY_EXPORT void lay_push(lay_context *ctx, lay_id parent, lay_id child);
LAY_EXPORT lay_vec2 lay_get_size(lay_context *ctx, lay_id item);
LAY_EXPORT void lay_set_size_xy(lay_context *ctx, lay_id item, lay_scalar width, lay_scalar height);
LAY_EXPORT void lay_set_contain(lay_context *ctx, lay_id item, uint32_t flags);
LAY_EXPORT void lay_set_behave(lay_context *ctx, lay_id item, uint32_t flags);
LAY_EXPORT lay_vec4 lay_get_margins(lay_context *ctx, lay_id item);
LAY_EXPORT void lay_set_margins(lay_context *ctx, lay_id item, lay_vec4 ltrb);
LAY_EXPORT void lay_set_margins_ltrb(lay_context *ctx, lay_id item, lay_scalar l, lay_scalar t, lay_scalar r, lay_scalar b);

LAY_STATIC_INLINE lay_item_t *lay_get_item(const lay_context *ctx, lay_id id)
{
    LAY_ASSERT(id != LAY_INVALID_ID && id < ctx->count);
    return ctx->items + id;
}

LAY_STATIC_INLINE lay_id lay_first_child(const lay_context *ctx, lay_id id)
{
    const lay_item_t *pitem = lay_get_item(ctx, id);
    return pitem->first_child;
}

LAY_STATIC_INLINE lay_id lay_next_sibling(const lay_context *ctx, lay_id id)
{
    const lay_item_t *pitem = lay_get_item(ctx, id);
    return pitem->next_sibling;
}

LAY_STATIC_INLINE lay_vec4 lay_get_rect(const lay_context *ctx, lay_id id)
{
    LAY_ASSERT(id != LAY_INVALID_ID && id < ctx->count);
    return ctx->rects[id];
}

#undef LAY_EXPORT
#undef LAY_STATIC_INLINE
