#pragma once

#include <stdint.h>

#ifndef LAY_EXPORT
#define LAY_EXPORT extern
#endif

// Users of this library can define LAY_ASSERT if they would like to use an
// assert other than the one from assert.h.
#ifndef LAY_ASSERT
#include <assert.h>
#define LAY_ASSERT assert
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

typedef uint32_t lay_id;
#if LAY_FLOAT == 1
typedef float lay_scalar;
#else
typedef int16_t lay_scalar;
#endif

#define LAY_INVALID_ID UINT32_MAX

// GCC and Clang allow us to create vectors based on a type with the
// vector_size extension. This will allow us to access individual components of
// the vector via indexing operations.
#if defined(__GNUC__) || defined(__clang__)

// Using floats for coordinates takes up more space than using int16. 128 bits.
// If you want to use __m128 for SSE, this is where you'd typedef it. If you do
// that, then make sure you take care of allocating aligned buffers via
// LAY_REALLOC if your platform's realloc doesn't meet the alignment
// requirements.
#ifdef LAY_FLOAT
typedef float lay_vec4 __attribute__ ((__vector_size__ (16), aligned(16)));
typedef float lay_vec2 __attribute__ ((__vector_size__ (8), aligned(8)));
// Integer version uses 64 bits.
#else
typedef int16_t lay_vec4 __attribute__ ((__vector_size__ (8), aligned(8)));
typedef int16_t lay_vec2 __attribute__ ((__vector_size__ (4), aligned(4)));
#endif // LAY_FLOAT

// MSVC doesn't have the vetor_size attribute, but we want convenient indexing
// operators for our layout logic code. Therefore, we force C++ compilation in
// MSVC, and use C++ operator overloading.
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
#endif // __GNUC__/__clang__ or _MSC_VER

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

// Container flags to pass to lay_set_container()
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

// child layout flags to pass to lay_set_behave()
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
    // When in a wrapping container, put this element on a new line. Wrapping
    // layout code auto-inserts LAY_BREAK flags as needed. See GitHub issues for
    // TODO related to this.
    //
    // Drawing routines can read this via item pointers as needed after
    // performing layout calculations.
    LAY_BREAK = 0x200
} lay_layout_flags;

enum {
    // these bits, starting at bit 16, can be safely assigned by the
    // application, e.g. as item types, other event types, drop targets, etc.
    // this is not yet exposed via API functions, you'll need to get/set these
    // by directly accessing item pointers.
    //
    // (In reality we have more free bits than this, TODO)
    //
    // TODO fix int/unsigned size mismatch (clang issues warning for this),
    // should be all bits as 1 instead of INT_MAX
    LAY_USERMASK = 0x7fff0000,

    // a special mask passed to lay_find_item() (currently does not exist, was
    // not ported from oui)
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

// Call this on a context before using it. You must also call this on a context
// if you would like to use it again after calling lay_destroy_context() on it.
LAY_EXPORT void lay_init_context(lay_context *ctx);

// Reserve enough heap memory to contain `count` items without needing to
// reallocate. The initial lay_init_context() call does not allocate any heap
// memory, so if you init a context and then call this once with a large enough
// number for the number of items you'll create, there will not be any further
// reallocations.
LAY_EXPORT void lay_reserve_items_capacity(lay_context *ctx, lay_id count);

// Frees any heap allocated memory used by a context. Don't call this on a
// context that did not have lay_init_context() call on it. To reuse a context
// after destroying it, you will need to call lay_init_context() on it again.
LAY_EXPORT void lay_destroy_context(lay_context *ctx);

// Clears all of the items in a context, setting its count to 0. Use this when
// you want to re-declare your layout starting from the root item. This does not
// free any memory or perform allocations. It's safe to use the context again
// after calling this. You should probably use this instead of init/destroy if
// you are recalculating your layouts in a loop.
LAY_EXPORT void lay_reset_context(lay_context *ctx);

// Performs the layout calculations, starting at the root item (id 0). After
// calling this, you can use lay_get_rect() to query for an item's calculated
// rectangle. If you use procedures such as lay_append() or lay_insert() after
// calling this, your calculated data may become invalid if a reallocation
// occurs.
//
// You should prefer to recreate your items starting from the root instead of
// doing fine-grained updates to the existing context.
//
// However, it's safe to use lay_set_size on an item, and then re-run
// lay_run_context. This might be useful if you are doing a resizing animation
// on items in a layout without any contents changing.
LAY_EXPORT void lay_run_context(lay_context *ctx);

// Returns the number of items that have been created in a context.
LAY_EXPORT lay_id lay_items_count(lay_context *ctx);

// Returns the number of items the context can hold without performing a
// reallocation.
LAY_EXPORT lay_id lay_items_capacity(lay_context *ctx);

// Create a new item, which can just be thought of as a rectangle. Returns the
// id (handle) used to identify the item.
LAY_EXPORT lay_id lay_item(lay_context *ctx);

// Inserts an item into another item, forming a parent - child relationship. An
// item can contain any number of child items. Items inserted into a parent are
// put at the end of the ordering, after any existing siblings.
LAY_EXPORT void lay_insert(lay_context *ctx, lay_id parent, lay_id child);

// lay_append inserts an item as a sibling after another item. This allows
// inserting an item into the middle of an existing list of items within a
// parent. It's also more efficient than repeatedly using lay_insert(ctx,
// parent, new_child) in a loop to create a list of items in a parent, because
// it does not need to traverse the parent's children each time. So if you're
// creating a long list of children inside of a parent, you might prefer to use
// this after using lay_insert to insert the first child.
LAY_EXPORT void lay_append(lay_context *ctx, lay_id earlier, lay_id later);

// Like lay_insert, but puts the new item as the first child in a parent instead
// of as the last.
LAY_EXPORT void lay_push(lay_context *ctx, lay_id parent, lay_id child);

// Gets the size that was set with lay_set_size or lay_set_size_xy.
LAY_EXPORT lay_vec2 lay_get_size(lay_context *ctx, lay_id item);

// Sets the size of an item. The _xy version passes the width and height as
// separate arguments, but functions the same.
LAY_EXPORT void lay_set_size(lay_context *ctx, lay_id item, lay_vec2 size);
LAY_EXPORT void lay_set_size_xy(lay_context *ctx, lay_id item, lay_scalar width, lay_scalar height);

// Set the flags on an item which determines how it behaves as a parent. For
// example, setting LAY_COLUMN will make an item behave as if it were a column
// -- it will lay out its children vertically.
LAY_EXPORT void lay_set_contain(lay_context *ctx, lay_id item, uint32_t flags);

// Set the flags on an item which determines how it behaves as a child inside of
// a parent item. For example, setting LAY_VFILL will make an item try to fill
// up all available vertical space inside of its parent.
LAY_EXPORT void lay_set_behave(lay_context *ctx, lay_id item, uint32_t flags);

// Get the margins that were set by lay_set_margins.
LAY_EXPORT lay_vec4 lay_get_margins(lay_context *ctx, lay_id item);

// Set the margins on an item. The components of the vector are:
// 0: left, 1: top, 2: right, 3: bottom.
LAY_EXPORT void lay_set_margins(lay_context *ctx, lay_id item, lay_vec4 ltrb);

// Same as lay_set_margins, but the components are passed as separate arguments
// (left, top, right, bottom).
LAY_EXPORT void lay_set_margins_ltrb(lay_context *ctx, lay_id item, lay_scalar l, lay_scalar t, lay_scalar r, lay_scalar b);

// Get the pointer to an item in the buffer by its id. Don't keep this around --
// it will become invalid as soon as any reallocation occurs. Just store the id
// instead (it's smaller, anyway, and the lookup cost will be nothing.)
LAY_STATIC_INLINE lay_item_t *lay_get_item(const lay_context *ctx, lay_id id)
{
    LAY_ASSERT(id != LAY_INVALID_ID && id < ctx->count);
    return ctx->items + id;
}

// Get the id of first child of an item, if any. Returns LAY_INVALID_ID if there
// is no child.
LAY_STATIC_INLINE lay_id lay_first_child(const lay_context *ctx, lay_id id)
{
    const lay_item_t *pitem = lay_get_item(ctx, id);
    return pitem->first_child;
}

// Get the id of the next sibling of an item, if any. Returns LAY_INVALID_ID if
// there is no next sibling.
LAY_STATIC_INLINE lay_id lay_next_sibling(const lay_context *ctx, lay_id id)
{
    const lay_item_t *pitem = lay_get_item(ctx, id);
    return pitem->next_sibling;
}

// Returns the calculated rectangle of an item. This is only valid after calling
// lay_run_context and before any other reallocation occurs. Otherwise, the
// result will be undefined. The vector components are:
// 0: x starting position, 1: y starting position
// 2: width, 3: height
LAY_STATIC_INLINE lay_vec4 lay_get_rect(const lay_context *ctx, lay_id id)
{
    LAY_ASSERT(id != LAY_INVALID_ID && id < ctx->count);
    return ctx->rects[id];
}

#undef LAY_EXPORT
#undef LAY_STATIC_INLINE
