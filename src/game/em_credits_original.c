/* em_credits_original.c - see em_credits_original.h. The float arithmetic
 * is the EE's (em_ee_float.h); float_to_int is the boot ELF's soft-float
 * 001281C0 (em_effect_original_float_to_int); 0011DF78 is fabsf (the sign
 * bit cleared). */
#include "game/em_credits_original.h"

#include <string.h>

#include "game/em_ee_float.h"
#include "game/em_effect_original.h"

static float f32_at(const uint8_t *s, unsigned at)
{
    float v;
    memcpy(&v, s + at, 4);
    return v;
}
static void f32_set(uint8_t *s, unsigned at, float v) { memcpy(s + at, &v, 4); }
static int32_t s32_at(const uint8_t *s, unsigned at)
{
    int32_t v;
    memcpy(&v, s + at, 4);
    return v;
}
static void s32_set(uint8_t *s, unsigned at, int32_t v) { memcpy(s + at, &v, 4); }

static float fabs_0011DF78(float v) { return em_ee_float(em_ee_bits(v) & 0x7FFFFFFFu); }

void em_credits_0022D360(uint8_t state[0x20]) { memset(state, 0, 0x20); }

/* The half-line offset of both colours from the fraction (+0x1C): 0 or 12. */
static uint8_t offset_of(const uint8_t *s)
{
    const float y = f32_at(s, 0x08);
    const int half = f32_at(s, 0x1C) == 0.5f;
    if (y < 0.0f) return half ? 0 : 12;
    return half ? 12 : 0;
}

/* The fraction +0x1C = fabsf(y - (float)float_to_int(y)). */
static void fraction(uint8_t *s)
{
    const float t = f32_at(s, 0x08);
    f32_set(s, 0x1C, fabs_0011DF78(em_ee_sub(t, em_ee_cvt_s_w(em_effect_original_float_to_int(t)))));
}

/* One row drawn at line y: three runs, and type 1's underline. */
static int draw_row(const EmCreditsHooks *h, int32_t type, uint32_t text, int32_t y,
                    const uint8_t a[8], const uint8_t b[8])
{
    static const int32_t dx[3] = {0x7FF, 0x801, 0x800};
    int32_t w;
    for (int k = 0; k < 3; ++k) {
        if (h->cc170(h->ctx, text, &w) < 0) return -1;
        if (h->cc1e0(h->ctx, 1, -(w >> 1) + dx[k], y + 0x800, 10, 20, text, k < 2 ? b : a) < 0) return -1;
    }
    if (type != 1) return 0;
    int32_t w2;
    if (h->cc170(h->ctx, text, &w) < 0) return -1;
    const int32_t x = -(w >> 1);
    if (h->cc170(h->ctx, text, &w2) < 0) return -1;
    const int32_t x2 = x + w2;
    return h->f80(h->ctx, 1, (int32_t)((uint32_t)(x + 0x800) << 4), (int32_t)(((uint32_t)(y + 0x80C) << 4) + a[7]),
                  (int32_t)((uint32_t)(x2 + 0x802) << 4), (int32_t)(((uint32_t)(y + 0x80D) << 4) + a[7]),
                  0x80808080u);
}

/* The row loop of states 1 and 2: state 1 tests the end row before the
 * line (the hold starts once that row's line is above 0x70), state 2 ends
 * at it. */
static int rows(uint8_t *s, const EmCreditsHooks *h, const uint8_t a[8], const uint8_t b[8], int scrolling)
{
    for (uint32_t i = 0;; ++i) {
        const uint8_t *row = h->bytes(h->ctx, EM_CREDITS_TABLE + 8u * i, 8);
        if (!row) return -1;
        const int32_t type = s32_at(row, 0);
        const uint32_t text = (uint32_t)s32_at(row, 4);
        const int32_t y = em_effect_original_float_to_int(f32_at(s, 0x08)) + (int32_t)(0x15 * i);
        if (type == 4) {
            if (scrolling && y < 0x70) {
                s[0] = (uint8_t)(s[0] + 1);
                s32_set(s, 0x18, 200);
            }
            return 0;
        }
        if (y <= -0x90) continue;
        if (y >= 0x91) return 0;
        if (type >= 1 && type <= 3 && draw_row(h, type, text, y, a, b) < 0) return -1;
    }
}

int em_credits_0022D380(uint8_t s[0x20], const EmCreditsHooks *h, int32_t *result)
{
    uint8_t a[8] = {0x60, 0x60, 0x60, 0, 0x80, 0, 0, 0};
    uint8_t b[8] = {0x60, 0x60, 0x60, 0, 0x28, 0, 0, 0};
    *result = 0;
    switch (s[0]) {
    case 0: {
        s[0] = (uint8_t)(s[0] + 1);
        s32_set(s, 0x14, 0);
        const float start = em_ee_neg(em_ee_mul(0.5f, em_ee_cvt_s_w(s32_at(s, 0x14))));
        f32_set(s, 0x08, start);
        f32_set(s, 0x1C, em_ee_neg(em_ee_mul(0.5f, em_ee_cvt_s_w(s32_at(s, 0x14)))));
        while (f32_at(s, 0x1C) < 0.0f) f32_set(s, 0x1C, em_ee_add(f32_at(s, 0x1C), 1.0f));
        return 0;
    }
    case 1:
        f32_set(s, 0x08, em_ee_sub(f32_at(s, 0x08), 0.5f));
        f32_set(s, 0x1C, em_ee_sub(f32_at(s, 0x1C), 0.5f));
        fraction(s);
        a[7] = b[7] = offset_of(s);
        if (rows(s, h, a, b, 1) < 0) return -1;
        s32_set(s, 0x14, s32_at(s, 0x14) + 1);
        return 0;
    case 2:
        fraction(s);
        a[7] = b[7] = offset_of(s);
        if (rows(s, h, a, b, 0) < 0) return -1;
        s32_set(s, 0x18, s32_at(s, 0x18) - 1);
        if (s32_at(s, 0x18) < 0) *result = 1;
        return 0;
    default:
        return 0;
    }
}
