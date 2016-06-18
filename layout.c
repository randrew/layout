#include "layout.h"
#include <stdbool.h>
#include <string.h> // memset
#include <stdlib.h> // realloc

#if defined(__GNUC__) || defined(__clang__)
#define LAY_FORCE_INLINE __attribute__((always_inline)) inline
#define LAY_RESTRICT restrict
#elif defined(_MSC_VER)
#define LAY_FORCE_INLINE __forceinline
#define LAY_RESTRICT __restrict
#else
#define LAY_FORCE_INLINE inline
#define LAY_RESTRICT
#endif

// Useful math utilities
static LAY_FORCE_INLINE lay_scalar lay_scalar_max(lay_scalar a, lay_scalar b)
{ return a > b ? a : b; }
static LAY_FORCE_INLINE lay_scalar lay_scalar_min(lay_scalar a, lay_scalar b)
{ return a < b ? a : b; }
static LAY_FORCE_INLINE float lay_float_max(float a, float b)
{ return a > b ? a : b; }
static LAY_FORCE_INLINE float lay_float_min(float a, float b)
{ return a < b ? a : b; }

void lay_init_context(lay_context *ctx)
{
    ctx->capacity = 0;
    ctx->count = 0;
    ctx->items = NULL;
    ctx->rects = NULL;
}

void lay_reserve_items_capacity(lay_context *ctx, lay_id count)
{
    if (count >= ctx->capacity) {
        ctx->capacity = count;
        const size_t item_size = sizeof(lay_item_t) + sizeof(lay_vec4);
        ctx->items = (lay_item_t*)realloc(ctx->items, ctx->capacity * item_size);
        const lay_item_t *past_last = ctx->items + ctx->capacity;
        ctx->rects = (lay_vec4*)past_last;
    }
}

void lay_destroy_context(lay_context *ctx)
{
    if (ctx->items != NULL) {
        free(ctx->items);
        ctx->items = NULL;
        ctx->rects = NULL;
    }
}

void lay_reset_context(lay_context *ctx)
{ ctx->count = 0; }

static void lay_calc_size(lay_context *ctx, lay_id item, int dim);
static void lay_arrange(lay_context *ctx, lay_id item, int dim);

void lay_run_context(lay_context *ctx)
{
    LAY_ASSERT(ctx != NULL);

    // TODO this is bad. We need to reset the 'break' flag, in case the item
    // data is being reused across 'run' invocations. Iterating all of the
    // items up-front like this and mutating data is bad for performance. But
    // if we don't do this, then the leftover LAY_BREAK bits will still be set
    // from the previous 'run', which may cause broken wrapping behavior if the
    // sizes of the container (or the items) have changed.
    //
    // Maybe we should instead have a separate 'reset_breaks' procedure the
    // user can explicitly run when they know they have 'wrap' items which are
    // being resued across frames.
    //
    // Alternatively, we could use a flag bit to indicate whether an item's
    // children have already been wrapped and may need re-wrapping.
    for (lay_id i = 0; i < ctx->count; ++i) {
        lay_item_t *pitem = lay_get_item(ctx, i);
        pitem->flags = pitem->flags & ~LAY_BREAK;
    }

    if (ctx->count > 0) {
        lay_calc_size(ctx, 0, 0);
        lay_arrange(ctx, 0, 0);
        lay_calc_size(ctx, 0, 1);
        lay_arrange(ctx, 0, 1);
    }
}

lay_id lay_items_count(lay_context *ctx)
{
    LAY_ASSERT(ctx != NULL);
    return ctx->count;
}

lay_id lay_items_capacity(lay_context *ctx)
{
    LAY_ASSERT(ctx != NULL);
    return ctx->capacity;
}

lay_id lay_item(lay_context *ctx)
{
    lay_id idx = ctx->count++;

    if (idx >= ctx->capacity) {
        ctx->capacity = ctx->capacity < 1 ? 32 : (ctx->capacity * 4);
        const size_t item_size = sizeof(lay_item_t) + sizeof(lay_vec4);
        ctx->items = (lay_item_t*)realloc(ctx->items, ctx->capacity * item_size);
        const lay_item_t *past_last = ctx->items + ctx->capacity;
        ctx->rects = (lay_vec4*)past_last;
    }

    lay_item_t *item = lay_get_item(ctx, idx);
    // We can either do this here, or when creating/resetting buffer
    memset(item, 0, sizeof(lay_item_t));
    item->first_child = LAY_INVALID_ID;
    item->next_sibling = LAY_INVALID_ID;
    // hmm
    memset(&ctx->rects[idx], 0, sizeof(lay_vec4));
    return idx;
}

static LAY_FORCE_INLINE
void lay_append_by_ptr(
        lay_item_t *LAY_RESTRICT pearlier,
        lay_id later, lay_item_t *LAY_RESTRICT plater)
{
    plater->next_sibling = pearlier->next_sibling;
    plater->flags |= LAY_ITEM_INSERTED;
    pearlier->next_sibling = later;
}

lay_id lay_last_child(const lay_context *ctx, lay_id parent)
{
    lay_item_t *pparent = lay_get_item(ctx, parent);
    lay_id child = pparent->first_child;
    if (child == LAY_INVALID_ID) return LAY_INVALID_ID;
    lay_item_t *pchild = lay_get_item(ctx, child);
    lay_id result = child;
    for (;;) {
        lay_id next = pchild->next_sibling;
        if (next == LAY_INVALID_ID) break;
        result = next;
        pchild = lay_get_item(ctx, next);
    }
    return result;
}

void lay_append(lay_context *ctx, lay_id earlier, lay_id later)
{
    LAY_ASSERT(later != 0); // Must not be root item
    LAY_ASSERT(earlier != later); // Must not be same item id
    lay_item_t *LAY_RESTRICT pearlier = lay_get_item(ctx, earlier);
    lay_item_t *LAY_RESTRICT plater = lay_get_item(ctx, later);
    lay_append_by_ptr(pearlier, later, plater);
}

void lay_insert(lay_context *ctx, lay_id parent, lay_id child)
{
    LAY_ASSERT(child != 0); // Must not be root item
    LAY_ASSERT(parent != child); // Must not be same item id
    lay_item_t *LAY_RESTRICT pparent = lay_get_item(ctx, parent);
    lay_item_t *LAY_RESTRICT pchild = lay_get_item(ctx, child);
    LAY_ASSERT(!(pchild->flags & LAY_ITEM_INSERTED));
    // Parent has no existing children, make inserted item the first child.
    if (pparent->first_child == LAY_INVALID_ID) {
        pparent->first_child = child;
        pchild->flags |= LAY_ITEM_INSERTED;
    // Parent has existing items, iterate to find the last child and append the
    // inserted item after it.
    } else {
        lay_id next = pparent->first_child;
        lay_item_t *LAY_RESTRICT pnext = lay_get_item(ctx, next);
        for (;;) {
            next = pnext->next_sibling;
            if (next == LAY_INVALID_ID) break;
            pnext = lay_get_item(ctx, next);
        }
        lay_append_by_ptr(pnext, child, pchild);
    }
}

void lay_push(lay_context *ctx, lay_id parent, lay_id new_child)
{
    LAY_ASSERT(new_child != 0); // Must not be root item
    LAY_ASSERT(parent != new_child); // Must not be same item id
    lay_item_t *LAY_RESTRICT pparent = lay_get_item(ctx, parent);
    lay_id old_child = pparent->first_child;
    lay_item_t *LAY_RESTRICT pchild = lay_get_item(ctx, new_child);
    LAY_ASSERT(!(pchild->flags & LAY_ITEM_INSERTED));
    pparent->first_child = new_child;
    pchild->flags |= LAY_ITEM_INSERTED;
    pchild->next_sibling = old_child;
}

lay_vec2 lay_get_size(lay_context *ctx, lay_id item)
{
    lay_item_t *pitem = lay_get_item(ctx, item);
    return pitem->size;
}


void lay_set_size(lay_context *ctx, lay_id item, lay_vec2 size)
{
    lay_item_t *pitem = lay_get_item(ctx, item);
    pitem->size = size;
    uint32_t flags = pitem->flags;
    if (size[0] == 0)
        flags &= ~LAY_ITEM_HFIXED;
    else
        flags |= LAY_ITEM_HFIXED;
    if (size[1] == 0)
        flags &= ~LAY_ITEM_VFIXED;
    else
        flags |= LAY_ITEM_VFIXED;
    pitem->flags = flags;
}

void lay_set_size_xy(
        lay_context *ctx, lay_id item,
        lay_scalar width, lay_scalar height)
{
    lay_item_t *pitem = lay_get_item(ctx, item);
    pitem->size[0] = width;
    pitem->size[1] = height;
    // Kinda redundant, whatever
    uint32_t flags = pitem->flags;
    if (width == 0)
        flags &= ~LAY_ITEM_HFIXED;
    else
        flags |= LAY_ITEM_HFIXED;
    if (height == 0)
        flags &= ~LAY_ITEM_VFIXED;
    else
        flags |= LAY_ITEM_VFIXED;
    pitem->flags = flags;
}

void lay_set_behave(lay_context *ctx, lay_id item, uint32_t flags)
{
    LAY_ASSERT((flags & LAY_ITEM_LAYOUT_MASK) == flags);
    lay_item_t *pitem = lay_get_item(ctx, item);
    pitem->flags = (pitem->flags & ~LAY_ITEM_LAYOUT_MASK) | flags;
}

void lay_set_contain(lay_context *ctx, lay_id item, uint32_t flags)
{
    LAY_ASSERT((flags & LAY_ITEM_BOX_MASK) == flags);
    lay_item_t *pitem = lay_get_item(ctx, item);
    pitem->flags = (pitem->flags & ~LAY_ITEM_BOX_MASK) | flags;
}
void lay_set_margins(lay_context *ctx, lay_id item, lay_vec4 ltrb)
{
    lay_item_t *pitem = lay_get_item(ctx, item);
    pitem->margins = ltrb;
}
void lay_set_margins_ltrb(
        lay_context *ctx, lay_id item,
        lay_scalar l, lay_scalar t, lay_scalar r, lay_scalar b)
{
    lay_item_t *pitem = lay_get_item(ctx, item);
    // Alternative, uses stack and addressed writes
    //pitem->margins = lay_vec4_xyzw(l, t, r, b);
    // Alternative, uses rax and left-shift
    //pitem->margins = (lay_vec4){l, t, r, b};
    // Fewest instructions, but uses more addressed writes?
    pitem->margins[0] = l;
    pitem->margins[1] = t;
    pitem->margins[2] = r;
    pitem->margins[3] = b;
}

lay_vec4 lay_get_margins(lay_context *ctx, lay_id item)
{ return lay_get_item(ctx, item)->margins; }

// TODO restrict item ptrs correctly
static LAY_FORCE_INLINE
lay_scalar lay_calc_overlayed_size(
        lay_context *ctx, lay_id item, int dim)
{
    const int wdim = dim + 2;
    lay_item_t *LAY_RESTRICT pitem = lay_get_item(ctx, item);
    lay_scalar need_size = 0;
    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        lay_item_t *pchild = lay_get_item(ctx, child);
        lay_vec4 rect = ctx->rects[child];
        // width = start margin + calculated width + end margin
        lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        need_size = lay_scalar_max(need_size, child_size);
        child = pchild->next_sibling;
    }
    return need_size;
}

static LAY_FORCE_INLINE
lay_scalar lay_calc_stacked_size(
        lay_context *ctx, lay_id item, int dim)
{
    const int wdim = dim + 2;
    lay_item_t *LAY_RESTRICT pitem = lay_get_item(ctx, item);
    lay_scalar need_size = 0;
    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        lay_item_t *pchild = lay_get_item(ctx, child);
        lay_vec4 rect = ctx->rects[child];
        need_size += rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        child = pchild->next_sibling;
    }
    return need_size;
}

static LAY_FORCE_INLINE
lay_scalar lay_calc_wrapped_overlayed_size(
        lay_context *ctx, lay_id item, int dim)
{
    const int wdim = dim + 2;
    lay_item_t *LAY_RESTRICT pitem = lay_get_item(ctx, item);
    lay_scalar need_size = 0;
    lay_scalar need_size2 = 0;
    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        lay_item_t *pchild = lay_get_item(ctx, child);
        lay_vec4 rect = ctx->rects[child];
        if (pchild->flags & LAY_BREAK) {
            need_size2 += need_size;
            need_size = 0;
        }
        lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        need_size = lay_scalar_max(need_size, child_size);
        child = pchild->next_sibling;
    }
    return need_size2 + need_size;
}

// Equivalent to uiComputeWrappedStackedSize
static LAY_FORCE_INLINE
lay_scalar lay_calc_wrapped_stacked_size(
        lay_context *ctx, lay_id item, int dim)
{
    const int wdim = dim + 2;
    lay_item_t *LAY_RESTRICT pitem = lay_get_item(ctx, item);
    lay_scalar need_size = 0;
    lay_scalar need_size2 = 0;
    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        lay_item_t *pchild = lay_get_item(ctx, child);
        lay_vec4 rect = ctx->rects[child];
        if (pchild->flags & LAY_BREAK) {
            need_size2 = lay_scalar_max(need_size2, need_size);
            need_size = 0;
        }
        need_size += rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        child = pchild->next_sibling;
    }
    return lay_scalar_max(need_size2, need_size);
}

static void lay_calc_size(lay_context *ctx, lay_id item, int dim)
{
    lay_item_t *pitem = lay_get_item(ctx, item);

    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        // NOTE: this is recursive and will run out of stack space if items are
        // nested too deeply.
        lay_calc_size(ctx, child, dim);
        lay_item_t *pchild = lay_get_item(ctx, child);
        child = pchild->next_sibling;
    }

    // Set the mutable rect output data to the starting input data
    ctx->rects[item][dim] = pitem->margins[dim];

    // If we have an explicit input size, just set our output size (which other
    // calc_size and arrange procedures will use) to it.
    if (pitem->size[dim] != 0) {
        ctx->rects[item][2 + dim] = pitem->size[dim];
        return;
    }

    // Calculate our size based on children items. Note that we've already
    // called lay_calc_size on our children at this point.
    lay_scalar cal_size;
    switch (pitem->flags & LAY_ITEM_BOX_MODEL_MASK) {
    case LAY_COLUMN|LAY_WRAP:
        // flex model
        if (dim) // direction
            cal_size = lay_calc_stacked_size(ctx, item, 1);
        else
            cal_size = lay_calc_overlayed_size(ctx, item, 0);
        break;
    case LAY_ROW|LAY_WRAP:
        // flex model
        if (!dim) // direction
            cal_size = lay_calc_wrapped_stacked_size(ctx, item, 0);
        else
            cal_size = lay_calc_wrapped_overlayed_size(ctx, item, 1);
        break;
    case LAY_COLUMN:
    case LAY_ROW:
        // flex model
        if ((pitem->flags & 1) == (uint32_t)dim) // direction
            cal_size = lay_calc_stacked_size(ctx, item, dim);
        else
            cal_size = lay_calc_overlayed_size(ctx, item, dim);
        break;
    default:
        // layout model
        cal_size = lay_calc_overlayed_size(ctx, item, dim);
        break;
    }

    // Set our output data size. Will be used by parent calc_size procedures.,
    // and by arrange procedures.
    ctx->rects[item][2 + dim] = cal_size;
}

static LAY_FORCE_INLINE
void lay_arrange_stacked(
            lay_context *ctx, lay_id item, int dim, bool wrap)
{
    const int wdim = dim + 2;
    lay_item_t *pitem = lay_get_item(ctx, item);

    const uint32_t item_flags = pitem->flags;
    lay_vec4 rect = ctx->rects[item];
    lay_scalar space = rect[2 + dim];

    float max_x2 = (float)(rect[dim] + space);

    lay_id start_child = pitem->first_child;
    while (start_child != LAY_INVALID_ID) {
        lay_scalar used = 0;
        uint32_t count = 0; // count of fillers
        uint32_t squeezed_count = 0; // count of squeezable elements
        uint32_t total = 0;
        bool hardbreak = false;
        // first pass: count items that need to be expanded,
        // and the space that is used
        lay_id child = start_child;
        lay_id end_child = LAY_INVALID_ID;
        while (child != LAY_INVALID_ID) {
            lay_item_t *pchild = lay_get_item(ctx, child);
            const uint32_t child_flags = pchild->flags;
            const uint32_t flags = (child_flags & LAY_ITEM_LAYOUT_MASK) >> dim;
            const uint32_t fflags = (child_flags & LAY_ITEM_FIXED_MASK) >> dim;
            const lay_vec4 child_margins = pchild->margins;
            lay_vec4 child_rect = ctx->rects[child];
            lay_scalar extend = used;
            if ((flags & LAY_HFILL) == LAY_HFILL) {
                ++count;
                extend += child_rect[dim] + child_margins[wdim];
            } else {
                if ((fflags & LAY_ITEM_HFIXED) != LAY_ITEM_HFIXED)
                    ++squeezed_count;
                extend += child_rect[dim] + child_rect[2 + dim] + child_margins[wdim];
            }
            // wrap on end of line or manual flag
            if (wrap && (
                    total && ((extend > space) ||
                    (child_flags & LAY_BREAK)))) {
                end_child = child;
                hardbreak = (child_flags & LAY_BREAK) == LAY_BREAK;
                // add marker for subsequent queries
                pchild->flags = child_flags | LAY_BREAK;
                break;
            } else {
                used = extend;
                child = pchild->next_sibling;
            }
            ++total;
        }

        lay_scalar extra_space = space - used;
        float filler = 0.0f;
        float spacer = 0.0f;
        float extra_margin = 0.0f;
        float eater = 0.0f;

        if (extra_space > 0) {
            if (count > 0)
                filler = (float)extra_space / (float)count;
            else if (total > 0) {
                switch (item_flags & LAY_JUSTIFY) {
                case LAY_JUSTIFY:
                    // justify when not wrapping or not in last line,
                    // or not manually breaking
                    if (!wrap || ((end_child != LAY_INVALID_ID) && !hardbreak))
                        spacer = (float)extra_space / (float)(total - 1);
                    break;
                case LAY_START:
                    break;
                case LAY_END:
                    extra_margin = extra_space;
                    break;
                default:
                    extra_margin = extra_space / 2.0f;
                    break;
                }
            }
        }
#ifdef LAY_FLOAT
        // In floating point, it's possible to end up with some small negative
        // value for extra_space, while also have a 0.0 squeezed_count. This
        // would cause divide by zero. Instead, we'll check to see if
        // squeezed_count is > 0. I believe this produces the same results as
        // the original oui int-only code. However, I don't have any tests for
        // it, so I'll leave it if-def'd for now.
        else if (!wrap && (squeezed_count > 0))
#else
        // This is the original oui code
        else if (!wrap && (extra_space < 0))
#endif
            eater = (float)extra_space / (float)squeezed_count;

        // distribute width among items
        float x = (float)rect[dim];
        float x1;
        // second pass: distribute and rescale
        child = start_child;
        while (child != end_child) {
            lay_scalar ix0, ix1;
            lay_item_t *pchild = lay_get_item(ctx, child);
            const uint32_t child_flags = pchild->flags;
            const uint32_t flags = (child_flags & LAY_ITEM_LAYOUT_MASK) >> dim;
            const uint32_t fflags = (child_flags & LAY_ITEM_FIXED_MASK) >> dim;
            const lay_vec4 child_margins = pchild->margins;
            lay_vec4 child_rect = ctx->rects[child];

            x += (float)child_rect[dim] + extra_margin;
            if ((flags & LAY_HFILL) == LAY_HFILL) // grow
                x1 = x + filler;
            else if ((fflags & LAY_ITEM_HFIXED) == LAY_ITEM_HFIXED)
                x1 = x + (float)child_rect[2 + dim];
            else // squeeze
                x1 = x + lay_float_max(0.0f, (float)child_rect[2 + dim] + eater);

            ix0 = (lay_scalar)x;
            if (wrap)
                ix1 = (lay_scalar)lay_float_min(max_x2 - (float)child_margins[wdim], x1);
            else
                ix1 = (lay_scalar)x1;
            child_rect[dim] = ix0; // pos
            child_rect[dim + 2] = ix1 - ix0; // size
            ctx->rects[child] = child_rect;
            x = x1 + (float)child_margins[wdim];
            child = pchild->next_sibling;
            extra_margin = spacer;
        }

        start_child = end_child;
    }
}

static LAY_FORCE_INLINE
void lay_arrange_overlay(lay_context *ctx, lay_id item, int dim)
{
    const int wdim = dim + 2;
    lay_item_t *pitem = lay_get_item(ctx, item);
    const lay_vec4 rect = ctx->rects[item];
    const lay_scalar offset = rect[dim];
    const lay_scalar space = rect[2 + dim];
    
    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        lay_item_t *pchild = lay_get_item(ctx, child);
        const uint32_t b_flags = (pchild->flags & LAY_ITEM_LAYOUT_MASK) >> dim;
        const lay_vec4 child_margins = pchild->margins;
        lay_vec4 child_rect = ctx->rects[child];

        switch (b_flags & LAY_HFILL) {
        case LAY_HCENTER:
            child_rect[dim] += (space - child_rect[2 + dim]) / 2 - child_margins[wdim];
            break;
        case LAY_RIGHT:
            child_rect[dim] += space - child_rect[2 + dim] - child_margins[wdim];
            break;
        case LAY_HFILL:
            child_rect[2 + dim] = lay_scalar_max(0, space - child_rect[dim] - child_margins[wdim]);
            break;
        default:
            break;
        }

        child_rect[dim] += offset;
        ctx->rects[child] = child_rect;
        child = pchild->next_sibling;
    }
}

static LAY_FORCE_INLINE
void lay_arrange_overlay_squeezed_range(
        lay_context *ctx, int dim,
        lay_id start_item, lay_id end_item,
        lay_scalar offset, lay_scalar space)
{
    int wdim = dim + 2;
    lay_id item = start_item;
    while (item != end_item) {
        lay_item_t *pitem = lay_get_item(ctx, item);
        const uint32_t b_flags = (pitem->flags & LAY_ITEM_LAYOUT_MASK) >> dim;
        const lay_vec4 margins = pitem->margins;
        lay_vec4 rect = ctx->rects[item];
        lay_scalar min_size = lay_scalar_max(0, space - rect[dim] - margins[wdim]);
        switch (b_flags & LAY_HFILL) {
            case LAY_HCENTER:
                rect[2 + dim] = lay_scalar_min(rect[2 + dim], min_size);
                rect[dim] += (space - rect[2 + dim]) / 2 - margins[wdim];
                break;
            case LAY_RIGHT:
                rect[2 + dim] = lay_scalar_min(rect[2 + dim], min_size);
                rect[dim] = space - rect[2 + dim] - margins[wdim];
                break;
            case LAY_HFILL:
                rect[2 + dim] = min_size;
                break;
            default:
                rect[2 + dim] = lay_scalar_min(rect[2 + dim], min_size);
                break;
        }
        rect[dim] += offset;
        ctx->rects[item] = rect;
        item = pitem->next_sibling;
    }
}

static LAY_FORCE_INLINE
lay_scalar lay_arrange_wrapped_overlay_squeezed(
        lay_context *ctx, lay_id item, int dim)
{
    const int wdim = dim + 2;
    lay_item_t *pitem = lay_get_item(ctx, item);
    lay_scalar offset = ctx->rects[item][dim];
    lay_scalar need_size = 0;
    lay_id child = pitem->first_child;
    lay_id start_child = child;
    while (child != LAY_INVALID_ID) {
        lay_item_t *pchild = lay_get_item(ctx, child);
        if (pchild->flags & LAY_BREAK) {
            lay_arrange_overlay_squeezed_range(ctx, dim, start_child, child, offset, need_size);
            offset += need_size;
            start_child = child;
            need_size = 0;
        }
        const lay_vec4 rect = ctx->rects[child];
        lay_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        need_size = lay_scalar_max(need_size, child_size);
        child = pchild->next_sibling;
    }
    lay_arrange_overlay_squeezed_range(ctx, dim, start_child, LAY_INVALID_ID, offset, need_size);
    offset += need_size;
    return offset;
}

static void lay_arrange(lay_context *ctx, lay_id item, int dim)
{
    lay_item_t *pitem = lay_get_item(ctx, item);

    const uint32_t flags = pitem->flags;
    switch (flags & LAY_ITEM_BOX_MODEL_MASK) {
    case LAY_COLUMN | LAY_WRAP:
        if (dim != 0) {
            lay_arrange_stacked(ctx, item, 1, true);
            lay_scalar offset = lay_arrange_wrapped_overlay_squeezed(ctx, item, 0);
            ctx->rects[item][2 + 0] = offset - ctx->rects[item][0];
        }
        break;
    case LAY_ROW | LAY_WRAP:
        if (dim == 0)
            lay_arrange_stacked(ctx, item, 0, true);
        else
            // discard return value
            lay_arrange_wrapped_overlay_squeezed(ctx, item, 1);
        break;
    case LAY_COLUMN:
    case LAY_ROW:
        if ((flags & 1) == (uint32_t)dim) {
            lay_arrange_stacked(ctx, item, dim, false);
        } else {
            const lay_vec4 rect = ctx->rects[item];
            lay_arrange_overlay_squeezed_range(
                ctx, dim, pitem->first_child, LAY_INVALID_ID,
                rect[dim], rect[2 + dim]);
        }
        break;
    default:
        lay_arrange_overlay(ctx, item, dim);
        break;
    }
    lay_id child = pitem->first_child;
    while (child != LAY_INVALID_ID) {
        // NOTE: this is recursive and will run out of stack space if items are
        // nested too deeply.
        lay_arrange(ctx, child, dim);
        lay_item_t *pchild = lay_get_item(ctx, child);
        child = pchild->next_sibling;
    }
}
