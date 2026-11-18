/* em_effect_001E3630.c - see em_effect_001E3630.h. Every routine names its
 * original address; the comments say what the original does. */
#include "game/em_effect_001E3630.h"

#include <setjmp.h>
#include <stddef.h>
#include <string.h>

#include "game/em_ee_float.h"

#define F_ZERO 0x00000000u
#define F_ONE 0x3F800000u
#define F_TWO31 0x4F000000u     /* 2147483648.0 */
#define F_50 0x42480000u
#define F_30 0x41F00000u
#define F_M145 0xC3110000u
#define F_450 0x43E10000u
#define F_0_04 0x3D23D70Au
#define F_1_7 0x3FD9999Au
#define F_42_5 0x422A0001u      /* 42.500004 */
#define F_7 0x40E00000u
#define F_7_5 0x40F00000u
#define F_4 0x40800000u
#define F_M4 0xC0800000u
#define F_M2 0xC0000000u
#define F_9 0x41100000u
#define F_TENTH 0x3DCCCCCDu     /* 1.0 / (float)10 */

#define D_008102D0 0x008102D0u
#define S_31B0 0x700031B0u
#define S_31D0 0x700031D0u
#define S_36A0 0x700036A0u
#define S_38A0 0x700038A0u
#define D_00253AA0 0x00253AA0u
#define D_00253B30 0x00253B30u
#define D_00253BC0 0x00253BC0u
#define D_00257000 0x00257000u
#define D_00275C34 0x00275C34u
#define D_0081F8F0 0x0081F8F0u

typedef struct {
    const EmEffect001E3630Hooks *h;
    EmEffect001E3630Fault *fault;
    jmp_buf out;
} Run;

static void stop(Run *r, int32_t code, uint32_t address)
{
    if (r->fault && !r->fault->code) {
        r->fault->code = code;
        r->fault->address = address;
    }
    longjmp(r->out, 1);
}

static uint8_t *span(Run *r, uint32_t a, uint32_t n, int write)
{
    uint8_t *p = r->h->bytes ? r->h->bytes(r->h->ctx, a, n, write) : NULL;
    if (!p) stop(r, EM_EFFECT_001E3630_FAULT_UNMAPPED, a);
    return p;
}

static uint32_t w32(Run *r, uint32_t a)
{
    const uint8_t *p = span(r, a, 4, 0);
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t u8(Run *r, uint32_t a) { return *span(r, a, 1, 0); }

static void st32(Run *r, uint32_t a, uint32_t v)
{
    uint8_t *p = span(r, a, 4, 1);
    for (unsigned k = 0; k < 4; ++k) p[k] = (uint8_t)(v >> (8 * k));
}

static void st8(Run *r, uint32_t a, uint32_t v) { *span(r, a, 1, 1) = (uint8_t)v; }

/* A worker's status: NULL or failed faults at the callee. */
#define NEED(w, fn) do { if (!r->h->w) stop(r, EM_EFFECT_001E3630_FAULT_NULL_WORKER, (fn)); } while (0)
#define CALL(w, fn, ...) do {                                                       \
        NEED(w, fn);                                                                \
        if (r->h->w(r->h->ctx, __VA_ARGS__) < 0) stop(r, EM_EFFECT_001E3630_FAULT_WORKER, (fn)); \
    } while (0)
#define CALL0(w, fn) do {                                                           \
        NEED(w, fn);                                                                \
        if (r->h->w(r->h->ctx) < 0) stop(r, EM_EFFECT_001E3630_FAULT_WORKER, (fn)); \
    } while (0)

/* rand() / 2^31: the draw as a float, then the quotient. */
static uint32_t unit_draw(Run *r)
{
    int32_t v = 0;
    CALL(w_00122BB8, 0x00122BB8u, &v);
    return em_ee_div_bits(em_ee_cvt_s_w_bits((uint32_t)v), F_TWO31);
}

/* 001028D0(0x700038A0, 0x700031B0, w+0x50), then w+0x18 = sqrt of the
 * squared length (x*x and y*y summed into ACC, z*z added by MADD) / 50. */
static void flight_time(Run *r, uint32_t w)
{
    uint32_t x, y, z, d, root = 0;
    CALL(w_001028D0, 0x001028D0u, S_38A0, S_31B0, w + 0x50u);
    x = w32(r, S_38A0);
    y = w32(r, S_38A0 + 4u);
    z = w32(r, S_38A0 + 8u);
    d = em_ee_madd_bits(em_ee_adda_bits(em_ee_mul_bits(x, x), em_ee_mul_bits(y, y)), z, z);
    CALL(w_0011E748, 0x0011E748u, d, &root);
    st32(r, w + 0x18u, root);
    st32(r, w + 0x18u, em_ee_div_bits(w32(r, w + 0x18u), F_50));
}

static void state1(Run *r, uint32_t node, uint32_t w, uint32_t sp);

static void state0(Run *r, uint32_t node, uint32_t w, uint32_t sp)
{
    const uint32_t self = w32(r, node + 0x14u) + 0x1F0u;
    const uint32_t target = w32(r, D_008102D0);
    int32_t hit = 0, more = 0;
    CALL(w_00102948, 0x00102948u, node + 0xB0u, node + 0x100u);
    CALL(w_00102958, 0x00102958u, w + 0x20u, node + 0xD0u);
    st32(r, node + 0x20u, F_ZERO);
    st32(r, node + 0x30u, self);
    st32(r, node + 0x34u, EM_EFFECT_001E3620_CONTACT);
    st32(r, w + 0x14u, F_ZERO);
    st32(r, w + 0xA0u, unit_draw(r));
    st32(r, w + 0xA4u, unit_draw(r));
    st32(r, w + 0xA8u, 0);
    st8(r, node + 0u, 1);
    st8(r, node + 4u, 1);
    CALL(w_0019A570, 0x0019A570u, target + 0xA0u, node + 0x100u, 6, 0, &hit);
    CALL(w_0019B6C0, 0x0019B6C0u, target + 0xA0u, node + 0x100u, &more);
    if ((hit | more) != 0) {
        st32(r, w + 0x1Cu, 0);
        st32(r, w + 0x18u, F_ZERO);
        return;
    }
    st32(r, S_38A0, F_ZERO);
    st32(r, S_38A0 + 4u, F_ZERO);
    st32(r, S_38A0 + 8u, F_50);
    st32(r, S_38A0 + 12u, F_ONE);
    CALL(w_001026A0, 0x001026A0u, S_38A0, w + 0x20u, S_38A0);
    st32(r, w + 0x1Cu, 0);
    CALL(w_0019B6C0, 0x0019B6C0u, w + 0x50u, S_38A0, &hit);
    if (hit != 0 && u8(r, w32(r, S_31D0) + 0x1Au) == 0x5Bu) {
        /* a grabbed actor ahead: the flight time to the probe point */
        flight_time(r, w);
        st32(r, w + 0x1Cu, 0x63u);
    }
    if (w32(r, w + 0x1Cu) == 0) {
        CALL(w_0019A570, 0x0019A570u, w + 0x50u, S_38A0, 6, 0, &hit);
        st32(r, w + 0x1Cu, (uint32_t)hit);
        if (w32(r, w + 0x1Cu) != 0) {
            uint32_t probe;
            flight_time(r, w);
            probe = w32(r, S_31D0);
            st32(r, S_38A0, w32(r, probe + 0x24u));
            st32(r, S_38A0 + 4u, w32(r, probe + 0x28u));
            st32(r, S_38A0 + 8u, w32(r, probe + 0x2Cu));
            CALL(w_001CD390, 0x001CD390u, w + 0x60u, S_38A0);
            CALL(w_00102918, 0x00102918u, w + 0x60u, w + 0x60u, S_31B0);
        } else {
            st32(r, w + 0x18u, F_ONE);
        }
    }
    state1(r, node, w, sp);
}

/* The retry window (once, past 1.0): the two side probes from w+0x60. */
static void retry(Run *r, uint32_t w)
{
    static const uint32_t sides[2][3] = {{F_M4, F_M4, F_M2}, {F_4, F_4, F_M2}};
    int side;
    for (side = 0; side < 2; ++side) {
        int32_t hit = 0;
        st32(r, S_38A0, sides[side][0]);
        st32(r, S_38A0 + 4u, sides[side][1]);
        st32(r, S_38A0 + 8u, sides[side][2]);
        st32(r, S_38A0 + 12u, F_ONE);
        CALL(w_001026A0, 0x001026A0u, S_38A0, w + 0x60u, S_38A0);
        CALL(w_0019A570, 0x0019A570u, w + 0x50u, S_38A0, 4, 0, &hit);
        if (hit == 0) break;
    }
    if (side == 2) {
        CALL(w_001EFD20, 0x001EFD20u, 0x8000001Au, w + 0x90u);
        CALL(w_001F0460, 0x001F0460u, 6, w + 0x60u);
    }
}

static void state1(Run *r, uint32_t node, uint32_t w, uint32_t sp)
{
    const uint32_t block = sp - EM_EFFECT_001E3630_FRAME + EM_EFFECT_001E3630_BLOCK;
    int32_t key = 0;
    uint32_t t, q;
    CALL(w_001F0190, 0x001F0190u, F_M145, F_450);
    CALL(w_001CCF70, 0x001CCF70u, w + 0x50u, &key);
    st32(r, D_00253BC0, F_ONE);
    st32(r, D_00253BC0 + 4u, F_ONE);
    st32(r, D_00253BC0 + 8u, em_ee_mul_bits(F_30, w32(r, w + 0x18u)));
    st32(r, D_00253BC0 + 12u, F_ONE);
    CALL(w_001CFA60, 0x001CFA60u, block, w + 0x20u, w32(r, w + 0x14u), w32(r, w + 0xA4u));
    CALL(w_001CFBE0, 0x001CFBE0u, key, 0, D_00253BC0, block, 1);
    CALL(w_001CCF70, 0x001CCF70u, node + 0x100u, &key);
    CALL(w_001CFA60, 0x001CFA60u, block, node + 0xD0u, w32(r, w + 0x14u), w32(r, w + 0xA0u));
    CALL(w_001CFBE0, 0x001CFBE0u, key, 1, D_00253B30, block, 0);
    CALL(w_001CFBE0, 0x001CFBE0u, key, 1, D_00253AA0, block, 0);
    CALL0(w_001F0290, 0x001F0290u);

    t = em_ee_add_bits(w32(r, w + 0x14u), F_0_04);
    st32(r, w + 0x14u, t);
    if (!em_ee_c_le_bits(t, F_1_7)) {
        st8(r, node + 0u, 2);
        st8(r, node + 4u, 3);
        return;
    }
    if (!em_ee_c_le_bits(t, F_ONE) && w32(r, w + 0xA8u) == 0) {
        st32(r, w + 0xA8u, w32(r, w + 0xA8u) + 1u);
        if (!em_ee_c_eq_bits(F_ONE, w32(r, w + 0x18u)) && w32(r, w + 0x1Cu) == 4u) retry(r, w);
    }

    /* The step along the launch frame: 0x700036A0 = +0xD0 with its
     * translation row cleared, the delta (0, 0, 50 * w+0x18 / 42.500004). */
    CALL(w_00102958, 0x00102958u, S_36A0, node + 0xD0u);
    st32(r, S_36A0 + 0x30u, 0);
    st32(r, S_36A0 + 0x34u, 0);
    st32(r, S_36A0 + 0x38u, 0);
    st32(r, S_38A0, F_ZERO);
    st32(r, S_38A0 + 4u, F_ZERO);
    st32(r, S_36A0 + 0x3Cu, F_ONE);
    st32(r, S_38A0 + 8u, em_ee_div_bits(em_ee_mul_bits(F_50, w32(r, w + 0x18u)), F_42_5));
    st32(r, S_38A0 + 12u, F_ONE);
    CALL(w_001026A0, 0x001026A0u, S_38A0, S_36A0, S_38A0);
    for (unsigned k = 0; k < 3; ++k)
        st32(r, node + 0x100u + 4u * k, em_ee_add_bits(w32(r, node + 0x100u + 4u * k), w32(r, S_38A0 + 4u * k)));
    CALL(w_00102948, 0x00102948u, node + 0xB0u, node + 0x100u);

    /* The wobble: w+0 / +4 / +8 from +0x20 / 1.7. */
    q = em_ee_div_bits(w32(r, node + 0x20u), F_1_7);
    st32(r, w + 0u, em_ee_mul_bits(F_7, q));
    q = em_ee_div_bits(w32(r, node + 0x20u), F_1_7);
    st32(r, w + 4u, em_ee_mul_bits(F_7_5, q));
    q = em_ee_div_bits(w32(r, node + 0x20u), F_1_7);
    st32(r, w + 8u, em_ee_mul_bits(F_7, q));
    st32(r, node + 0x20u, em_ee_add_bits(w32(r, node + 0x20u), F_0_04));
    CALL(w_001B17A0, 0x001B17A0u, node);
}

int em_effect_001E3630(const EmEffect001E3630Hooks *h, uint32_t node, uint32_t sp, EmEffect001E3630Fault *fault)
{
    Run run;
    Run *r = &run;
    if (fault) {
        fault->code = 0;
        fault->address = 0;
    }
    if (!h) {
        if (fault) {
            fault->code = EM_EFFECT_001E3630_FAULT_NULL_WORKER;
            fault->address = EM_EFFECT_001E3630_CALLBACK;
        }
        return -1;
    }
    run.h = h;
    run.fault = fault;
    if (setjmp(run.out)) return -1;
    {
        const uint32_t w = node + 0x1F0u;
        switch (u8(r, node + 4u)) {
        case 0:
            state0(r, node, w, sp);
            break;
        case 1:
            state1(r, node, w, sp);
            break;
        case 2:
        case 3:
            CALL(w_001AFC10, 0x001AFC10u, node);
            break;
        default:
            break;
        }
    }
    return 0;
}

int em_effect_001E3620(void) { return 0; }

int em_effect_001ECE70(const EmEffect001E3630Hooks *h, uint32_t a0, int32_t a1, EmEffect001E3630Fault *fault)
{
    Run run;
    Run *r = &run;
    if (fault) {
        fault->code = 0;
        fault->address = 0;
    }
    if (!h) {
        if (fault) {
            fault->code = EM_EFFECT_001E3630_FAULT_NULL_WORKER;
            fault->address = 0x001ECE70u;
        }
        return -1;
    }
    run.h = h;
    run.fault = fault;
    if (setjmp(run.out)) return -1;
    {
        const uint32_t c = w32(r, D_00275C34);
        const uint32_t f[5] = {w32(r, c + 0x54u), w32(r, c + 0x5Cu), F_ONE, F_TENTH, F_9};
        CALL(w_001CFB50, 0x001CFB50u, D_0081F8F0, 0, a0, f);
        CALL(w_001CFBE0, 0x001CFBE0u, a1, 1, D_00257000, D_0081F8F0, 0);
    }
    return 0;
}
