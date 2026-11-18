/* Memory and fail-stop plumbing of em_area17_overlay (the design of
 * em_area00s2_creature_internal.h). */
#ifndef EM_AREA17_OVERLAY_INTERNAL_H
#define EM_AREA17_OVERLAY_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area17_overlay.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea17OverlayHooks *h;
    EmArea17OverlayFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A17;

static inline int a17_latch(A17 *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA17_OVERLAY_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a17_failed(const A17 *o) { return o->fault->code != EM_AREA17_OVERLAY_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a17_at(A17 *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a17_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a17_latch(o, address, EM_AREA17_OVERLAY_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a17_u8(A17 *o, uint32_t a) { return *a17_at(o, a, 1); }
static inline uint32_t a17_u16(A17 *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a17_at(o, a, 2), 2);
    return v;
}
static inline int32_t a17_s16(A17 *o, uint32_t a) { return (int16_t)a17_u16(o, a); }
static inline uint32_t a17_u32(A17 *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a17_at(o, a, 4), 4);
    return v;
}
static inline int32_t a17_s32(A17 *o, uint32_t a) { return (int32_t)a17_u32(o, a); }
static inline int32_t a17_s8(A17 *o, uint32_t a) { return (int8_t)a17_u8(o, a); }
static inline void a17_w8(A17 *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a17_at(o, a, 1), &b, 1);
}
static inline void a17_w16(A17 *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a17_at(o, a, 2), &h, 2);
}
static inline void a17_w32(A17 *o, uint32_t a, uint32_t v) { memcpy(a17_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a17_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a17_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a17_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A17_ADD(a, b) em_ee_add_bits((a), (b))
#define A17_SUB(a, b) em_ee_sub_bits((a), (b))
#define A17_MUL(a, b) em_ee_mul_bits((a), (b))
#define A17_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A17_LE(a, b) em_ee_c_le_bits((a), (b))
#define A17_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A17_DIV(a, b) em_ee_div_bits((a), (b))
#define A17_MADD(acc, a, b) em_ee_madd_bits((acc), (a), (b))
#define A17_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A17_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a17_u64(A17 *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a17_at(o, a, 8), 8);
    return v;
}
static inline void a17_w64(A17 *o, uint32_t a, uint64_t v) { memcpy(a17_at(o, a, 8), &v, 8); }

static inline int a17_begin(A17 *o, const EmArea17OverlayHooks *h, EmArea17OverlayFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a17_failed(o) ? -1 : 0;
}

static inline int a17_end(const A17 *o) { return a17_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a17_method(A17 *o, uint32_t self)
{
    if (a17_failed(o)) return -1;
    const uint32_t fn = a17_u32(o, self + 0x4C);
    if (a17_failed(o)) return -1;
    if (!o->h->w_callback) return a17_latch(o, fn, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a17_latch(o, fn, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}


/* BEGIN GENERATED WRAPPERS */
static inline int a17_c_001026A0(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001026A0) return a17_latch(o, 0x001026A0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a17_latch(o, 0x001026A0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_00102738(A17 *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_00102738) return a17_latch(o, 0x00102738u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_00102738(o->h->ctx, a0, a1, &r) < 0) return a17_latch(o, 0x00102738u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    *out = a17_bits(r);
    return 0;
}

static inline int a17_c_001028D0(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001028D0) return a17_latch(o, 0x001028D0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001028D0(o->h->ctx, a0, a1, a2) < 0) return a17_latch(o, 0x001028D0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_00102918(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_00102918) return a17_latch(o, 0x00102918u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102918(o->h->ctx, a0, a1, a2) < 0) return a17_latch(o, 0x00102918u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_00102948(A17 *o, uint32_t a0, uint32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_00102948) return a17_latch(o, 0x00102948u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x00102948u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001029C0(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001029C0) return a17_latch(o, 0x001029C0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a17_latch(o, 0x001029C0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_00102B08(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_00102B08) return a17_latch(o, 0x00102B08u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102B08(o->h->ctx, a0, a1, a17_float(a2)) < 0) return a17_latch(o, 0x00102B08u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_0011DE90(A17 *o, uint32_t a0, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0011DE90) return a17_latch(o, 0x0011DE90u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DE90(o->h->ctx, a17_float(a0), &r) < 0) return a17_latch(o, 0x0011DE90u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    *out = a17_bits(r);
    return 0;
}

static inline int a17_c_0011E2A8(A17 *o, uint32_t a0, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0011E2A8) return a17_latch(o, 0x0011E2A8u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E2A8(o->h->ctx, a17_float(a0), &r) < 0) return a17_latch(o, 0x0011E2A8u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    *out = a17_bits(r);
    return 0;
}

static inline int a17_c_0011E748(A17 *o, uint32_t a0, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0011E748) return a17_latch(o, 0x0011E748u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E748(o->h->ctx, a17_float(a0), &r) < 0) return a17_latch(o, 0x0011E748u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    *out = a17_bits(r);
    return 0;
}

static inline int a17_c_00122BB8(A17 *o, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a17_latch(o, 0x00122BB8u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a17_latch(o, 0x00122BB8u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_0019A570(A17 *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0019A570) return a17_latch(o, 0x0019A570u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019A570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a17_latch(o, 0x0019A570u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_0019AB20(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0019AB20) return a17_latch(o, 0x0019AB20u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019AB20(o->h->ctx, a0, a1, a2, a3, out) < 0) return a17_latch(o, 0x0019AB20u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_0019AD00(A17 *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0019AD00) return a17_latch(o, 0x0019AD00u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019AD00(o->h->ctx, a0, a1, a2, out) < 0) return a17_latch(o, 0x0019AD00u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_0019C6F0(A17 *o, int32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_0019C6F0) return a17_latch(o, 0x0019C6F0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019C6F0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x0019C6F0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001A2370(A17 *o, uint32_t a0, uint32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001A2370) return a17_latch(o, 0x001A2370u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001A2370(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001A2370u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001AEDE0(A17 *o, int32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001AEDE0) return a17_latch(o, 0x001AEDE0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AEDE0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001AEDE0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001AEE10(A17 *o, int32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001AEE10) return a17_latch(o, 0x001AEE10u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AEE10(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001AEE10u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001AFC10(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a17_latch(o, 0x001AFC10u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a17_latch(o, 0x001AFC10u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B0C60(A17 *o, int32_t a0, int32_t a1, int32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B0C60) return a17_latch(o, 0x001B0C60u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0C60(o->h->ctx, a0, a1, a2) < 0) return a17_latch(o, 0x001B0C60u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B0D80(A17 *o, uint32_t a0, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B0D80) return a17_latch(o, 0x001B0D80u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0D80(o->h->ctx, a0, out) < 0) return a17_latch(o, 0x001B0D80u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B0FD0(A17 *o, uint32_t a0, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a17_latch(o, 0x001B0FD0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a17_latch(o, 0x001B0FD0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B1020(A17 *o, uint32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B1020) return a17_latch(o, 0x001B1020u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1020(o->h->ctx, a0, a1, a2, a3, out) < 0) return a17_latch(o, 0x001B1020u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B10B0(A17 *o, uint32_t a0, int32_t a1, int32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a17_latch(o, 0x001B10B0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2) < 0) return a17_latch(o, 0x001B10B0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B1470(A17 *o, uint32_t a0, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B1470) return a17_latch(o, 0x001B1470u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a17_float(a0), &r) < 0) return a17_latch(o, 0x001B1470u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    *out = a17_bits(r);
    return 0;
}

static inline int a17_c_001B17A0(A17 *o, uint32_t a0, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a17_latch(o, 0x001B17A0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0, out) < 0) return a17_latch(o, 0x001B17A0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B1B70(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B1B70) return a17_latch(o, 0x001B1B70u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1B70(o->h->ctx, a0) < 0) return a17_latch(o, 0x001B1B70u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B1D20(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B1D20) return a17_latch(o, 0x001B1D20u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1D20(o->h->ctx, a0) < 0) return a17_latch(o, 0x001B1D20u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B1EA0(A17 *o, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B1EA0) return a17_latch(o, 0x001B1EA0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1EA0(o->h->ctx, a0, a1, a2, a3, out) < 0) return a17_latch(o, 0x001B1EA0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001B6660(A17 *o, uint32_t a0, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001B6660) return a17_latch(o, 0x001B6660u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B6660(o->h->ctx, a0, out) < 0) return a17_latch(o, 0x001B6660u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BA1A0(A17 *o, uint32_t a0, uint32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BA1A0) return a17_latch(o, 0x001BA1A0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1A0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001BA1A0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BA1C0(A17 *o, uint32_t a0, int32_t a1, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BA1C0) return a17_latch(o, 0x001BA1C0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1C0(o->h->ctx, a0, a1, out) < 0) return a17_latch(o, 0x001BA1C0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BA1F0(A17 *o, uint32_t a0, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BA1F0) return a17_latch(o, 0x001BA1F0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1F0(o->h->ctx, a0, out) < 0) return a17_latch(o, 0x001BA1F0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BA540(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BA540) return a17_latch(o, 0x001BA540u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA540(o->h->ctx, a0) < 0) return a17_latch(o, 0x001BA540u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BA580(A17 *o, uint32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BA580) return a17_latch(o, 0x001BA580u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA580(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001BA580u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BA8E0(A17 *o, uint32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BA8E0) return a17_latch(o, 0x001BA8E0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA8E0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001BA8E0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001BAD40(A17 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001BAD40) return a17_latch(o, 0x001BAD40u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BAD40(o->h->ctx, a0, a1, out) < 0) return a17_latch(o, 0x001BAD40u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C4760(A17 *o, int32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C4760) return a17_latch(o, 0x001C4760u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C4760(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001C4760u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C47A0(A17 *o, int32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C47A0) return a17_latch(o, 0x001C47A0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C47A0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001C47A0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C47E0(A17 *o, int32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C47E0) return a17_latch(o, 0x001C47E0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C47E0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001C47E0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C5570(A17 *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C5570) return a17_latch(o, 0x001C5570u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C5570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a17_latch(o, 0x001C5570u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C5C90(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C5C90) return a17_latch(o, 0x001C5C90u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C5C90(o->h->ctx, a0) < 0) return a17_latch(o, 0x001C5C90u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C6120(A17 *o, int32_t a0, int32_t a1, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C6120) return a17_latch(o, 0x001C6120u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6120(o->h->ctx, a0, a1, out) < 0) return a17_latch(o, 0x001C6120u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C62C0(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C62C0) return a17_latch(o, 0x001C62C0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C62C0(o->h->ctx, a0) < 0) return a17_latch(o, 0x001C62C0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C6380(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C6380) return a17_latch(o, 0x001C6380u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a17_latch(o, 0x001C6380u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C63E0(A17 *o, uint32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a17_latch(o, 0x001C63E0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001C63E0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C64F0(A17 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a17_latch(o, 0x001C64F0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a17_float(a1), out) < 0) return a17_latch(o, 0x001C64F0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C67E0(A17 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C67E0) return a17_latch(o, 0x001C67E0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C67E0(o->h->ctx, a0, a1, a17_float(a2), a17_float(a3)) < 0) return a17_latch(o, 0x001C67E0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001C68C0(A17 *o, uint32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a17_latch(o, 0x001C68C0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a17_latch(o, 0x001C68C0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001CA6E0(A17 *o, uint32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001CA6E0) return a17_latch(o, 0x001CA6E0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA6E0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001CA6E0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001CA6F0(A17 *o, uint32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001CA6F0) return a17_latch(o, 0x001CA6F0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA6F0(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001CA6F0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001CCF70(A17 *o, uint32_t a0, int32_t *out)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001CCF70) return a17_latch(o, 0x001CCF70u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CCF70(o->h->ctx, a0, out) < 0) return a17_latch(o, 0x001CCF70u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001CFB50(A17 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001CFB50) return a17_latch(o, 0x001CFB50u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFB50(o->h->ctx, a0, a1, a2, a17_float(a3), a17_float(a4), a17_float(a5), a17_float(a6), a17_float(a7)) < 0) return a17_latch(o, 0x001CFB50u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001CFBE0(A17 *o, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001CFBE0) return a17_latch(o, 0x001CFBE0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFBE0(o->h->ctx, a0, a1, a2, a3, a4) < 0) return a17_latch(o, 0x001CFBE0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001D04B0(A17 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001D04B0) return a17_latch(o, 0x001D04B0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D04B0(o->h->ctx, a0, a1, a2, a17_float(a3), a17_float(a4)) < 0) return a17_latch(o, 0x001D04B0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001EFD20(A17 *o, int32_t a0, uint32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001EFD20) return a17_latch(o, 0x001EFD20u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFD20(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001EFD20u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001F0460(A17 *o, int32_t a0, uint32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001F0460) return a17_latch(o, 0x001F0460u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F0460(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001F0460u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001F4E20(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001F4E20) return a17_latch(o, 0x001F4E20u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F4E20(o->h->ctx, a0, a1, a17_float(a2)) < 0) return a17_latch(o, 0x001F4E20u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001F5940(A17 *o, int32_t a0, uint32_t a1, int32_t a2)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001F5940) return a17_latch(o, 0x001F5940u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F5940(o->h->ctx, a0, a1, a2) < 0) return a17_latch(o, 0x001F5940u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001F6B60(A17 *o)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001F6B60) return a17_latch(o, 0x001F6B60u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F6B60(o->h->ctx) < 0) return a17_latch(o, 0x001F6B60u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001F9100(A17 *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001F9100) return a17_latch(o, 0x001F9100u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F9100(o->h->ctx, a0, a1, a2, a17_float(a3)) < 0) return a17_latch(o, 0x001F9100u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001FAE70(A17 *o, int32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001FAE70) return a17_latch(o, 0x001FAE70u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FAE70(o->h->ctx, a0) < 0) return a17_latch(o, 0x001FAE70u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001FB0B0(A17 *o, int32_t a0)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001FB0B0) return a17_latch(o, 0x001FB0B0u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FB0B0(o->h->ctx, a0) < 0) return a17_latch(o, 0x001FB0B0u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001FBD50(A17 *o, uint32_t a0, int32_t a1, int32_t a2, uint32_t a3)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001FBD50) return a17_latch(o, 0x001FBD50u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FBD50(o->h->ctx, a0, a1, a2, a17_float(a3)) < 0) return a17_latch(o, 0x001FBD50u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a17_c_001FC580(A17 *o, uint32_t a0, int32_t a1)
{
    if (a17_failed(o)) return -1;
    if (!o->h->w_001FC580) return a17_latch(o, 0x001FC580u, EM_AREA17_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FC580(o->h->ctx, a0, a1) < 0) return a17_latch(o, 0x001FC580u, EM_AREA17_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
