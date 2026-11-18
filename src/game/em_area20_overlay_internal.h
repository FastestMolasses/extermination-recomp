/* Memory and fail-stop plumbing of em_area20_overlay (the design of
 * em_area00s2_creature_internal.h). */
#ifndef EM_AREA20_OVERLAY_INTERNAL_H
#define EM_AREA20_OVERLAY_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area20_overlay.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea20OverlayHooks *h;
    EmArea20OverlayFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A20;

static inline int a20_latch(A20 *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA20_OVERLAY_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a20_failed(const A20 *o) { return o->fault->code != EM_AREA20_OVERLAY_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a20_at(A20 *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a20_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a20_latch(o, address, EM_AREA20_OVERLAY_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a20_u8(A20 *o, uint32_t a) { return *a20_at(o, a, 1); }
static inline uint32_t a20_u16(A20 *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a20_at(o, a, 2), 2);
    return v;
}
static inline int32_t a20_s16(A20 *o, uint32_t a) { return (int16_t)a20_u16(o, a); }
static inline uint32_t a20_u32(A20 *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a20_at(o, a, 4), 4);
    return v;
}
static inline int32_t a20_s32(A20 *o, uint32_t a) { return (int32_t)a20_u32(o, a); }
static inline int32_t a20_s8(A20 *o, uint32_t a) { return (int8_t)a20_u8(o, a); }
static inline void a20_w8(A20 *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a20_at(o, a, 1), &b, 1);
}
static inline void a20_w16(A20 *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a20_at(o, a, 2), &h, 2);
}
static inline void a20_w32(A20 *o, uint32_t a, uint32_t v) { memcpy(a20_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a20_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a20_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a20_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A20_ADD(a, b) em_ee_add_bits((a), (b))
#define A20_SUB(a, b) em_ee_sub_bits((a), (b))
#define A20_MUL(a, b) em_ee_mul_bits((a), (b))
#define A20_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A20_LE(a, b) em_ee_c_le_bits((a), (b))
#define A20_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A20_DIV(a, b) em_ee_div_bits((a), (b))
#define A20_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A20_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a20_u64(A20 *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a20_at(o, a, 8), 8);
    return v;
}
static inline void a20_w64(A20 *o, uint32_t a, uint64_t v) { memcpy(a20_at(o, a, 8), &v, 8); }

static inline int a20_begin(A20 *o, const EmArea20OverlayHooks *h, EmArea20OverlayFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a20_failed(o) ? -1 : 0;
}

static inline int a20_end(const A20 *o) { return a20_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a20_method(A20 *o, uint32_t self)
{
    if (a20_failed(o)) return -1;
    const uint32_t fn = a20_u32(o, self + 0x4C);
    if (a20_failed(o)) return -1;
    if (!o->h->w_callback) return a20_latch(o, fn, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a20_latch(o, fn, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}


/* BEGIN GENERATED WRAPPERS */
static inline int a20_c_00102918(A20 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_00102918) return a20_latch(o, 0x00102918u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102918(o->h->ctx, a0, a1, a2) < 0) return a20_latch(o, 0x00102918u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_00102948(A20 *o, uint32_t a0, uint32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_00102948) return a20_latch(o, 0x00102948u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x00102948u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001029C0(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001029C0) return a20_latch(o, 0x001029C0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a20_latch(o, 0x001029C0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_00102B08(A20 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_00102B08) return a20_latch(o, 0x00102B08u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102B08(o->h->ctx, a0, a1, a20_float(a2)) < 0) return a20_latch(o, 0x00102B08u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_00102C58(A20 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_00102C58) return a20_latch(o, 0x00102C58u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102C58(o->h->ctx, a0, a1, a2) < 0) return a20_latch(o, 0x00102C58u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_0011DE90(A20 *o, uint32_t a0, uint32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_0011DE90) return a20_latch(o, 0x0011DE90u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DE90(o->h->ctx, a20_float(a0), &r) < 0) return a20_latch(o, 0x0011DE90u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    *out = a20_bits(r);
    return 0;
}

static inline int a20_c_0011E2A8(A20 *o, uint32_t a0, uint32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_0011E2A8) return a20_latch(o, 0x0011E2A8u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E2A8(o->h->ctx, a20_float(a0), &r) < 0) return a20_latch(o, 0x0011E2A8u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    *out = a20_bits(r);
    return 0;
}

static inline int a20_c_00122BB8(A20 *o, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a20_latch(o, 0x00122BB8u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a20_latch(o, 0x00122BB8u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_0019A570(A20 *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_0019A570) return a20_latch(o, 0x0019A570u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019A570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a20_latch(o, 0x0019A570u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_0019AB20(A20 *o, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_0019AB20) return a20_latch(o, 0x0019AB20u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019AB20(o->h->ctx, a0, a1, a2, a3, out) < 0) return a20_latch(o, 0x0019AB20u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_0019AD00(A20 *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_0019AD00) return a20_latch(o, 0x0019AD00u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019AD00(o->h->ctx, a0, a1, a2, out) < 0) return a20_latch(o, 0x0019AD00u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001A2370(A20 *o, uint32_t a0, uint32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001A2370) return a20_latch(o, 0x001A2370u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001A2370(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001A2370u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001AFC10(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a20_latch(o, 0x001AFC10u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a20_latch(o, 0x001AFC10u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B0C60(A20 *o, int32_t a0, int32_t a1, int32_t a2)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B0C60) return a20_latch(o, 0x001B0C60u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0C60(o->h->ctx, a0, a1, a2) < 0) return a20_latch(o, 0x001B0C60u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B0D80(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B0D80) return a20_latch(o, 0x001B0D80u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0D80(o->h->ctx, a0) < 0) return a20_latch(o, 0x001B0D80u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B0FD0(A20 *o, uint32_t a0, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a20_latch(o, 0x001B0FD0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a20_latch(o, 0x001B0FD0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B10B0(A20 *o, uint32_t a0, int32_t a1, int32_t a2)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a20_latch(o, 0x001B10B0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2) < 0) return a20_latch(o, 0x001B10B0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B1470(A20 *o, uint32_t a0, uint32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B1470) return a20_latch(o, 0x001B1470u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a20_float(a0), &r) < 0) return a20_latch(o, 0x001B1470u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    *out = a20_bits(r);
    return 0;
}

static inline int a20_c_001B17A0(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a20_latch(o, 0x001B17A0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0) < 0) return a20_latch(o, 0x001B17A0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B1B70(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B1B70) return a20_latch(o, 0x001B1B70u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1B70(o->h->ctx, a0) < 0) return a20_latch(o, 0x001B1B70u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001B1EA0(A20 *o, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001B1EA0) return a20_latch(o, 0x001B1EA0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1EA0(o->h->ctx, a0, a1, a2, a3, out) < 0) return a20_latch(o, 0x001B1EA0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001BA1A0(A20 *o, uint32_t a0, uint32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001BA1A0) return a20_latch(o, 0x001BA1A0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1A0(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001BA1A0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001BA1C0(A20 *o, uint32_t a0, int32_t a1, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001BA1C0) return a20_latch(o, 0x001BA1C0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1C0(o->h->ctx, a0, a1, out) < 0) return a20_latch(o, 0x001BA1C0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001BA1F0(A20 *o, uint32_t a0, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001BA1F0) return a20_latch(o, 0x001BA1F0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1F0(o->h->ctx, a0, out) < 0) return a20_latch(o, 0x001BA1F0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001BA540(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001BA540) return a20_latch(o, 0x001BA540u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA540(o->h->ctx, a0) < 0) return a20_latch(o, 0x001BA540u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001BA580(A20 *o, uint32_t a0, int32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001BA580) return a20_latch(o, 0x001BA580u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA580(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001BA580u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001BA8E0(A20 *o, uint32_t a0, int32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001BA8E0) return a20_latch(o, 0x001BA8E0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA8E0(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001BA8E0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001C6190(A20 *o, uint32_t a0, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001C6190) return a20_latch(o, 0x001C6190u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6190(o->h->ctx, a0, out) < 0) return a20_latch(o, 0x001C6190u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001C6380(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001C6380) return a20_latch(o, 0x001C6380u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a20_latch(o, 0x001C6380u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001C63E0(A20 *o, uint32_t a0, int32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a20_latch(o, 0x001C63E0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001C63E0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001C64F0(A20 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a20_latch(o, 0x001C64F0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a20_float(a1), out) < 0) return a20_latch(o, 0x001C64F0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001C68C0(A20 *o, uint32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a20_latch(o, 0x001C68C0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a20_latch(o, 0x001C68C0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001CA6F0(A20 *o, uint32_t a0, int32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001CA6F0) return a20_latch(o, 0x001CA6F0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA6F0(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001CA6F0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001D04B0(A20 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001D04B0) return a20_latch(o, 0x001D04B0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D04B0(o->h->ctx, a0, a1, a2, a20_float(a3), a20_float(a4)) < 0) return a20_latch(o, 0x001D04B0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001EFD20(A20 *o, int32_t a0, uint32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001EFD20) return a20_latch(o, 0x001EFD20u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFD20(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001EFD20u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001EFEB0(A20 *o, int32_t a0, uint32_t a1, uint32_t *out)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001EFEB0) return a20_latch(o, 0x001EFEB0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFEB0(o->h->ctx, a0, a1, out) < 0) return a20_latch(o, 0x001EFEB0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001F0460(A20 *o, int32_t a0, uint32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001F0460) return a20_latch(o, 0x001F0460u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F0460(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001F0460u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001F9100(A20 *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001F9100) return a20_latch(o, 0x001F9100u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F9100(o->h->ctx, a0, a1, a2, a20_float(a3)) < 0) return a20_latch(o, 0x001F9100u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001FABB0(A20 *o)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001FABB0) return a20_latch(o, 0x001FABB0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FABB0(o->h->ctx) < 0) return a20_latch(o, 0x001FABB0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001FB0B0(A20 *o, int32_t a0)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001FB0B0) return a20_latch(o, 0x001FB0B0u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FB0B0(o->h->ctx, a0) < 0) return a20_latch(o, 0x001FB0B0u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a20_c_001FC580(A20 *o, uint32_t a0, int32_t a1)
{
    if (a20_failed(o)) return -1;
    if (!o->h->w_001FC580) return a20_latch(o, 0x001FC580u, EM_AREA20_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FC580(o->h->ctx, a0, a1) < 0) return a20_latch(o, 0x001FC580u, EM_AREA20_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
