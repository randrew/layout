#include <stdio.h>
#include "layout.c"

#ifdef _WIN32
#include <windows.h>

LONG WINAPI LayTestUnhandledExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    fputs("Exception", stdout);
    fflush(stdout);
    // TODO Do useful stuff
    return EXCEPTION_EXECUTE_HANDLER;
}

#endif

/*
int sprint_vec4(char* str, lay_vec4 v)
{
    return sprintf(str, "%d, %d, %d, %d", v[0], v[1], v[2], v[3]);
}
int print_vec4(lay_vec4 v)
{
    return printf("%d, %d, %d, %d", v[0], v[1], v[2], v[3]);
}
int sprint_vec2(char* str, lay_vec2 v)
{
    return sprintf(str, "%d, %d", v[0], v[1]);
}
int sprint_item_info(char* str, lay_context *ctx, lay_id item)
{
    int offs = 0;
    lay_vec4 item_rect = lay_rect_get(ctx, item);

    lay_vec4 item_margins = lay_margins_get(ctx, item);
    lay_vec2 item_size = lay_size_get(ctx, item);

    offs += sprintf(str + offs, "rect: ");
    offs += sprint_vec4(str + offs, item_rect);

    offs += sprintf(str + offs, " | margins: ");
    offs += sprint_vec4(str + offs, item_margins);

    offs += sprintf(str + offs, " | size: ");
    offs += sprint_vec2(str + offs, item_size);

    return offs;
}
*/

#define LTEST_TRUE(cond) \
    if (cond) {} \
    else { \
        printf("Failed test at line %d in %s", __LINE__, __func__); \
        abort(); \
    }
#define LTEST_FALSE(cond) \
    if (cond) { \
        printf("Failed test at line %d in %s", __LINE__, __func__); \
        abort(); \
    } \
    else {}

#define LTEST_DECLARE(testname) \
    static void test_##testname(lay_context *ctx, char *strbuf)

// vec4 equals test
#define LTEST_VEC4EQ(vecvar, x, y, z, w) \
    LTEST_TRUE(vecvar[0] == x && vecvar[1] == y && vecvar[2] == z && vecvar[3] == w)
#define LTEST_VEC4UNEQ(vecvar, x, y, z, w) \
    LTEST_FALSE(vecvar[0] == x && vecvar[1] == y && vecvar[2] == z && vecvar[3] == w)

LTEST_DECLARE(testing_sanity)
{
	// MSVC (rightly) warns us that these are constant expressions which are
    // always or never true.
#ifndef _MSC_VER
    LTEST_TRUE(1);
    LTEST_TRUE(-1);
    LTEST_TRUE(((bool)(1)));
    LTEST_TRUE(!((bool)(0)));
    LTEST_TRUE(34518 == 34518);
    LTEST_TRUE(1337 != 420);

    LTEST_FALSE(0);
    LTEST_FALSE(!((bool)(1)));
    LTEST_FALSE(((bool)(0)));
    LTEST_FALSE(34518 != 34518);
    LTEST_FALSE(1337 == 420);
#endif

    lay_vec4 v1 = lay_vec4_xyzw(56, 57, 58, 59);
    LTEST_VEC4EQ(v1, 56, 57, 58, 59);
    LTEST_VEC4UNEQ(v1, 55, 57, 58, 59);
    LTEST_VEC4UNEQ(v1, 55, 57, 58, 59);

	LTEST_TRUE(ctx != NULL);
}

LTEST_DECLARE(simple_fill)
{
    lay_id root = lay_item(ctx);
    lay_id child = lay_item(ctx);

    lay_size_set_xy(ctx, root, 30, 40);
    lay_behave_set(ctx, child, LAY_FILL);
    lay_insert(ctx, root, child);

    lay_context_run(ctx);

    lay_vec4 root_r = lay_rect_get(ctx, root);
    lay_vec4 child_r = lay_rect_get(ctx, child);

    LTEST_TRUE(root_r[0] == 0 && root_r[1] == 0);
    LTEST_TRUE(root_r[2] == 30 && root_r[3] == 40);

    LTEST_TRUE(child_r[0] == 0 && child_r[1] == 0);
    LTEST_TRUE(child_r[2] == 30 && child_r[3] == 40);
}

LTEST_DECLARE(reserve_capacity)
{
    lay_context_reserve(ctx, 512);
    LTEST_TRUE(lay_context_item_capacity(ctx) >= 512);

    // Run some simple stuff like above, just to make sure it's still working

    lay_id root = lay_item(ctx);
    lay_id child = lay_item(ctx);

    lay_size_set_xy(ctx, root, 30, 40);
    lay_behave_set(ctx, child, LAY_FILL);
    lay_insert(ctx, root, child);

    lay_context_run(ctx);

    lay_vec4 root_r = lay_rect_get(ctx, root);
    lay_vec4 child_r = lay_rect_get(ctx, child);

    LTEST_TRUE(root_r[0] == 0 && root_r[1] == 0);
    LTEST_TRUE(root_r[2] == 30 && root_r[3] == 40);

    LTEST_TRUE(child_r[0] == 0 && child_r[1] == 0);
    LTEST_TRUE(child_r[2] == 30 && child_r[3] == 40);

    LTEST_TRUE(lay_context_item_capacity(ctx) >= 512);
}

LTEST_DECLARE(multiple_uninserted)
{
    lay_id root = lay_item(ctx);
    lay_id child1 = lay_item(ctx);
    lay_id child2 = lay_item(ctx);

    lay_size_set_xy(ctx, root, 155, 177);
    lay_size_set_xy(ctx, child2, 1, 1);

    lay_context_run(ctx);

    lay_vec4 root_r, child1_r, child2_r;
    root_r = lay_rect_get(ctx, root);
    child1_r = lay_rect_get(ctx, child1);
    child2_r = lay_rect_get(ctx, child2);

    LTEST_VEC4EQ(root_r, 0, 0, 155, 177);
    // This uninserted child should obviously be zero:
    LTEST_VEC4EQ(child1_r, 0, 0, 0, 0);
    // You might expect this to pass for this child:
    //
    // LTEST_VEC4EQ(child2_r, 0, 0, 1, 1);
    //
    // But it won't, because items not inserted into the root will not have
    // their output rect set. (Hmm, is this a good API design idea?)
    //
    // Instead, it will be zero:
    LTEST_VEC4EQ(child2_r, 0, 0, 0, 0);
}

LTEST_DECLARE(column_even_fill)
{
    lay_id root = lay_item(ctx);
    lay_id child_a = lay_item(ctx);
    lay_id child_b = lay_item(ctx);
    lay_id child_c = lay_item(ctx);

    lay_size_set_xy(ctx, root, 50, 60);
    lay_contain_set(ctx, root, LAY_COLUMN);
    lay_behave_set(ctx, child_a, LAY_FILL);
    lay_behave_set(ctx, child_b, LAY_FILL);
    lay_behave_set(ctx, child_c, LAY_FILL);
    lay_size_set_xy(ctx, child_a, 0, 0);
    lay_size_set_xy(ctx, child_b, 0, 0);
    lay_size_set_xy(ctx, child_c, 0, 0);
    lay_insert(ctx, root, child_a);
    lay_insert(ctx, root, child_b);
    lay_insert(ctx, root, child_c);

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, root), 0, 0, 50, 60);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_a), 0, 0, 50, 20);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_b), 0, 20, 50, 20);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_c), 0, 40, 50, 20);
}

LTEST_DECLARE(row_even_fill)
{
    lay_id root = lay_item(ctx);
    lay_id child_a = lay_item(ctx);
    lay_id child_b = lay_item(ctx);
    lay_id child_c = lay_item(ctx);

    lay_size_set_xy(ctx, root, 90, 3);
    lay_contain_set(ctx, root, LAY_ROW);
    lay_behave_set(ctx, child_a, LAY_HFILL | LAY_TOP);
    lay_behave_set(ctx, child_b, LAY_HFILL | LAY_VCENTER);
    lay_behave_set(ctx, child_c, LAY_HFILL | LAY_BOTTOM);
    lay_size_set_xy(ctx, child_a, 0, 1);
    lay_size_set_xy(ctx, child_b, 0, 1);
    lay_size_set_xy(ctx, child_c, 0, 1);
    lay_insert(ctx, root, child_a);
    lay_insert(ctx, root, child_b);
    lay_insert(ctx, root, child_c);

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, root),    0,  0, 90, 3);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_a), 0,  0, 30, 1);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_b), 30, 1, 30, 1);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_c), 60, 2, 30, 1);
}

LTEST_DECLARE(fixed_and_fill)
{
    lay_id root = lay_item(ctx);
    lay_id fixed_a = lay_item(ctx);
    lay_id fixed_b = lay_item(ctx);
    lay_id filler = lay_item(ctx);

    lay_contain_set(ctx, root, LAY_COLUMN);

    lay_size_set_xy(ctx, root, 50, 60);
    lay_size_set_xy(ctx, fixed_a, 50, 15);
    lay_size_set_xy(ctx, fixed_b, 50, 15);
    lay_behave_set(ctx, filler, LAY_FILL);

    lay_insert(ctx, root, fixed_a);
    lay_insert(ctx, root, filler);
    lay_insert(ctx, root, fixed_b);

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, root),    0,  0, 50, 60);
    LTEST_VEC4EQ(lay_rect_get(ctx, fixed_a), 0,  0, 50, 15);
    LTEST_VEC4EQ(lay_rect_get(ctx, filler),  0, 15, 50, 30);
    LTEST_VEC4EQ(lay_rect_get(ctx, fixed_b), 0, 45, 50, 15);
}

LTEST_DECLARE(simple_margins_1)
{
    lay_id root = lay_item(ctx);
    lay_id child_a = lay_item(ctx);
    lay_id child_b = lay_item(ctx);
    lay_id child_c = lay_item(ctx);

    lay_contain_set(ctx, root, LAY_COLUMN);
    lay_behave_set(ctx, child_a, LAY_HFILL);
    lay_behave_set(ctx, child_b, LAY_FILL);
    lay_behave_set(ctx, child_c, LAY_HFILL);

    lay_size_set_xy(ctx, root, 100, 90);

    lay_margins_set_ltrb(ctx, child_a, 3, 5, 7, 10);
    lay_size_set_xy(ctx, child_a, 0, (30 - (5 + 10)));
    lay_size_set_xy(ctx, child_c, 0, 30);

    lay_insert(ctx, root, child_a);
    lay_insert(ctx, root, child_b);
    lay_insert(ctx, root, child_c);

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, child_a), 3, 5, 90, (5 + 10));
    LTEST_VEC4EQ(lay_rect_get(ctx, child_b), 0, 30, 100, 30);
    LTEST_VEC4EQ(lay_rect_get(ctx, child_c), 0, 60, 100, 30);
}

LTEST_DECLARE(nested_boxes_1)
{
    const size_t num_rows = 5;
    // one of the rows is "fake" and will have 0 units tall height
    const size_t num_rows_with_height = num_rows - 1;

    lay_id root = lay_item(ctx);
    // main_child is a column that contains rows, and those rows
    // will contain columns.
    lay_id main_child = lay_item(ctx);
    lay_size_set_xy(ctx, root,
        70,
        // 10 units extra size above and below for main_child
        // margin
        (num_rows_with_height * 10 + 2 * 10)
    );
    lay_margins_set_ltrb(ctx, main_child, 10, 10, 10, 10);
    lay_contain_set(ctx, main_child, LAY_COLUMN);
    lay_insert(ctx, root, main_child);
    lay_behave_set(ctx, main_child, LAY_FILL);

    lay_id *rows = (lay_id*)calloc(num_rows, sizeof(lay_id));

    // auto-filling columns-in-row, each one should end up being
    // 10 units wide
    rows[0] = lay_item(ctx);
    lay_contain_set(ctx, rows[0], LAY_ROW);
    lay_behave_set(ctx, rows[0], LAY_FILL);
    lay_id cols1[5];
    // hmm so both the row and its child columns need to be set to
    // fill? which means main_child also needs to be set to fill?
    for (uint16_t i = 0; i < 5; ++i) {
        lay_id col = lay_item(ctx);
        // fill empty space
        lay_behave_set(ctx, col, LAY_FILL);
        lay_insert(ctx, rows[0], col);
        cols1[i] = col;
    }

    rows[1] = lay_item(ctx);
    lay_contain_set(ctx, rows[1], LAY_ROW);
    lay_behave_set(ctx, rows[1], LAY_VFILL);
    lay_id cols2[5];
    for (uint16_t i = 0; i < 5; ++i) {
        lay_id col = lay_item(ctx);
        // fixed-size horizontally, fill vertically
        lay_size_set_xy(ctx, col, 10, 0);
        lay_behave_set(ctx, col, LAY_VFILL);
        lay_insert(ctx, rows[1], col);
        cols2[i] = col;
    }

    // these columns have an inner item which sizes them
    rows[2] = lay_item(ctx);
    lay_contain_set(ctx, rows[2], LAY_ROW);
    lay_id cols3[2];
    for (uint16_t i = 0; i < 2; ++i) {
        lay_id col = lay_item(ctx);
        lay_id inner_sizer = lay_item(ctx);
        // only the second one will have height
        lay_size_set_xy(ctx, inner_sizer, 25, (lay_scalar)10 * i);
        // align to bottom, only should make a difference for
        // first item
        lay_behave_set(ctx, col, LAY_BOTTOM);
        lay_insert(ctx, col, inner_sizer);
        lay_insert(ctx, rows[2], col);
        cols3[i] = col;
    }

    // row 4 should end up being 0 units tall after layout
    rows[3] = lay_item(ctx);
    lay_contain_set(ctx, rows[3], LAY_ROW);
    lay_behave_set(ctx, rows[3], LAY_HFILL);
    lay_id cols4[99];
    for (uint16_t i = 0; i < 99; ++i) {
        lay_id col = lay_item(ctx);
        lay_insert(ctx, rows[3], col);
        cols4[i] = col;
    }

    // row 5 should be 10 pixels tall after layout, and each of
    // its columns should be 1 pixel wide
    rows[4] = lay_item(ctx);
    lay_contain_set(ctx, rows[4], LAY_ROW);
    lay_behave_set(ctx, rows[4], LAY_FILL);
    lay_id cols5[50];
    for (uint16_t i = 0; i < 50; ++i) {
        lay_id col = lay_item(ctx);
        lay_behave_set(ctx, col, LAY_FILL);
        lay_insert(ctx, rows[4], col);
        cols5[i] = col;
    }

    for (uint16_t i = 0; i < num_rows; ++i) {
        lay_insert(ctx, main_child, rows[i]);
    }

    // Repeat the run and tests multiple times to make sure we get the expected
    // results each time. The original version of oui would overwrite its input
    // state (intentionally) with the output state, so the context's input data
    // (margins, size) had to be "rebuilt" by the client code by doing a reset
    // and then filling it back up for each run. 'lay' does not have that
    // restriction.
    //
    // This is one of the more complex tests, so it's a good
    // choice for testing multiple runs of the same context.
    for (uint32_t run_n = 0; run_n < 5; ++run_n) {
        printf("    Iteration #%d\n", run_n + 1);
        lay_context_run(ctx);

        LTEST_VEC4EQ(lay_rect_get(ctx, main_child), 10, 10, 50, 40);
        // These rows should all be 10 units in height
        LTEST_VEC4EQ(lay_rect_get(ctx, rows[0]), 10, 10, 50, 10);
        LTEST_VEC4EQ(lay_rect_get(ctx, rows[1]), 10, 20, 50, 10);
        LTEST_VEC4EQ(lay_rect_get(ctx, rows[2]), 10, 30, 50, 10);
        // this row should have 0 height
        LTEST_VEC4EQ(lay_rect_get(ctx, rows[3]), 10, 40, 50,  0);
        LTEST_VEC4EQ(lay_rect_get(ctx, rows[4]), 10, 40, 50, 10);

        for (int16_t i = 0; i < 5; ++i) {
            lay_vec4 r = lay_rect_get(ctx, cols1[i]);
            // each of these should be 10 units wide, and stacked
            // horizontally
            LTEST_VEC4EQ(r, 10 + 10 * i, 10, 10, 10);
        }

        // the cols in the second row are similar to first row
        for (int16_t i = 0; i < 5; ++i) {
            lay_vec4 r = lay_rect_get(ctx, cols2[i]);
            LTEST_VEC4EQ(r, 10 + 10 * i, 20, 10, 10);
        }

        // leftmost (first of two items), aligned to bottom of row, 0
        // units tall
        LTEST_VEC4EQ(lay_rect_get(ctx, cols3[0]), 10, 40, 25, 0);
        // rightmost (second of two items), same height as row, which
        // is 10 units tall
        LTEST_VEC4EQ(lay_rect_get(ctx, cols3[1]), 35, 30, 25, 10);

        // these should all have size 0 and be in the middle of the
        // row
        for (int16_t i = 0; i < 99; ++i) {
            lay_vec4 r = lay_rect_get(ctx, cols4[i]);
            LTEST_VEC4EQ(r, 25 + 10, 40, 0, 0);
        }

        // these should all be 1 unit wide and 10 units tall
        for (int16_t i = 0; i < 50; ++i) {
            lay_vec4 r = lay_rect_get(ctx, cols5[i]);
            LTEST_VEC4EQ(r, 10 + i, 40, 1, 10);
        }
    }

    free(rows);
}

LTEST_DECLARE(deep_nest_1)
{
    lay_id root = lay_item(ctx);

    const int16_t num_items = 500;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    lay_id parent = root;
    for (int16_t i = 0; i < num_items; ++i)
    {
        lay_id item = lay_item(ctx);
        lay_insert(ctx, parent, item);
        parent = item;
    }

    lay_size_set_xy(ctx, parent, 77, 99);

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, root), 0, 0, 77, 99)

    free(items);
}

LTEST_DECLARE(many_children_1)
{
    const int16_t num_items = 20000;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 1, 0);
    lay_contain_set(ctx, root, LAY_COLUMN);

    lay_id prev = lay_item(ctx);
    lay_size_set_xy(ctx, prev, 1, 1);
    lay_insert(ctx, root, prev);
    for (int16_t i = 0; i < num_items - 1; ++i)
    {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 1, 1);
        lay_append(ctx, prev, item);
        prev = item;
    }

    lay_context_run(ctx);

    // with each child item being 1 unit tall, the total height should be num_items
    LTEST_VEC4EQ(lay_rect_get(ctx, root), 0, 0, 1, num_items)

    free(items);
}

LTEST_DECLARE(child_align_1)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 50, 50);

#define MK_ALIGN_BOX(n, flags) \
    lay_id aligned_box_##n = lay_item(ctx); \
    lay_size_set_xy(ctx, aligned_box_##n, 10, 10); \
    lay_behave_set(ctx, aligned_box_##n, flags); \
    lay_insert(ctx, root, aligned_box_##n);

    MK_ALIGN_BOX(1, LAY_TOP | LAY_LEFT);
    MK_ALIGN_BOX(2, LAY_TOP | LAY_RIGHT);
    MK_ALIGN_BOX(3, LAY_TOP | LAY_HCENTER);

    MK_ALIGN_BOX(4, LAY_VCENTER | LAY_LEFT);
    MK_ALIGN_BOX(5, LAY_VCENTER | LAY_RIGHT);
    MK_ALIGN_BOX(6, LAY_VCENTER | LAY_HCENTER);

    MK_ALIGN_BOX(7, LAY_BOTTOM | LAY_LEFT);
    MK_ALIGN_BOX(8, LAY_BOTTOM | LAY_RIGHT);
    MK_ALIGN_BOX(9, LAY_BOTTOM | LAY_HCENTER);

#undef MK_ALIGN_BOX

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_1),  0,  0, 10, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_2), 40,  0, 10, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_3), 20,  0, 10, 10);

    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_4),  0, 20, 10, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_5), 40, 20, 10, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_6), 20, 20, 10, 10);

    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_7),  0, 40, 10, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_8), 40, 40, 10, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_9), 20, 40, 10, 10);
}

LTEST_DECLARE(child_align_2)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 50, 50);

#define MK_ALIGN_BOX(n, flags) \
    lay_id aligned_box_##n = lay_item(ctx); \
    lay_size_set_xy(ctx, aligned_box_##n, 10, 10); \
    lay_behave_set(ctx, aligned_box_##n, flags); \
    lay_insert(ctx, root, aligned_box_##n);

    MK_ALIGN_BOX(1, LAY_TOP | LAY_HFILL);
    MK_ALIGN_BOX(2, LAY_VCENTER | LAY_HFILL);
    MK_ALIGN_BOX(3, LAY_BOTTOM | LAY_HFILL);

    MK_ALIGN_BOX(4, LAY_VFILL | LAY_LEFT);
    MK_ALIGN_BOX(5, LAY_VFILL | LAY_RIGHT);
    MK_ALIGN_BOX(6, LAY_VFILL | LAY_HCENTER);

#undef MK_ALIGN_BOX

    lay_context_run(ctx);

    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_1),  0,  0, 50, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_2),  0, 20, 50, 10);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_3),  0, 40, 50, 10);

    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_4),  0,  0, 10, 50);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_5), 40,  0, 10, 50);
    LTEST_VEC4EQ(lay_rect_get(ctx, aligned_box_6), 20,  0, 10, 50);
}

LTEST_DECLARE(wrap_row_1)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 50, 50);
    lay_contain_set(ctx, root, LAY_ROW | LAY_WRAP);

    // We will create a 5x5 grid of boxes that are 10x10 units per each box.
    // There should be no empty space, gaps, or extra wrapping.

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        x = i % 5;
        y = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  x * 10,  y * 10, 10, 10);
    }

    free(items);
}

LTEST_DECLARE(wrap_row_2)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 57, 57);
    lay_contain_set(ctx, root, LAY_ROW | LAY_WRAP | LAY_START);

    // This one should have extra space on the right edge and bottom (7 units)

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        x = i % 5;
        y = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  x * 10,  y * 10, 10, 10);
    }

    free(items);
}

LTEST_DECLARE(wrap_row_3)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 57, 57);
    lay_contain_set(ctx, root, LAY_ROW | LAY_WRAP | LAY_END);

    // This one should have extra space on the left edge and bottom (7 units)

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        //lay_behave_set(ctx, item, LAY_BOTTOM | LAY_RIGHT);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        x = i % 5;
        y = i / 5;
        //print_vec4(lay_rect_get(ctx, items[i]));
        //printf("\n");
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  7 + x * 10,  y * 10, 10, 10);
    }

    free(items);
}

LTEST_DECLARE(wrap_row_4)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 58, 57);
    lay_contain_set(ctx, root, LAY_ROW | LAY_WRAP | LAY_MIDDLE);

    lay_id spacer = lay_item(ctx);
    lay_size_set_xy(ctx, spacer, 58, 7);
    lay_insert(ctx, root, spacer);

    // This one should split the horizontal extra space between the left and
    // right, and have the vertical extra space at the top (via extra inserted
    // spacer item, with explicit size)

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        x = i % 5;
        y = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  4 + x * 10,  7 + y * 10, 10, 10);
    }

    free(items);
}

// bug? the last row of a LAY_JUSTIFY wrapping row container will not have its
// space divided evenly, and will instead behave like LAY_MIDDLE (or LAY_START?)

LTEST_DECLARE(wrap_row_5)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 54, 50);
    lay_contain_set(ctx, root, LAY_ROW | LAY_WRAP | LAY_JUSTIFY);

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    // TODO note we're adding the -5 here so we ignore the last row, which
    // seems to be bugged
    for (int16_t i = 0; i < num_items - 5; ++i) {
        int16_t x, y;
        x = i % 5;
        y = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]), x * 11,  y * 10, 10, 10);
    }

    free(items);
}

// Same as wrap_row_1, but for columns
LTEST_DECLARE(wrap_column_1)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 50, 50);
    lay_contain_set(ctx, root, LAY_COLUMN | LAY_WRAP);

    // We will create a 5x5 grid of boxes that are 10x10 units per each box.
    // There should be no empty space, gaps, or extra wrapping.

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        y = i % 5;
        x = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  x * 10,  y * 10, 10, 10);
    }

    free(items);
}

LTEST_DECLARE(wrap_column_2)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 57, 57);
    lay_contain_set(ctx, root, LAY_COLUMN | LAY_WRAP | LAY_START);

    // This one should have extra space on the right and bottom (7 units)

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        y = i % 5;
        x = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  x * 10,  y * 10, 10, 10);
    }

    free(items);
}

LTEST_DECLARE(wrap_column_3)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 57, 57);
    lay_contain_set(ctx, root, LAY_COLUMN | LAY_WRAP | LAY_END);

    // This one should have extra space on the top and right (7 units)

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        //lay_behave_set(ctx, item, LAY_BOTTOM | LAY_RIGHT);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        y = i % 5;
        x = i / 5;
        //print_vec4(lay_rect_get(ctx, items[i]));
        //printf("\n");
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  x * 10,  7 + y * 10, 10, 10);
    }

    free(items);
}

// Just like wrap_row_4, but as columns instead of rows
LTEST_DECLARE(wrap_column_4)
{
    lay_id root = lay_item(ctx);
    lay_size_set_xy(ctx, root, 57, 58);
    lay_contain_set(ctx, root, LAY_COLUMN | LAY_WRAP | LAY_MIDDLE);

    lay_id spacer = lay_item(ctx);
    lay_size_set_xy(ctx, spacer, 7, 58);
    lay_insert(ctx, root, spacer);

    const int16_t num_items = 5 * 5;
    lay_id *items = (lay_id*)calloc(num_items, sizeof(lay_id));

    for (int16_t i = 0; i < num_items; ++i) {
        lay_id item = lay_item(ctx);
        lay_size_set_xy(ctx, item, 10, 10);
        lay_insert(ctx, root, item);
        items[i] = item;
    }

    lay_context_run(ctx);

    for (int16_t i = 0; i < num_items; ++i) {
        int16_t x, y;
        y = i % 5;
        x = i / 5;
        LTEST_VEC4EQ(lay_rect_get(ctx, items[i]),  7 + x * 10,  4 + y * 10, 10, 10);
    }

    free(items);
}

// Call in main to run a test by name
//
// Resets string buffer and lay context before running test
#define LTEST_RUN(testname) \
    memset(sbuf, 0, 4096); \
    lay_context_reset(&ctx); \
    printf(" * " #testname "\n"); \
    test_##testname(&ctx, sbuf);

int main()
{
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(LayTestUnhandledExceptionFilter);
#endif

    char *sbuf = (char*)malloc(4096);

    lay_context ctx;
    lay_context_init(&ctx);

    printf("Running tests\n");

    LTEST_RUN(testing_sanity);
    LTEST_RUN(simple_fill);
    LTEST_RUN(reserve_capacity);
    LTEST_RUN(multiple_uninserted);
    LTEST_RUN(column_even_fill);
    LTEST_RUN(row_even_fill);
    LTEST_RUN(fixed_and_fill);
    LTEST_RUN(simple_margins_1);
    LTEST_RUN(nested_boxes_1);
    LTEST_RUN(deep_nest_1);
    LTEST_RUN(many_children_1);
    LTEST_RUN(child_align_1);
    LTEST_RUN(child_align_2);
    LTEST_RUN(wrap_row_1);
    LTEST_RUN(wrap_row_2);
    LTEST_RUN(wrap_row_3);
    LTEST_RUN(wrap_row_4);
    LTEST_RUN(wrap_row_5);
    LTEST_RUN(wrap_column_1);
    LTEST_RUN(wrap_column_2);
    LTEST_RUN(wrap_column_3);
    LTEST_RUN(wrap_column_4);

    printf("Finished tests\n");

    lay_context_destroy(&ctx);
    free(sbuf);
	return 0;
}
