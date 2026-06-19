/*
** test.c — test suite for libft_malloc
**
** Tests are organised into sections, each targeting a specific guarantee
** of the allocator. 
** Every ASSERT prints [OK] or [FAIL] with its description.
** The process exits with code 0 if the assertion pass, != 0 otherwise.
**
** Sections
** --------
** malloc: basic
**   malloc(0) must return NULL.
**   malloc of a TINY (1-128), SMALL (129-1024) and LARGE (>1024) size must
**   return a non-NULL pointer.
**
** malloc: alignment
**   Every pointer returned by malloc must be 16-byte aligned, regardless of
**   the requested size. This is required by the 64-bit ABI for SIMD types.
**
** malloc: no overlap
**   Two independent allocations must never share any byte. Tested for
**   same-class pairs (TINY/TINY, SMALL/SMALL, LARGE/LARGE) and cross-class
**   triples (TINY + SMALL + LARGE).
**
** malloc: writable
**   The full requested region must be readable and writable. Each class is
**   filled with a sentinel byte via memset and verified byte-by-byte.
**
** free: edge cases
**   free(NULL) must be a no-op (POSIX requirement).
**   Freeing a valid pointer once must not crash.
**   Double-freeing the same pointer must not crash (our guard checks
**   block->free == 1 and silently returns on the second call).
**
** free: zone reuse
**   After a block is freed, the next malloc of the same size must succeed
**   and return a valid, aligned pointer (the freed slot is reused).
**
** realloc: POSIX edge cases
**   realloc(NULL, n)  must behave like malloc(n).
**   realloc(ptr, 0)   must free ptr and return NULL.
**   realloc(ptr, n)   where n produces the same aligned size as the current
**                     block must return the original pointer unchanged.
**
** realloc: data preserved
**   Growing across classes (TINY→SMALL, SMALL→LARGE) must copy the original
**   bytes into the new allocation. Shrinking within the same class must
**   return the same pointer with data intact.
**
** stress: 150 TINY allocs
**   Each TINY zone holds at most 102 max-size (128-byte) blocks. Allocating
**   150 forces the allocator to mmap a second zone. All 150 pointers must be
**   non-NULL and pairwise non-overlapping.
**
** stress: interleaved
**   10 TINY + 10 SMALL + 5 LARGE allocations are made concurrently, then
**   freed in two alternating passes (even indices first, then odd). Tests
**   that the free list and zone accounting remain consistent under mixed load.
**
** show_alloc_mem: visual check
**   show_alloc_mem() is called at three states (after alloc, after partial
**   free, after full free) and inspected manually. The automated assertion
**   only verifies it does not crash.
*/

#include <stdio.h>
#include <string.h>
#include "malloc.h"

/* ── test framework ──────────────────────────────────────────────────────── */

static int g_pass = 0;
static int g_fail = 0;

#define SECTION(name) printf("\n=== %s ===\n", (name))

#define ASSERT(desc, expr) do {                        \
    if (expr) {                                        \
        printf("  [OK]   %s\n", (desc));               \
        g_pass++;                                      \
    } else {                                           \
        printf("  [FAIL] %s  (line %d)\n",             \
               (desc), __LINE__);                      \
        g_fail++;                                      \
    }                                                  \
} while (0)

static int overlaps(void *a, size_t sa, void *b, size_t sb)
{
    return !((char *)a + sa <= (char *)b
          || (char *)b + sb <= (char *)a);
}

/* ── malloc ──────────────────────────────────────────────────────────────── */

static void test_malloc_basic(void)
{
    void *p;

    SECTION("malloc: basic");

    ASSERT("malloc(0) returns NULL",        malloc(0) == NULL);

    p = malloc(1);
    ASSERT("malloc(1) returns non-NULL",    p != NULL);
    free(p);

    p = malloc(16);
    ASSERT("malloc(16) returns non-NULL",   p != NULL);
    free(p);

    p = malloc(128);
    ASSERT("malloc(128) (max TINY) non-NULL", p != NULL);
    free(p);

    p = malloc(129);
    ASSERT("malloc(129) (min SMALL) non-NULL", p != NULL);
    free(p);

    p = malloc(1024);
    ASSERT("malloc(1024) (max SMALL) non-NULL", p != NULL);
    free(p);

    p = malloc(1025);
    ASSERT("malloc(1025) (min LARGE) non-NULL", p != NULL);
    free(p);

    p = malloc(1024 * 1024);
    ASSERT("malloc(1 MB) non-NULL",         p != NULL);
    free(p);
}

static void test_malloc_alignment(void)
{
    void    *ptrs[8];
    size_t  sizes[8] = {1, 7, 8, 9, 15, 16, 127, 128};

    SECTION("malloc: alignment (all pointers must be 16-byte aligned)");

    for (int i = 0; i < 8; i++)
    {
        ptrs[i] = malloc(sizes[i]);
        ASSERT("pointer is 16-byte aligned",
               ptrs[i] != NULL && ((unsigned long)ptrs[i] % 16) == 0);
    }
    for (int i = 0; i < 8; i++)
        free(ptrs[i]);

    void *p = malloc(1025);
    ASSERT("LARGE pointer is 16-byte aligned",
           p != NULL && ((unsigned long)p % 16) == 0);
    free(p);
}

static void test_malloc_no_overlap(void)
{
    void *a, *b, *c;

    SECTION("malloc: allocations do not overlap");

    a = malloc(64);
    b = malloc(64);
    ASSERT("two TINY allocs don't overlap", !overlaps(a, 64, b, 64));
    free(a);
    free(b);

    a = malloc(512);
    b = malloc(512);
    ASSERT("two SMALL allocs don't overlap", !overlaps(a, 512, b, 512));
    free(a);
    free(b);

    a = malloc(4096);
    b = malloc(4096);
    ASSERT("two LARGE allocs don't overlap", !overlaps(a, 4096, b, 4096));
    free(a);
    free(b);

    a = malloc(16);
    b = malloc(512);
    c = malloc(2048);
    ASSERT("TINY + SMALL + LARGE don't overlap (a,b)", !overlaps(a, 16, b, 512));
    ASSERT("TINY + SMALL + LARGE don't overlap (a,c)", !overlaps(a, 16, c, 2048));
    ASSERT("TINY + SMALL + LARGE don't overlap (b,c)", !overlaps(b, 512, c, 2048));
    free(a);
    free(b);
    free(c);
}

static void test_malloc_writable(void)
{
    char    *p;
    int     ok;

    SECTION("malloc: allocated memory is writable");

    p = malloc(128);
    memset(p, 0xAB, 128);
    ok = 1;
    for (int i = 0; i < 128; i++)
        if ((unsigned char)p[i] != 0xAB) { ok = 0; break; }
    ASSERT("TINY: write + read back 0xAB over 128 bytes", ok);
    free(p);

    p = malloc(1024);
    memset(p, 0xCD, 1024);
    ok = 1;
    for (int i = 0; i < 1024; i++)
        if ((unsigned char)p[i] != 0xCD) { ok = 0; break; }
    ASSERT("SMALL: write + read back 0xCD over 1024 bytes", ok);
    free(p);

    p = malloc(8192);
    memset(p, 0xEF, 8192);
    ok = 1;
    for (int i = 0; i < 8192; i++)
        if ((unsigned char)p[i] != 0xEF) { ok = 0; break; }
    ASSERT("LARGE: write + read back 0xEF over 8192 bytes", ok);
    free(p);
}

/* ── free ────────────────────────────────────────────────────────────────── */

static void test_free_basic(void)
{
    void *p;

    SECTION("free: edge cases");

    free(NULL);
    ASSERT("free(NULL) does not crash", 1);

    p = malloc(16);
    free(p);
    ASSERT("free after TINY malloc does not crash", 1);

    p = malloc(512);
    free(p);
    ASSERT("free after SMALL malloc does not crash", 1);

    p = malloc(2000);
    free(p);
    ASSERT("free after LARGE malloc does not crash", 1);

    p = malloc(64);
    free(p);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
    free(p);  /* intentional double-free: our guard (block->free == 1) silently returns */
#pragma GCC diagnostic pop
    ASSERT("double free does not crash (guard: block->free == 1)", 1);
}

static void test_free_zone_reuse(void)
{
    void *first, *second;

    SECTION("free: zone memory is reused");

    first = malloc(16);
    free(first);
    second = malloc(16);
    ASSERT("re-malloc after free returns a valid pointer", second != NULL);
    ASSERT("re-malloc is 16-byte aligned", ((unsigned long)second % 16) == 0);
    free(second);
}

/* ── realloc ─────────────────────────────────────────────────────────────── */

static void test_realloc_edge_cases(void)
{
    void *p, *q;

    SECTION("realloc: POSIX edge cases");

    p = realloc(NULL, 64);
    ASSERT("realloc(NULL, 64) behaves like malloc(64)", p != NULL);
    free(p);

    p = malloc(64);
    q = realloc(p, 0);
    ASSERT("realloc(ptr, 0) returns NULL", q == NULL);

    p = malloc(128);
    {
        void *orig = p;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
        q = realloc(p, 128); /* same size: our impl returns p unchanged */
#pragma GCC diagnostic pop
        ASSERT("realloc(ptr, same_size) returns same pointer", q == orig);
    }
    free(q);
}

static void test_realloc_data_preserved(void)
{
    char    *p, *q;
    int     ok;

    SECTION("realloc: data is preserved across resize");

    p = malloc(64);
    memset(p, 0x42, 64);
    q = realloc(p, 512);
    ASSERT("realloc TINY→SMALL returns non-NULL", q != NULL);
    ok = 1;
    for (int i = 0; i < 64; i++)
        if ((unsigned char)q[i] != 0x42) { ok = 0; break; }
    ASSERT("realloc TINY→SMALL: first 64 bytes preserved", ok);
    free(q);

    p = malloc(512);
    memset(p, 0x55, 512);
    q = realloc(p, 4096);
    ASSERT("realloc SMALL→LARGE returns non-NULL", q != NULL);
    ok = 1;
    for (int i = 0; i < 512; i++)
        if ((unsigned char)q[i] != 0x55) { ok = 0; break; }
    ASSERT("realloc SMALL→LARGE: first 512 bytes preserved", ok);
    free(q);

    p = malloc(512);
    {
        void *orig = p;
        q = realloc(p, 128);
        ASSERT("realloc shrink same-class returns same pointer", q == orig);
    }
    free(q);
}

/* ── stress ──────────────────────────────────────────────────────────────── */

static void test_stress_many_tiny(void)
{
    void    *ptrs[150];
    int     all_non_null;
    int     no_overlap;

    SECTION("stress: 150 TINY allocs (forces 2 zones, max capacity is 102 per zone)");

    all_non_null = 1;
    for (int i = 0; i < 150; i++)
    {
        ptrs[i] = malloc(128);
        if (!ptrs[i]) { all_non_null = 0; break; }
    }
    ASSERT("all 150 malloc(128) return non-NULL", all_non_null);

    no_overlap = 1;
    for (int i = 0; i < 150 && no_overlap; i++)
        for (int j = i + 1; j < 150 && no_overlap; j++)
            if (overlaps(ptrs[i], 128, ptrs[j], 128))
                no_overlap = 0;
    ASSERT("no two of the 150 blocks overlap", no_overlap);

    for (int i = 0; i < 150; i++)
        free(ptrs[i]);
    ASSERT("freeing all 150 blocks does not crash", 1);
}

static void test_stress_interleaved(void)
{
    void    *tiny[10];
    void    *small[10];
    void    *large[5];

    SECTION("stress: interleaved TINY / SMALL / LARGE alloc + free");

    for (int i = 0; i < 10; i++) tiny[i]  = malloc(64);
    for (int i = 0; i < 10; i++) small[i] = malloc(512);
    for (int i = 0; i < 5;  i++) large[i] = malloc(8192);

    for (int i = 0; i < 10; i++)
        ASSERT("interleaved: TINY ptr non-NULL",  tiny[i]  != NULL);
    for (int i = 0; i < 10; i++)
        ASSERT("interleaved: SMALL ptr non-NULL", small[i] != NULL);
    for (int i = 0; i < 5;  i++)
        ASSERT("interleaved: LARGE ptr non-NULL", large[i] != NULL);

    for (int i = 0; i < 10; i += 2) free(tiny[i]);
    for (int i = 0; i < 10; i += 2) free(small[i]);
    for (int i = 0; i < 5;  i += 2) free(large[i]);
    ASSERT("freeing every other block does not crash", 1);

    for (int i = 1; i < 10; i += 2) free(tiny[i]);
    for (int i = 1; i < 10; i += 2) free(small[i]);
    for (int i = 1; i < 5;  i += 2) free(large[i]);
    ASSERT("freeing remaining blocks does not crash", 1);
}

/* ── show_alloc_mem ──────────────────────────────────────────────────────── */

static void test_show_alloc_mem(void)
{
    void *a, *b, *c;

    SECTION("show_alloc_mem: visual check (inspect output manually)");

    a = malloc(42);
    b = malloc(400);
    c = malloc(10000);
    printf("  [INFO] allocations: TINY=%p SMALL=%p LARGE=%p\n", a, b, c);
    show_alloc_mem();
    free(b);
    printf("  [INFO] after free(SMALL):\n");
    show_alloc_mem();
    free(a);
    free(c);
    printf("  [INFO] after free(TINY) and free(LARGE) — should be empty:\n");
    show_alloc_mem();
    ASSERT("show_alloc_mem does not crash", 1);
}

/* ── coalesce ────────────────────────────────────────────────────────────── */

/*
** Coalescing is verified by engineering merged blocks of a precise size so that
** a subsequent malloc returns exactly the original address (no split, first-fit).
**
** Merged size formula: size_A + sizeof(t_block) + size_B
** split_block skips when: merged_size <= aligned + sizeof(t_block) + 16
** → choosing aligned = merged_size - sizeof(t_block) - 16 gives a tight equality,
**   preventing any split and making the returned pointer deterministic.
**
** A(32) + B(48): merged = 32 + 32 + 48 = 112. malloc(64): 112 == 64 + 32 + 16 → no split.
** A(32) + B(48) + C(32): merged = 32+32+48+32+32 = 176. malloc(128): 176 == 128+48 → no split.
*/
static void test_coalesce(void)
{
    char    *a, *b, *c, *d, *guard;
    int     ok, i;

    SECTION("coalesce: adjacent free blocks are merged");

    /* backward merge: free a then b — b's prev (a) is free → a absorbs b */
    a = malloc(32); b = malloc(48); guard = malloc(16);
    free(a);
    free(b);
    c = malloc(64); /* merged size 112 == 64+48 → no split; first-fit gives us a */
    ASSERT("backward-merge: malloc from merged block returns original ptr",
           (void *)c == (void *)a);
    free(c);
    free(guard);

    /* forward merge: free b then a — a's next (b) is free → a absorbs b */
    a = malloc(32); b = malloc(48); guard = malloc(16);
    free(b);
    free(a);
    c = malloc(64); /* same merged size and result */
    ASSERT("forward-merge: malloc from merged block returns original ptr",
           (void *)c == (void *)a);
    free(c);
    free(guard);

    /* triple merge: free a and c, then free b — b merges forward with c,
       then backward with a; result is one block spanning all three */
    a = malloc(32); b = malloc(48); c = malloc(32); guard = malloc(16);
    free(a);
    free(c);
    free(b); /* forward: B+C → 48+32+32=112; backward: A+(B+C) → 32+32+112=176 */
    d = malloc(128); /* 176 == 128+48 → no split; returns a */
    ASSERT("triple-merge: all three blocks merge, ptr == original a",
           (void *)d == (void *)a);
    free(d);
    free(guard);

    /* coalesced region is writable */
    a = malloc(64); b = malloc(64); guard = malloc(16);
    free(a);
    free(b);
    c = malloc(100);
    ASSERT("coalesced block is non-NULL", c != NULL);
    memset(c, 0xCC, 100);
    ok = 1;
    for (i = 0; i < 100; i++)
        if ((unsigned char)c[i] != 0xCC) { ok = 0; break; }
    ASSERT("coalesced block is fully writable", ok);
    free(c);
    free(guard);
}

/* ── realloc: in-place shrink ────────────────────────────────────────────── */

static void test_realloc_inplace_shrink(void)
{
    char    *a, *q, *tail;
    int     ok, i;

    SECTION("realloc: in-place shrink");

    /* shrink returns same pointer and preserves data */
    a = malloc(128);
    memset(a, 0x42, 128);
    q = realloc(a, 16);
    ASSERT("shrink TINY 128→16: same pointer returned", (void *)q == (void *)a);
    ok = 1;
    for (i = 0; i < 16; i++)
        if ((unsigned char)q[i] != 0x42) { ok = 0; break; }
    ASSERT("shrink TINY 128→16: kept bytes preserved", ok);
    free(q);

    /* freed tail becomes available for a subsequent malloc
       tail block header at a+16, user data at a+16+sizeof(t_block) = a+48 */
    a = malloc(128);
    q = realloc(a, 16);
    tail = malloc(64);
    ASSERT("shrink: freed tail is reusable (non-NULL)", tail != NULL);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
    ASSERT("shrink: tail allocation is at the expected offset from original ptr",
           (char *)tail == (char *)a + 16 + (int)sizeof(t_block));
#pragma GCC diagnostic pop
    free(q);
    free(tail);

    /* remainder too small to split: same pointer, no tail block created */
    a = malloc(64);
    q = realloc(a, 48);
    /* remainder = 64-48 = 16; threshold = 32+16 = 48; 16 <= 48 → no split */
    ASSERT("shrink: remainder too small to split → same pointer, no crash", (void *)q == (void *)a);
    free(q);

    /* shrink SMALL, data preserved */
    a = malloc(512);
    memset(a, 0x77, 512);
    q = realloc(a, 128);
    ASSERT("shrink SMALL 512→128: same pointer returned", (void *)q == (void *)a);
    ok = 1;
    for (i = 0; i < 128; i++)
        if ((unsigned char)q[i] != 0x77) { ok = 0; break; }
    ASSERT("shrink SMALL 512→128: kept bytes preserved", ok);
    free(q);
}

/* ── realloc: in-place grow ──────────────────────────────────────────────── */

static void test_realloc_inplace_grow(void)
{
    char    *a, *b, *q, *guard;
    int     ok, i;

    SECTION("realloc: in-place grow");

    /* grow within TINY when next block is free */
    a     = malloc(32);
    b     = malloc(64);
    guard = malloc(16);
    memset(a, 0x99, 32);
    free(b);
    q = realloc(a, 64);
    /* next_total=32+64=96; combined=32+96=128; remainder=128-64=64 > 48 → split */
    ASSERT("grow TINY in-place: same pointer returned", (void *)q == (void *)a);
    ok = 1;
    for (i = 0; i < 32; i++)
        if ((unsigned char)q[i] != 0x99) { ok = 0; break; }
    ASSERT("grow TINY in-place: original data preserved", ok);
    memset(q + 32, 0xAA, 32);
    ok = 1;
    for (i = 32; i < 64; i++)
        if ((unsigned char)q[i] != 0xAA) { ok = 0; break; }
    ASSERT("grow TINY in-place: extended region is writable", ok);
    free(q);
    free(guard);

    /* grow when next block is occupied: fallback to malloc+copy */
    a = malloc(32);
    b = malloc(64); /* b stays allocated → can't grow in-place */
    memset(a, 0xBB, 32);
    q = realloc(a, 64);
    ASSERT("grow with occupied next: result is non-NULL", q != NULL);
    ok = 1;
    for (i = 0; i < 32; i++)
        if ((unsigned char)q[i] != 0xBB) { ok = 0; break; }
    ASSERT("grow with occupied next: data preserved via copy", ok);
    free(q);
    free(b);
}

/* ── show_alloc_mem_ex ───────────────────────────────────────────────────── */

static void test_show_alloc_mem_ex(void)
{
    void    *a, *b, *c;

    SECTION("show_alloc_mem_ex: all blocks shown with hex dump (inspect output)");

    a = malloc(24);
    b = malloc(300);
    c = malloc(8000);
    memset(a, 'A', 24);
    memset(b, 'B', 300);
    memset(c, 'C', 8000);
    printf("  [INFO] 3 live allocations (TINY/SMALL/LARGE):\n");
    show_alloc_mem_ex();

    free(b);
    printf("  [INFO] after free(SMALL) — one free block should appear:\n");
    show_alloc_mem_ex();

    free(a);
    free(c);
    printf("  [INFO] all freed — should be empty:\n");
    show_alloc_mem_ex();

    ASSERT("show_alloc_mem_ex does not crash", 1);
}

/* ── summary ─────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("malloc test suite\n");

    test_malloc_basic();
    test_malloc_alignment();
    test_malloc_no_overlap();
    test_malloc_writable();
    test_free_basic();
    test_free_zone_reuse();
    test_realloc_edge_cases();
    test_realloc_data_preserved();
    test_stress_many_tiny();
    test_stress_interleaved();
    test_show_alloc_mem();
    test_coalesce();
    test_realloc_inplace_shrink();
    test_realloc_inplace_grow();
    test_show_alloc_mem_ex();

    printf("\n\r  ==========================================\n");
    printf("\r    Results: %d passed, %d failed\n", g_pass, g_fail);
    printf("\r  ==========================================\n");

    return (g_fail > 0 ? 1 : 0);
}
