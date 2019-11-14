#include <stdio.h>
#define SOKOL_IMPL
#include "sokol_time.h"
#undef SOKOL_IMPL
#define LAY_IMPLEMENTATION
#include "layout.h"
#undef LAY_IMPLEMENTATION

#ifdef _WIN32
#include <windows.h>

LONG WINAPI LayTestUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
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
    lay_vec4 item_rect = lay_get_rect(ctx, item);

    lay_vec4 item_margins = lay_get_margins(ctx, item);
    lay_vec2 item_size = lay_get_size(ctx, item);

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
    static void test_##testname(lay_context *ctx)

// vec4 equals test
#define LTEST_VEC4EQ(vecvar, x, y, z, w) \
    LTEST_TRUE(vecvar[0] == x && vecvar[1] == y && vecvar[2] == z && vecvar[3] == w)
#define LTEST_VEC4UNEQ(vecvar, x, y, z, w) \
    LTEST_FALSE(vecvar[0] == x && vecvar[1] == y && vecvar[2] == z && vecvar[3] == w)

static inline void benchmark_nested(lay_context *ctx)
{
    const size_t num_rows = 5;
    // one of the rows is "fake" and will have 0 units tall height
    const size_t num_rows_with_height = num_rows - 1;

    lay_id root = lay_item(ctx);
    // main_child is a column that contains rows, and those rows
    // will contain columns.
    lay_id main_child = lay_item(ctx);
    lay_set_size_xy(ctx, root,
        70,
        // 10 units extra size above and below for main_child
        // margin
        (num_rows_with_height * 10 + 2 * 10)
    );
    lay_set_margins_ltrb(ctx, main_child, 10, 10, 10, 10);
    lay_set_contain(ctx, main_child, LAY_COLUMN);
    lay_insert(ctx, root, main_child);
    lay_set_behave(ctx, main_child, LAY_FILL);

    lay_id *rows = (lay_id*)calloc(num_rows, sizeof(lay_id));

    // auto-filling columns-in-row, each one should end up being
    // 10 units wide
    rows[0] = lay_item(ctx);
    lay_set_contain(ctx, rows[0], LAY_ROW);
    lay_set_behave(ctx, rows[0], LAY_FILL);
    lay_id cols1[5];
    // hmm so both the row and its child columns need to be set to
    // fill? which means main_child also needs to be set to fill?
    for (size_t i = 0; i < 5; ++i) {
        lay_id col = lay_item(ctx);
        // fill empty space
        lay_set_behave(ctx, col, LAY_FILL);
        lay_insert(ctx, rows[0], col);
        cols1[i] = col;
    }

    rows[1] = lay_item(ctx);
    lay_set_contain(ctx, rows[1], LAY_ROW);
    lay_set_behave(ctx, rows[1], LAY_VFILL);
    lay_id cols2[5];
    for (size_t i = 0; i < 5; ++i) {
        lay_id col = lay_item(ctx);
        // fixed-size horizontally, fill vertically
        lay_set_size_xy(ctx, col, 10, 0);
        lay_set_behave(ctx, col, LAY_VFILL);
        lay_insert(ctx, rows[1], col);
        cols2[i] = col;
    }

    // these columns have an inner item which sizes them
    rows[2] = lay_item(ctx);
    lay_set_contain(ctx, rows[2], LAY_ROW);
    lay_id cols3[2];
    for (size_t i = 0; i < 2; ++i) {
        lay_id col = lay_item(ctx);
        lay_id inner_sizer = lay_item(ctx);
        // only the second one will have height
        lay_set_size_xy(ctx, inner_sizer, 25, 10 * (lay_scalar)i);
        // align to bottom, only should make a difference for
        // first item
        lay_set_behave(ctx, col, LAY_BOTTOM);
        lay_insert(ctx, col, inner_sizer);
        lay_insert(ctx, rows[2], col);
        cols3[i] = col;
    }

    // row 4 should end up being 0 units tall after layout
    rows[3] = lay_item(ctx);
    lay_set_contain(ctx, rows[3], LAY_ROW);
    lay_set_behave(ctx, rows[3], LAY_HFILL);
    lay_id cols4[99];
    for (size_t i = 0; i < 99; ++i) {
        lay_id col = lay_item(ctx);
        lay_insert(ctx, rows[3], col);
        cols4[i] = col;
    }

    // row 5 should be 10 pixels tall after layout, and each of
    // its columns should be 1 pixel wide
    rows[4] = lay_item(ctx);
    lay_set_contain(ctx, rows[4], LAY_ROW);
    lay_set_behave(ctx, rows[4], LAY_FILL);
    lay_id cols5[50];
    for (size_t i = 0; i < 50; ++i) {
        lay_id col = lay_item(ctx);
        lay_set_behave(ctx, col, LAY_FILL);
        lay_insert(ctx, rows[4], col);
        cols5[i] = col;
    }

    for (size_t i = 0; i < num_rows; ++i) {
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
        //printf("    Iteration #%d\n", run_n + 1);
        lay_run_context(ctx);

        LTEST_VEC4EQ(lay_get_rect(ctx, main_child), 10, 10, 50, 40);
        // These rows should all be 10 units in height
        LTEST_VEC4EQ(lay_get_rect(ctx, rows[0]), 10, 10, 50, 10);
        LTEST_VEC4EQ(lay_get_rect(ctx, rows[1]), 10, 20, 50, 10);
        LTEST_VEC4EQ(lay_get_rect(ctx, rows[2]), 10, 30, 50, 10);
        // this row should have 0 height
        LTEST_VEC4EQ(lay_get_rect(ctx, rows[3]), 10, 40, 50,  0);
        LTEST_VEC4EQ(lay_get_rect(ctx, rows[4]), 10, 40, 50, 10);

        for (int16_t i = 0; i < 5; ++i) {
            lay_vec4 r = lay_get_rect(ctx, cols1[i]);
            // each of these should be 10 units wide, and stacked
            // horizontally
            LTEST_VEC4EQ(r, 10 + 10 * i, 10, 10, 10);
        }

        // the cols in the second row are similar to first row
        for (int16_t i = 0; i < 5; ++i) {
            lay_vec4 r = lay_get_rect(ctx, cols2[i]);
            LTEST_VEC4EQ(r, 10 + 10 * i, 20, 10, 10);
        }

        // leftmost (first of two items), aligned to bottom of row, 0
        // units tall
        LTEST_VEC4EQ(lay_get_rect(ctx, cols3[0]), 10, 40, 25, 0);
        // rightmost (second of two items), same height as row, which
        // is 10 units tall
        LTEST_VEC4EQ(lay_get_rect(ctx, cols3[1]), 35, 30, 25, 10);

        // these should all have size 0 and be in the middle of the
        // row
        for (int16_t i = 0; i < 99; ++i) {
            lay_vec4 r = lay_get_rect(ctx, cols4[i]);
            LTEST_VEC4EQ(r, 25 + 10, 40, 0, 0);
        }

        // these should all be 1 unit wide and 10 units tall
        for (int16_t i = 0; i < 50; ++i) {
            lay_vec4 r = lay_get_rect(ctx, cols5[i]);
            LTEST_VEC4EQ(r, 10 + i, 40, 1, 10);
        }
    }

    free(rows);
}

// Call in main to run a test by name
//
// Resets string buffer and lay context before running test
#define LBENCH_RUN(testname) \
    lay_reset_context(&ctx); \
    printf(" * " #testname "\n");

int main()
{
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(LayTestUnhandledExceptionFilter);
#endif
    stm_setup();

    lay_context ctx;
    lay_init_context(&ctx);

    printf("Running benchmarks\n");

    //LBENCH_RUN(benchmark_nested);
    uint64_t total_perfc = 0;
    const uint32_t num_runs = 100000;
    uint64_t *run_times = (uint64_t*)calloc(num_runs, sizeof(uint64_t));
    for (uint32_t run_n = 0; run_n < num_runs; ++run_n) {
        lay_reset_context(&ctx);
        uint64_t t1 = stm_now();
        //printf(" * " #testname "\n");
        benchmark_nested(&ctx);
        uint64_t diff = stm_since(t1);
        total_perfc += diff;
        run_times[run_n] = diff;
        //double seconds = perf_seconds(freq, diff);
        //printf("Run %d: %f microsecs\n", run_n + 1, seconds * 1000000.0);
    }

    double avg = stm_us(total_perfc) / (double)num_runs;
    printf("Average time: %f usecs", avg);

    free(run_times);

    lay_destroy_context(&ctx);
    return 0;
}
