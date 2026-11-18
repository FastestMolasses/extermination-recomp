/* The effect handler 001EB440 (em_effect_001EB440.h: its behaviour),
 * translated from the decomp's byte-identical C; loads and stores in the
 * original's order. */
#include "game/em_effect_001EB440.h"
#include "game/em_ee_float.h"
#include <string.h>

#define XF 0x0081F8F0u
#define WORK 0x00275C34u
enum { K_0 = 0u, K_2000 = 0x44FA0000u, K_65535 = 0x477FFF00u, K_BIAS = 0x38D1B717u,
       K_ONE = 0x3F800000u, K_EPS = 0x358637BDu, K_15 = 0x41700000u };

typedef struct { const EmEffect001EB440Hooks *h; EmEffect001EB440Fault *fault; } E;

static int latch(E *e, uint32_t address, int32_t code)
{
    if (e->fault && !e->fault->code) { e->fault->address = address; e->fault->code = code; }
    return -1;
}
static int word(E *e, uint32_t address, uint32_t *value, int write)
{
    uint8_t *p = e->h->bytes ? e->h->bytes(e->h->ctx, address, 4, write) : NULL;
    if (!p) return latch(e, address, EM_EFFECT_001EB440_FAULT_BAD_ADDRESS);
    if (write) memcpy(p, value, 4); else memcpy(value, p, 4);
    return 0;
}
static int fog(E *e, int32_t mode, uint32_t f13)
{
    if (!e->h->w_0021B9A0) return latch(e, 0x0021B9A0u, EM_EFFECT_001EB440_FAULT_NULL_WORKER);
    return e->h->w_0021B9A0(e->h->ctx, mode, K_0, f13) < 0
               ? latch(e, 0x0021B9A0u, EM_EFFECT_001EB440_FAULT_WORKER_FAILED) : 0;
}
#define TRY(x) do { if ((x) < 0) return -1; } while (0)

static int pass(E *e, uint32_t a0, int32_t a1, uint32_t source)
{
    uint32_t w, r, scale;
    TRY(word(e, WORK, &w, 0));
    TRY(word(e, w + 4u, &r, 0));
    uint32_t f = em_ee_div_bits(em_ee_cvt_s_w_bits((r >> 16) & 0xFFFFu), K_65535);
    uint32_t next = r * 0x25u + 0xBu;
    TRY(word(e, w + 4u, &next, 1));
    TRY(word(e, WORK, &w, 0));
    TRY(word(e, w + 0x54u, &scale, 0));
    f = em_ee_add_bits(f, K_BIAS);
    if (!e->h->w_001CFB50) return latch(e, 0x001CFB50u, EM_EFFECT_001EB440_FAULT_NULL_WORKER);
    if (e->h->w_001CFB50(e->h->ctx, XF, 0, a0, scale, f, K_ONE, K_EPS, K_15) < 0)
        return latch(e, 0x001CFB50u, EM_EFFECT_001EB440_FAULT_WORKER_FAILED);
    if (!e->h->w_001CFBE0) return latch(e, 0x001CFBE0u, EM_EFFECT_001EB440_FAULT_NULL_WORKER);
    if (e->h->w_001CFBE0(e->h->ctx, a1, 5, source, XF, 0) < 0)
        return latch(e, 0x001CFBE0u, EM_EFFECT_001EB440_FAULT_WORKER_FAILED);
    return 0;
}

static int pair(const EmEffect001EB440Hooks *h, uint32_t a0, int32_t a1, EmEffect001EB440Fault *fault,
                int lower, uint32_t first, uint32_t second)
{
    if (!h) return -1;
    E e = {h, fault};
    if (lower) {   /* 001EB250: a0 + 0x38 -= 2 unless w[+0x54] <= 0.8 */
        uint32_t w, scale, z;
        TRY(word(&e, WORK, &w, 0));
        TRY(word(&e, w + 0x54u, &scale, 0));
        if (!em_ee_c_le_bits(scale, 0x3F4CCCCDu)) {
            TRY(word(&e, a0 + 0x38u, &z, 0));
            z = em_ee_sub_bits(z, 0x40000000u);
            TRY(word(&e, a0 + 0x38u, &z, 1));
        }
    }
    TRY(fog(&e, 2, K_0));
    TRY(fog(&e, 3, K_2000));
    TRY(pass(&e, a0, a1, first));
    TRY(pass(&e, a0, a1, second));
    return fog(&e, 1, K_0);
}

int em_effect_001EB440(const EmEffect001EB440Hooks *h, uint32_t a0, int32_t a1,
                       EmEffect001EB440Fault *fault)
{
    return pair(h, a0, a1, fault, 0, 0x00255C50u, 0x00255CE0u);
}

int em_effect_001EB250(const EmEffect001EB440Hooks *h, uint32_t a0, int32_t a1,
                       EmEffect001EB440Fault *fault)
{
    return pair(h, a0, a1, fault, 1, 0x00255B30u, 0x00255BC0u);
}
