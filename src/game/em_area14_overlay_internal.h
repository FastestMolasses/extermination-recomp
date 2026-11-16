/* Memory and fail-stop plumbing of em_area14_overlay (the design of
 * em_area00s2_creature_internal.h). */
#ifndef EM_AREA14_OVERLAY_INTERNAL_H
#define EM_AREA14_OVERLAY_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area14_overlay.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea14OverlayHooks *h;
    EmArea14OverlayFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A14;

static inline int a14_latch(A14 *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA14_OVERLAY_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a14_failed(const A14 *o) { return o->fault->code != EM_AREA14_OVERLAY_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a14_at(A14 *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a14_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a14_latch(o, address, EM_AREA14_OVERLAY_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a14_u8(A14 *o, uint32_t a) { return *a14_at(o, a, 1); }
static inline uint32_t a14_u16(A14 *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a14_at(o, a, 2), 2);
    return v;
}
static inline int32_t a14_s16(A14 *o, uint32_t a) { return (int16_t)a14_u16(o, a); }
static inline uint32_t a14_u32(A14 *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a14_at(o, a, 4), 4);
    return v;
}
static inline int32_t a14_s32(A14 *o, uint32_t a) { return (int32_t)a14_u32(o, a); }
static inline int32_t a14_s8(A14 *o, uint32_t a) { return (int8_t)a14_u8(o, a); }
static inline void a14_w8(A14 *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a14_at(o, a, 1), &b, 1);
}
static inline void a14_w16(A14 *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a14_at(o, a, 2), &h, 2);
}
static inline void a14_w32(A14 *o, uint32_t a, uint32_t v) { memcpy(a14_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a14_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a14_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a14_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A14_ADD(a, b) em_ee_add_bits((a), (b))
#define A14_SUB(a, b) em_ee_sub_bits((a), (b))
#define A14_MUL(a, b) em_ee_mul_bits((a), (b))
#define A14_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A14_LE(a, b) em_ee_c_le_bits((a), (b))
#define A14_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A14_DIV(a, b) em_ee_div_bits((a), (b))
#define A14_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A14_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a14_u64(A14 *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a14_at(o, a, 8), 8);
    return v;
}
static inline void a14_w64(A14 *o, uint32_t a, uint64_t v) { memcpy(a14_at(o, a, 8), &v, 8); }

static inline int a14_begin(A14 *o, const EmArea14OverlayHooks *h, EmArea14OverlayFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a14_failed(o) ? -1 : 0;
}

static inline int a14_end(const A14 *o) { return a14_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a14_method(A14 *o, uint32_t self)
{
    if (a14_failed(o)) return -1;
    const uint32_t fn = a14_u32(o, self + 0x4C);
    if (a14_failed(o)) return -1;
    if (!o->h->w_callback) return a14_latch(o, fn, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a14_latch(o, fn, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}


/* BEGIN GENERATED WRAPPERS */
static inline int a14_c_001028D0(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001028D0) return a14_latch(o, 0x001028D0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001028D0(o->h->ctx, a0, a1, a2) < 0) return a14_latch(o, 0x001028D0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00102918(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00102918) return a14_latch(o, 0x00102918u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102918(o->h->ctx, a0, a1, a2) < 0) return a14_latch(o, 0x00102918u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00102948(A14 *o, uint32_t a0, uint32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00102948) return a14_latch(o, 0x00102948u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x00102948u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001029C0(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001029C0) return a14_latch(o, 0x001029C0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a14_latch(o, 0x001029C0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00102C58(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00102C58) return a14_latch(o, 0x00102C58u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102C58(o->h->ctx, a0, a1, a2) < 0) return a14_latch(o, 0x00102C58u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_0011DE90(A14 *o, uint32_t a0, uint32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0011DE90) return a14_latch(o, 0x0011DE90u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DE90(o->h->ctx, a14_float(a0), &r) < 0) return a14_latch(o, 0x0011DE90u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    *out = a14_bits(r);
    return 0;
}

static inline int a14_c_0011DF78(A14 *o, uint32_t a0, uint32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0011DF78) return a14_latch(o, 0x0011DF78u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DF78(o->h->ctx, a14_float(a0), &r) < 0) return a14_latch(o, 0x0011DF78u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    *out = a14_bits(r);
    return 0;
}

static inline int a14_c_0011E2A8(A14 *o, uint32_t a0, uint32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0011E2A8) return a14_latch(o, 0x0011E2A8u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E2A8(o->h->ctx, a14_float(a0), &r) < 0) return a14_latch(o, 0x0011E2A8u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    *out = a14_bits(r);
    return 0;
}

static inline int a14_c_00122BB8(A14 *o, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a14_latch(o, 0x00122BB8u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a14_latch(o, 0x00122BB8u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00158590(A14 *o, uint32_t a0, int32_t a1, int32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00158590) return a14_latch(o, 0x00158590u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00158590(o->h->ctx, a0, a1, a2) < 0) return a14_latch(o, 0x00158590u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00183010(A14 *o, uint32_t a0, uint32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00183010) return a14_latch(o, 0x00183010u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00183010(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x00183010u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00183160(A14 *o, int32_t a0, uint32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00183160) return a14_latch(o, 0x00183160u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00183160(o->h->ctx, a0, a14_float(a1)) < 0) return a14_latch(o, 0x00183160u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_0018C4B0(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0018C4B0) return a14_latch(o, 0x0018C4B0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0018C4B0(o->h->ctx, a0, a14_float(a1), a14_float(a2)) < 0) return a14_latch(o, 0x0018C4B0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_0018C6A0(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0018C6A0) return a14_latch(o, 0x0018C6A0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0018C6A0(o->h->ctx, a0, a1, a14_float(a2)) < 0) return a14_latch(o, 0x0018C6A0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_0018D7B0(A14 *o, uint32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0018D7B0) return a14_latch(o, 0x0018D7B0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0018D7B0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x0018D7B0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00191120(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00191120) return a14_latch(o, 0x00191120u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_00191120(o->h->ctx, a14_float(a0), a14_float(a1), a14_float(a2), a14_float(a3), &r) < 0) return a14_latch(o, 0x00191120u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    *out = a14_bits(r);
    return 0;
}

static inline int a14_c_00191D40(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00191D40) return a14_latch(o, 0x00191D40u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00191D40(o->h->ctx, a0, a14_float(a1), a14_float(a2)) < 0) return a14_latch(o, 0x00191D40u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_00192010(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_00192010) return a14_latch(o, 0x00192010u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00192010(o->h->ctx, a0, a14_float(a1), a14_float(a2), a14_float(a3)) < 0) return a14_latch(o, 0x00192010u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001AEDE0(A14 *o, int32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001AEDE0) return a14_latch(o, 0x001AEDE0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AEDE0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001AEDE0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001AFC10(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a14_latch(o, 0x001AFC10u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a14_latch(o, 0x001AFC10u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001B0C60(A14 *o, int32_t a0, int32_t a1, int32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B0C60) return a14_latch(o, 0x001B0C60u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0C60(o->h->ctx, a0, a1, a2) < 0) return a14_latch(o, 0x001B0C60u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001B0FD0(A14 *o, uint32_t a0, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a14_latch(o, 0x001B0FD0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a14_latch(o, 0x001B0FD0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001B10B0(A14 *o, uint32_t a0, int32_t a1, int32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a14_latch(o, 0x001B10B0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2) < 0) return a14_latch(o, 0x001B10B0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001B12B0(A14 *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B12B0) return a14_latch(o, 0x001B12B0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B12B0(o->h->ctx, a14_float(a0), a14_float(a1), a14_float(a2), &r) < 0) return a14_latch(o, 0x001B12B0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    *out = a14_bits(r);
    return 0;
}

static inline int a14_c_001B1470(A14 *o, uint32_t a0, uint32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B1470) return a14_latch(o, 0x001B1470u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a14_float(a0), &r) < 0) return a14_latch(o, 0x001B1470u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    *out = a14_bits(r);
    return 0;
}

static inline int a14_c_001B17A0(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a14_latch(o, 0x001B17A0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0) < 0) return a14_latch(o, 0x001B17A0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001B1EA0(A14 *o, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B1EA0) return a14_latch(o, 0x001B1EA0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1EA0(o->h->ctx, a0, a1, a2, a3, out) < 0) return a14_latch(o, 0x001B1EA0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001B6660(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001B6660) return a14_latch(o, 0x001B6660u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B6660(o->h->ctx, a0) < 0) return a14_latch(o, 0x001B6660u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001BA1A0(A14 *o, uint32_t a0, uint32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001BA1A0) return a14_latch(o, 0x001BA1A0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1A0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001BA1A0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001BA1C0(A14 *o, uint32_t a0, int32_t a1, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001BA1C0) return a14_latch(o, 0x001BA1C0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1C0(o->h->ctx, a0, a1, out) < 0) return a14_latch(o, 0x001BA1C0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001BA1F0(A14 *o, uint32_t a0, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001BA1F0) return a14_latch(o, 0x001BA1F0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1F0(o->h->ctx, a0, out) < 0) return a14_latch(o, 0x001BA1F0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001BA540(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001BA540) return a14_latch(o, 0x001BA540u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA540(o->h->ctx, a0) < 0) return a14_latch(o, 0x001BA540u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001BA580(A14 *o, uint32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001BA580) return a14_latch(o, 0x001BA580u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA580(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001BA580u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001BA8E0(A14 *o, uint32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001BA8E0) return a14_latch(o, 0x001BA8E0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA8E0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001BA8E0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C4760(A14 *o, int32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C4760) return a14_latch(o, 0x001C4760u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C4760(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001C4760u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C47A0(A14 *o, int32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C47A0) return a14_latch(o, 0x001C47A0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C47A0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001C47A0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C6380(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C6380) return a14_latch(o, 0x001C6380u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a14_latch(o, 0x001C6380u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C63E0(A14 *o, uint32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a14_latch(o, 0x001C63E0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001C63E0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C64F0(A14 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a14_latch(o, 0x001C64F0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a14_float(a1), out) < 0) return a14_latch(o, 0x001C64F0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C67E0(A14 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C67E0) return a14_latch(o, 0x001C67E0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C67E0(o->h->ctx, a0, a1, a14_float(a2), a14_float(a3)) < 0) return a14_latch(o, 0x001C67E0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001C68C0(A14 *o, uint32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a14_latch(o, 0x001C68C0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a14_latch(o, 0x001C68C0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001CA6F0(A14 *o, uint32_t a0, int32_t a1)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001CA6F0) return a14_latch(o, 0x001CA6F0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA6F0(o->h->ctx, a0, a1) < 0) return a14_latch(o, 0x001CA6F0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001CCF70(A14 *o, uint32_t a0, int32_t *out)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001CCF70) return a14_latch(o, 0x001CCF70u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CCF70(o->h->ctx, a0, out) < 0) return a14_latch(o, 0x001CCF70u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001CFB50(A14 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001CFB50) return a14_latch(o, 0x001CFB50u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFB50(o->h->ctx, a0, a1, a2, a14_float(a3), a14_float(a4), a14_float(a5), a14_float(a6), a14_float(a7)) < 0) return a14_latch(o, 0x001CFB50u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001CFBE0(A14 *o, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001CFBE0) return a14_latch(o, 0x001CFBE0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFBE0(o->h->ctx, a0, a1, a2, a3, a4) < 0) return a14_latch(o, 0x001CFBE0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001F6B00(A14 *o)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001F6B00) return a14_latch(o, 0x001F6B00u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F6B00(o->h->ctx) < 0) return a14_latch(o, 0x001F6B00u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001FAE70(A14 *o, int32_t a0)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001FAE70) return a14_latch(o, 0x001FAE70u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FAE70(o->h->ctx, a0) < 0) return a14_latch(o, 0x001FAE70u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_001FB9F0(A14 *o, int32_t a0, int32_t a1, int32_t a2, int32_t a3)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_001FB9F0) return a14_latch(o, 0x001FB9F0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FB9F0(o->h->ctx, a0, a1, a2, a3) < 0) return a14_latch(o, 0x001FB9F0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a14_c_0021B9A0(A14 *o, int32_t a0, uint32_t a1, uint32_t a2)
{
    if (a14_failed(o)) return -1;
    if (!o->h->w_0021B9A0) return a14_latch(o, 0x0021B9A0u, EM_AREA14_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0021B9A0(o->h->ctx, a0, a14_float(a1), a14_float(a2)) < 0) return a14_latch(o, 0x0021B9A0u, EM_AREA14_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
