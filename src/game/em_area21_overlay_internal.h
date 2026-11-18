/* Memory and fail-stop plumbing of em_area21_overlay (the design of
 * em_area14_overlay_internal.h, as em_area21_doors_internal.h). */
#ifndef EM_AREA21_OVERLAY_INTERNAL_H
#define EM_AREA21_OVERLAY_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area21_overlay.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea21OverlayHooks *h;
    EmArea21OverlayFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A21G;

static inline int a21g_latch(A21G *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA21_OVERLAY_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a21g_failed(const A21G *o) { return o->fault->code != EM_AREA21_OVERLAY_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a21g_at(A21G *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a21g_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a21g_latch(o, address, EM_AREA21_OVERLAY_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a21g_u8(A21G *o, uint32_t a) { return *a21g_at(o, a, 1); }
static inline uint32_t a21g_u16(A21G *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a21g_at(o, a, 2), 2);
    return v;
}
static inline int32_t a21g_s16(A21G *o, uint32_t a) { return (int16_t)a21g_u16(o, a); }
static inline uint32_t a21g_u32(A21G *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a21g_at(o, a, 4), 4);
    return v;
}
static inline int32_t a21g_s32(A21G *o, uint32_t a) { return (int32_t)a21g_u32(o, a); }
static inline void a21g_w8(A21G *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a21g_at(o, a, 1), &b, 1);
}
static inline void a21g_w16(A21G *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a21g_at(o, a, 2), &h, 2);
}
static inline void a21g_w32(A21G *o, uint32_t a, uint32_t v) { memcpy(a21g_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a21g_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a21g_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A21G_ADD(a, b) em_ee_add_bits((a), (b))
#define A21G_SUB(a, b) em_ee_sub_bits((a), (b))
#define A21G_MUL(a, b) em_ee_mul_bits((a), (b))
#define A21G_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A21G_LE(a, b) em_ee_c_le_bits((a), (b))
#define A21G_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A21G_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A21G_DIV(a, b) em_ee_div_bits((a), (b))
#define A21G_NEG(a) em_ee_neg_bits((a))

static inline int a21g_begin(A21G *o, const EmArea21OverlayHooks *h, EmArea21OverlayFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a21g_failed(o) ? -1 : 0;
}

static inline int a21g_end(const A21G *o) { return a21g_failed(o) ? -1 : 0; }

/* A node's +0x4C method, called with the node (the function word is read
 * first, as the original loads it before its register call). */
static inline int a21g_method(A21G *o, uint32_t self)
{
    if (a21g_failed(o)) return -1;
    const uint32_t fn = a21g_u32(o, self + 0x4C);
    if (a21g_failed(o)) return -1;
    if (!o->h->w_callback) return a21g_latch(o, fn, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a21g_latch(o, fn, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on success, -1 when skipped or failed. */
/* BEGIN GENERATED WRAPPERS */
static inline int a21g_c_001026A0(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001026A0) return a21g_latch(o, 0x001026A0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x001026A0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001026D0(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001026D0) return a21g_latch(o, 0x001026D0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001026D0(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x001026D0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00102760(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00102760) return a21g_latch(o, 0x00102760u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102760(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x00102760u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001028D0(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001028D0) return a21g_latch(o, 0x001028D0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001028D0(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x001028D0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00102918(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00102918) return a21g_latch(o, 0x00102918u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102918(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x00102918u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00102948(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00102948) return a21g_latch(o, 0x00102948u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x00102948u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00102958(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00102958) return a21g_latch(o, 0x00102958u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102958(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x00102958u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001029C0(A21G *o, uint32_t a0)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001029C0) return a21g_latch(o, 0x001029C0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a21g_latch(o, 0x001029C0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00102B08(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00102B08) return a21g_latch(o, 0x00102B08u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102B08(o->h->ctx, a0, a1, a21g_float(a2)) < 0) return a21g_latch(o, 0x00102B08u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00102BB0(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00102BB0) return a21g_latch(o, 0x00102BB0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00102BB0(o->h->ctx, a0, a1, a21g_float(a2)) < 0) return a21g_latch(o, 0x00102BB0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001031E0(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001031E0) return a21g_latch(o, 0x001031E0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001031E0(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x001031E0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00103230(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00103230) return a21g_latch(o, 0x00103230u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00103230(o->h->ctx, a0, a1, a21g_float(a2)) < 0) return a21g_latch(o, 0x00103230u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_0011DE90(A21G *o, uint32_t a0, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_0011DE90) return a21g_latch(o, 0x0011DE90u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DE90(o->h->ctx, a21g_float(a0), &r) < 0) return a21g_latch(o, 0x0011DE90u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    *out = a21g_bits(r);
    return 0;
}

static inline int a21g_c_0011DF78(A21G *o, uint32_t a0, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_0011DF78) return a21g_latch(o, 0x0011DF78u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DF78(o->h->ctx, a21g_float(a0), &r) < 0) return a21g_latch(o, 0x0011DF78u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    *out = a21g_bits(r);
    return 0;
}

static inline int a21g_c_00122BB8(A21G *o, int32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a21g_latch(o, 0x00122BB8u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a21g_latch(o, 0x00122BB8u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001281C0(A21G *o, uint32_t a0, int32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001281C0) return a21g_latch(o, 0x001281C0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001281C0(o->h->ctx, a21g_float(a0), out) < 0) return a21g_latch(o, 0x001281C0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00182F90(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00182F90) return a21g_latch(o, 0x00182F90u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00182F90(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x00182F90u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_0019A570(A21G *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_0019A570) return a21g_latch(o, 0x0019A570u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_0019A570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a21g_latch(o, 0x0019A570u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001A2370(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001A2370) return a21g_latch(o, 0x001A2370u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001A2370(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x001A2370u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001AFA90(A21G *o, int32_t a0, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001AFA90) return a21g_latch(o, 0x001AFA90u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFA90(o->h->ctx, a0, out) < 0) return a21g_latch(o, 0x001AFA90u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001AFC10(A21G *o, uint32_t a0)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a21g_latch(o, 0x001AFC10u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a21g_latch(o, 0x001AFC10u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001B0FD0(A21G *o, uint32_t a0, int32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a21g_latch(o, 0x001B0FD0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a21g_latch(o, 0x001B0FD0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001B1470(A21G *o, uint32_t a0, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001B1470) return a21g_latch(o, 0x001B1470u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a21g_float(a0), &r) < 0) return a21g_latch(o, 0x001B1470u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    *out = a21g_bits(r);
    return 0;
}

static inline int a21g_c_001B17A0(A21G *o, uint32_t a0)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a21g_latch(o, 0x001B17A0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0) < 0) return a21g_latch(o, 0x001B17A0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001B1E20(A21G *o, int32_t a0, int32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001B1E20) return a21g_latch(o, 0x001B1E20u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B1E20(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x001B1E20u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001B41F0(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001B41F0) return a21g_latch(o, 0x001B41F0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B41F0(o->h->ctx, a0, a1, a2, a3, a4, a5) < 0) return a21g_latch(o, 0x001B41F0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001B5DC0(A21G *o, int32_t a0, int32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001B5DC0) return a21g_latch(o, 0x001B5DC0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001B5DC0(o->h->ctx, a0, out) < 0) return a21g_latch(o, 0x001B5DC0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001C5570(A21G *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001C5570) return a21g_latch(o, 0x001C5570u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C5570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a21g_latch(o, 0x001C5570u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001C6380(A21G *o, uint32_t a0)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001C6380) return a21g_latch(o, 0x001C6380u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a21g_latch(o, 0x001C6380u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001C94B0(A21G *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001C94B0) return a21g_latch(o, 0x001C94B0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001C94B0(o->h->ctx, a0, a1, a2, a3) < 0) return a21g_latch(o, 0x001C94B0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001CCF70(A21G *o, uint32_t a0, int32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001CCF70) return a21g_latch(o, 0x001CCF70u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CCF70(o->h->ctx, a0, out) < 0) return a21g_latch(o, 0x001CCF70u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001CFB50(A21G *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001CFB50) return a21g_latch(o, 0x001CFB50u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFB50(o->h->ctx, a0, a1, a2, a21g_float(a3), a21g_float(a4), a21g_float(a5), a21g_float(a6), a21g_float(a7)) < 0) return a21g_latch(o, 0x001CFB50u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001CFBE0(A21G *o, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001CFBE0) return a21g_latch(o, 0x001CFBE0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001CFBE0(o->h->ctx, a0, a1, a2, a3, a4) < 0) return a21g_latch(o, 0x001CFBE0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001D1F80(A21G *o, int32_t a0, int32_t a1, int32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001D1F80) return a21g_latch(o, 0x001D1F80u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001D1F80(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x001D1F80u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001EFD20(A21G *o, uint32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001EFD20) return a21g_latch(o, 0x001EFD20u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFD20(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x001EFD20u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001EFD90(A21G *o, int32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001EFD90) return a21g_latch(o, 0x001EFD90u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFD90(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x001EFD90u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001EFE00(A21G *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001EFE00) return a21g_latch(o, 0x001EFE00u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFE00(o->h->ctx, a0, a1, out) < 0) return a21g_latch(o, 0x001EFE00u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001EFEB0(A21G *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001EFEB0) return a21g_latch(o, 0x001EFEB0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001EFEB0(o->h->ctx, a0, a1, out) < 0) return a21g_latch(o, 0x001EFEB0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001F4010(A21G *o, int32_t a0, uint32_t a1)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001F4010) return a21g_latch(o, 0x001F4010u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F4010(o->h->ctx, a0, a1) < 0) return a21g_latch(o, 0x001F4010u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001F4F40(A21G *o, int32_t a0, uint32_t *out)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001F4F40) return a21g_latch(o, 0x001F4F40u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F4F40(o->h->ctx, a0, out) < 0) return a21g_latch(o, 0x001F4F40u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001F5940(A21G *o, int32_t a0, uint32_t a1, int32_t a2)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001F5940) return a21g_latch(o, 0x001F5940u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001F5940(o->h->ctx, a0, a1, a2) < 0) return a21g_latch(o, 0x001F5940u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001FB9F0(A21G *o, int32_t a0, int32_t a1, int32_t a2, int32_t a3)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001FB9F0) return a21g_latch(o, 0x001FB9F0u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FB9F0(o->h->ctx, a0, a1, a2, a3) < 0) return a21g_latch(o, 0x001FB9F0u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_001FBD50(A21G *o, uint32_t a0, int32_t a1, int32_t a2, uint32_t a3)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_001FBD50) return a21g_latch(o, 0x001FBD50u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_001FBD50(o->h->ctx, a0, a1, a2, a21g_float(a3)) < 0) return a21g_latch(o, 0x001FBD50u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21g_c_00207F80(A21G *o, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, uint32_t a5)
{
    if (a21g_failed(o)) return -1;
    if (!o->h->w_00207F80) return a21g_latch(o, 0x00207F80u, EM_AREA21_OVERLAY_FAULT_NULL_WORKER);
    if (o->h->w_00207F80(o->h->ctx, a0, a1, a2, a3, a4, a5) < 0) return a21g_latch(o, 0x00207F80u, EM_AREA21_OVERLAY_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

/* The translations shared by the two source files (one original function
 * each; em_area21_overlay.c). */
void a21g_829310(A21G *o, uint32_t v, int32_t key);
void a21g_points(A21G *o, uint32_t self, uint32_t out);

#endif
