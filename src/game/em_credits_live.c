/* em_credits_live.c - see em_credits_live.h. Nothing here computes. */
#include "game/em_credits_live.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/em_credits_original.h"
#include "game/em_message_live.h"
#include "game/em_status_gs_page.h"

enum { WINDOWS = 2 };
static struct {
    int tried, loaded;
    uint8_t *file;
    struct { uint32_t base, size; const uint8_t *bytes; } w[WINDOWS];
    uint8_t state[0x20];
} S;

static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

static int load(void)
{
    if (S.tried) return S.loaded ? 0 : -1;
    S.tried = 1;
    FILE *f = fopen(EM_CREDITS_EXPORT, "rb");
    if (!f) {
        fprintf(stderr, "credits: %s is missing (tools/export_credits.py)\n", EM_CREDITS_EXPORT);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    S.file = n > 12 && n < (1 << 20) ? malloc((size_t)n) : NULL;
    int ok = S.file && fread(S.file, 1, (size_t)n, f) == (size_t)n && !memcmp(S.file, "EMCR", 4) &&
             rd32(S.file + 4) == 1 && rd32(S.file + 8) == WINDOWS;
    fclose(f);
    long at = 12;
    for (unsigned k = 0; ok && k < WINDOWS; ++k) {
        ok = at + 8 <= n;
        if (!ok) break;
        S.w[k].base = rd32(S.file + at);
        S.w[k].size = rd32(S.file + at + 4);
        ok = (long)S.w[k].size <= n - at - 8;
        S.w[k].bytes = S.file + at + 8;
        at += 8 + (long)S.w[k].size;
    }
    if (!ok || at != n) {
        fprintf(stderr, "credits: %s is not an EMCR v1 export\n", EM_CREDITS_EXPORT);
        free(S.file);
        S.file = NULL;
        return -1;
    }
    S.loaded = 1;
    return 0;
}

static const uint8_t *window(uint32_t address, uint32_t *avail)
{
    for (unsigned k = 0; k < WINDOWS; ++k)
        if (address >= S.w[k].base && address - S.w[k].base < S.w[k].size) {
            *avail = S.w[k].size - (address - S.w[k].base);
            return S.w[k].bytes + (address - S.w[k].base);
        }
    return NULL;
}

static const uint8_t *h_bytes(void *ctx, uint32_t address, uint32_t size)
{
    (void)ctx;
    uint32_t avail = 0;
    const uint8_t *p = window(address, &avail);
    return p && avail >= size ? p : NULL;
}

static int h_cc170(void *ctx, uint32_t text, int32_t *width)
{
    (void)ctx;
    uint32_t avail = 0;
    const uint8_t *t = window(text, &avail);
    return t ? em_message_live_cc170(t, avail, width) : -1;
}

static int h_cc1e0(void *ctx, int32_t slot, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t text,
                   const uint8_t colour[8])
{
    (void)ctx;
    uint32_t avail = 0;
    const uint8_t *t = window(text, &avail);
    if (!t) return -1;
    EmMessageTextStyle st;
    memset(&st, 0, sizeof st);
    st.color = (int32_t)rd32(colour);
    st.glyph = colour[4];
    st.flag = colour[5];
    st.pad[0] = colour[6];
    st.pad[1] = colour[7];
    return em_message_live_cc1e0(slot, x, y, w, h, t, avail, &st);
}

static int h_f80(void *ctx, int32_t slot, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t rgba)
{
    (void)ctx;
    return em_sgp_00207F80(slot, x0, y0, x1, y1, rgba) ? 0 : -1;
}

int em_credits_live_0022D360(void)
{
    if (load() < 0) return -1;
    em_credits_0022D360(S.state);
    return 0;
}

int em_credits_live_0022D380(int32_t *result)
{
    if (load() < 0) return -1;
    const EmCreditsHooks h = {NULL, h_bytes, h_cc170, h_cc1e0, h_f80};
    if (em_credits_0022D380(S.state, &h, result) < 0) {
        fprintf(stderr, "credits: 0022D380 faulted (state %u)\n", (unsigned)S.state[0]);
        return -1;
    }
    return 0;
}

const uint8_t *em_credits_live_state(void) { return S.state; }
