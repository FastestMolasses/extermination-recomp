/* Memory and fail-stop plumbing of em_area16_overlay (the design of
 * em_area00s2_creature_internal.h). */
#ifndef EM_AREA16_OVERLAY_INTERNAL_H
#define EM_AREA16_OVERLAY_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area16_overlay.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea16OverlayHooks *h;
    EmArea16OverlayFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A16;

static inline int a16_latch(A16 *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA16_OVERLAY_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a16_failed(const A16 *o) { return o->fault->code != EM_AREA16_OVERLAY_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a16_at(A16 *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a16_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a16_latch(o, address, EM_AREA16_OVERLAY_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a16_u8(A16 *o, uint32_t a) { return *a16_at(o, a, 1); }
static inline uint32_t a16_u16(A16 *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a16_at(o, a, 2), 2);
    return v;
}
static inline int32_t a16_s16(A16 *o, uint32_t a) { return (int16_t)a16_u16(o, a); }
static inline uint32_t a16_u32(A16 *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a16_at(o, a, 4), 4);
    return v;
}
static inline int32_t a16_s32(A16 *o, uint32_t a) { return (int32_t)a16_u32(o, a); }
static inline int32_t a16_s8(A16 *o, uint32_t a) { return (int8_t)a16_u8(o, a); }
static inline void a16_w8(A16 *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a16_at(o, a, 1), &b, 1);
}
static inline void a16_w16(A16 *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a16_at(o, a, 2), &h, 2);
}
static inline void a16_w32(A16 *o, uint32_t a, uint32_t v) { memcpy(a16_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a16_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a16_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a16_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A16_ADD(a, b) em_ee_add_bits((a), (b))
#define A16_SUB(a, b) em_ee_sub_bits((a), (b))
#define A16_MUL(a, b) em_ee_mul_bits((a), (b))
#define A16_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A16_LE(a, b) em_ee_c_le_bits((a), (b))
#define A16_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A16_DIV(a, b) em_ee_div_bits((a), (b))
#define A16_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A16_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a16_u64(A16 *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a16_at(o, a, 8), 8);
    return v;
}
static inline void a16_w64(A16 *o, uint32_t a, uint64_t v) { memcpy(a16_at(o, a, 8), &v, 8); }

static inline int a16_begin(A16 *o, const EmArea16OverlayHooks *h, EmArea16OverlayFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a16_failed(o) ? -1 : 0;
}

static inline int a16_end(const A16 *o) { return a16_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a16_method(A16 *o, uint32_t self)
{
    if (a16_failed(o)) return -1;
    const uint32_t fn = a16_u32(o, self + 0x4C);
    if (a16_failed(o)) return -1;
    if (!o->h->w_callback) return a16_latch(o, fn, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a16_latch(o, fn, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}


/* BEGIN GENERATED WRAPPERS */
static inline int a16_c_001026A0(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001026A0) return a16_latch(o, 0x001026A0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x001026A0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001026D0(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001026D0) return a16_latch(o, 0x001026D0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001026D0(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x001026D0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00102918(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00102918) return a16_latch(o, 0x00102918u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102918(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x00102918u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00102948(A16 *o, uint32_t a0, uint32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00102948) return a16_latch(o, 0x00102948u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x00102948u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00102958(A16 *o, uint32_t a0, uint32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00102958) return a16_latch(o, 0x00102958u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102958(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x00102958u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001029C0(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001029C0) return a16_latch(o, 0x001029C0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a16_latch(o, 0x001029C0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00102B08(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00102B08) return a16_latch(o, 0x00102B08u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102B08(o->h->ctx, a0, a1, a16_float(a2)) < 0) return a16_latch(o, 0x00102B08u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00102BB0(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00102BB0) return a16_latch(o, 0x00102BB0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102BB0(o->h->ctx, a0, a1, a16_float(a2)) < 0) return a16_latch(o, 0x00102BB0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00102C58(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00102C58) return a16_latch(o, 0x00102C58u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102C58(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x00102C58u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_00103230(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00103230) return a16_latch(o, 0x00103230u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00103230(o->h->ctx, a0, a1, a16_float(a2)) < 0) return a16_latch(o, 0x00103230u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_0011E2A8(A16 *o, uint32_t a0, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_0011E2A8) return a16_latch(o, 0x0011E2A8u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E2A8(o->h->ctx, a16_float(a0), &r) < 0) return a16_latch(o, 0x0011E2A8u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    *out = a16_bits(r);
    return 0;
}

static inline int a16_c_00122BB8(A16 *o, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a16_latch(o, 0x00122BB8u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a16_latch(o, 0x00122BB8u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001281C0(A16 *o, uint32_t a0, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001281C0) return a16_latch(o, 0x001281C0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001281C0(o->h->ctx, a16_float(a0), out) < 0) return a16_latch(o, 0x001281C0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_0019C6F0(A16 *o, int32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_0019C6F0) return a16_latch(o, 0x0019C6F0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019C6F0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x0019C6F0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001A2370(A16 *o, uint32_t a0, uint32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001A2370) return a16_latch(o, 0x001A2370u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001A2370(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001A2370u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001AF800(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001AF800) return a16_latch(o, 0x001AF800u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AF800(o->h->ctx, a0) < 0) return a16_latch(o, 0x001AF800u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001AFA90(A16 *o, int32_t a0, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001AFA90) return a16_latch(o, 0x001AFA90u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFA90(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001AFA90u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001AFC10(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a16_latch(o, 0x001AFC10u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a16_latch(o, 0x001AFC10u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B0FD0(A16 *o, uint32_t a0, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a16_latch(o, 0x001B0FD0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001B0FD0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B10B0(A16 *o, uint32_t a0, int32_t a1, int32_t a2, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a16_latch(o, 0x001B10B0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2, out) < 0) return a16_latch(o, 0x001B10B0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B11E0(A16 *o, int32_t a0, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B11E0) return a16_latch(o, 0x001B11E0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B11E0(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001B11E0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B17A0(A16 *o, uint32_t a0, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a16_latch(o, 0x001B17A0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001B17A0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B1B70(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B1B70) return a16_latch(o, 0x001B1B70u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1B70(o->h->ctx, a0) < 0) return a16_latch(o, 0x001B1B70u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B1EA0(A16 *o, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B1EA0) return a16_latch(o, 0x001B1EA0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1EA0(o->h->ctx, a0, a1, a2, a3, out) < 0) return a16_latch(o, 0x001B1EA0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001B6660(A16 *o, uint32_t a0, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001B6660) return a16_latch(o, 0x001B6660u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B6660(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001B6660u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001BA1A0(A16 *o, uint32_t a0, uint32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001BA1A0) return a16_latch(o, 0x001BA1A0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1A0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001BA1A0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001BA1C0(A16 *o, uint32_t a0, int32_t a1, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001BA1C0) return a16_latch(o, 0x001BA1C0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1C0(o->h->ctx, a0, a1, out) < 0) return a16_latch(o, 0x001BA1C0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001BA1F0(A16 *o, uint32_t a0, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001BA1F0) return a16_latch(o, 0x001BA1F0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA1F0(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001BA1F0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001BA540(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001BA540) return a16_latch(o, 0x001BA540u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA540(o->h->ctx, a0) < 0) return a16_latch(o, 0x001BA540u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001BA580(A16 *o, uint32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001BA580) return a16_latch(o, 0x001BA580u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BA580(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001BA580u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001BAD40(A16 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001BAD40) return a16_latch(o, 0x001BAD40u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001BAD40(o->h->ctx, a0, a1, out) < 0) return a16_latch(o, 0x001BAD40u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C4760(A16 *o, int32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C4760) return a16_latch(o, 0x001C4760u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C4760(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001C4760u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C5570(A16 *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C5570) return a16_latch(o, 0x001C5570u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C5570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a16_latch(o, 0x001C5570u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C5C90(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C5C90) return a16_latch(o, 0x001C5C90u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C5C90(o->h->ctx, a0) < 0) return a16_latch(o, 0x001C5C90u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C6120(A16 *o, int32_t a0, int32_t a1, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C6120) return a16_latch(o, 0x001C6120u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6120(o->h->ctx, a0, a1, out) < 0) return a16_latch(o, 0x001C6120u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C62C0(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C62C0) return a16_latch(o, 0x001C62C0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C62C0(o->h->ctx, a0) < 0) return a16_latch(o, 0x001C62C0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C6380(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C6380) return a16_latch(o, 0x001C6380u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a16_latch(o, 0x001C6380u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C63E0(A16 *o, uint32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a16_latch(o, 0x001C63E0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001C63E0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C64F0(A16 *o, uint32_t a0, uint32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a16_latch(o, 0x001C64F0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a16_float(a1)) < 0) return a16_latch(o, 0x001C64F0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001C68C0(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a16_latch(o, 0x001C68C0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a16_latch(o, 0x001C68C0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CA5E0(A16 *o, uint32_t a0, int32_t a1, int32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CA5E0) return a16_latch(o, 0x001CA5E0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA5E0(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x001CA5E0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CA5F0(A16 *o, uint32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CA5F0) return a16_latch(o, 0x001CA5F0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA5F0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001CA5F0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CA6E0(A16 *o, uint32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CA6E0) return a16_latch(o, 0x001CA6E0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CA6E0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001CA6E0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CB5B0(A16 *o, int32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CB5B0) return a16_latch(o, 0x001CB5B0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CB5B0(o->h->ctx, a0) < 0) return a16_latch(o, 0x001CB5B0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CCF70(A16 *o, uint32_t a0, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CCF70) return a16_latch(o, 0x001CCF70u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CCF70(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001CCF70u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CD520(A16 *o, int32_t a0, int32_t a1, uint32_t a2, uint64_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CD520) return a16_latch(o, 0x001CD520u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CD520(o->h->ctx, a0, a1, a2, a3, a4, a16_float(a5), a16_float(a6), a16_float(a7)) < 0) return a16_latch(o, 0x001CD520u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CFAE0(A16 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CFAE0) return a16_latch(o, 0x001CFAE0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFAE0(o->h->ctx, a0, a1, a2, a16_float(a3), a16_float(a4), a16_float(a5), a16_float(a6)) < 0) return a16_latch(o, 0x001CFAE0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001CFBE0(A16 *o, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001CFBE0) return a16_latch(o, 0x001CFBE0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFBE0(o->h->ctx, a0, a1, a2, a3, a4) < 0) return a16_latch(o, 0x001CFBE0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001D04B0(A16 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001D04B0) return a16_latch(o, 0x001D04B0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D04B0(o->h->ctx, a0, a1, a2, a16_float(a3), a16_float(a4)) < 0) return a16_latch(o, 0x001D04B0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001D7FA0(A16 *o, uint32_t a0, uint32_t a1, int32_t a2, uint32_t a3, uint32_t a4, int32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001D7FA0) return a16_latch(o, 0x001D7FA0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D7FA0(o->h->ctx, a0, a1, a2, a16_float(a3), a16_float(a4), out) < 0) return a16_latch(o, 0x001D7FA0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001D8060(A16 *o, int32_t a0, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001D8060) return a16_latch(o, 0x001D8060u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D8060(o->h->ctx, a0, out) < 0) return a16_latch(o, 0x001D8060u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001D80B0(A16 *o, int32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001D80B0) return a16_latch(o, 0x001D80B0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D80B0(o->h->ctx, a0) < 0) return a16_latch(o, 0x001D80B0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001D8BF0(A16 *o, uint32_t a0, int32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001D8BF0) return a16_latch(o, 0x001D8BF0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D8BF0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001D8BF0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001DAFA0(A16 *o)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001DAFA0) return a16_latch(o, 0x001DAFA0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001DAFA0(o->h->ctx) < 0) return a16_latch(o, 0x001DAFA0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001DB240(A16 *o, int32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001DB240) return a16_latch(o, 0x001DB240u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001DB240(o->h->ctx, a0) < 0) return a16_latch(o, 0x001DB240u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001DB250(A16 *o)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001DB250) return a16_latch(o, 0x001DB250u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001DB250(o->h->ctx) < 0) return a16_latch(o, 0x001DB250u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001DB480(A16 *o)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001DB480) return a16_latch(o, 0x001DB480u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001DB480(o->h->ctx) < 0) return a16_latch(o, 0x001DB480u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001EFD20(A16 *o, int32_t a0, uint32_t a1, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001EFD20) return a16_latch(o, 0x001EFD20u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFD20(o->h->ctx, a0, a1, out) < 0) return a16_latch(o, 0x001EFD20u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001EFD90(A16 *o, int32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001EFD90) return a16_latch(o, 0x001EFD90u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFD90(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x001EFD90u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001EFEB0(A16 *o, int32_t a0, uint32_t a1, uint32_t *out)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001EFEB0) return a16_latch(o, 0x001EFEB0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFEB0(o->h->ctx, a0, a1, out) < 0) return a16_latch(o, 0x001EFEB0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001F4BF0(A16 *o, uint32_t a0, uint32_t a1)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001F4BF0) return a16_latch(o, 0x001F4BF0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F4BF0(o->h->ctx, a0, a1) < 0) return a16_latch(o, 0x001F4BF0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001F4E20(A16 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001F4E20) return a16_latch(o, 0x001F4E20u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F4E20(o->h->ctx, a0, a1, a16_float(a2)) < 0) return a16_latch(o, 0x001F4E20u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001F5940(A16 *o, int32_t a0, uint32_t a1, int32_t a2)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001F5940) return a16_latch(o, 0x001F5940u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F5940(o->h->ctx, a0, a1, a2) < 0) return a16_latch(o, 0x001F5940u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001FAE70(A16 *o, int32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001FAE70) return a16_latch(o, 0x001FAE70u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FAE70(o->h->ctx, a0) < 0) return a16_latch(o, 0x001FAE70u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001FBD50(A16 *o, uint32_t a0, int32_t a1, int32_t a2, uint32_t a3)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001FBD50) return a16_latch(o, 0x001FBD50u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FBD50(o->h->ctx, a0, a1, a2, a16_float(a3)) < 0) return a16_latch(o, 0x001FBD50u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001FC3C0(A16 *o, uint32_t a0, uint32_t a1, int32_t a2, uint32_t a3, uint32_t a4)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001FC3C0) return a16_latch(o, 0x001FC3C0u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FC3C0(o->h->ctx, a0, a1, a2, a16_float(a3), a16_float(a4)) < 0) return a16_latch(o, 0x001FC3C0u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16_c_001FC520(A16 *o, uint32_t a0)
{
    if (a16_failed(o)) return -1;
    if (!o->h->w_001FC520) return a16_latch(o, 0x001FC520u, EM_AREA16_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FC520(o->h->ctx, a0) < 0) return a16_latch(o, 0x001FC520u, EM_AREA16_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
