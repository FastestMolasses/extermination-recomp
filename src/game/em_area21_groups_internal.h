/* Memory and fail-stop plumbing of em_area21_groups (the design of
 * em_area14_overlay_internal.h). */
#ifndef EM_AREA21_GROUPS_INTERNAL_H
#define EM_AREA21_GROUPS_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area21_groups.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea21GroupsHooks *h;
    EmArea21GroupsFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A21P;

static inline int a21p_latch(A21P *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA21_GROUPS_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a21p_failed(const A21P *o) { return o->fault->code != EM_AREA21_GROUPS_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a21p_at(A21P *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a21p_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a21p_latch(o, address, EM_AREA21_GROUPS_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a21p_u8(A21P *o, uint32_t a) { return *a21p_at(o, a, 1); }
static inline uint32_t a21p_u16(A21P *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a21p_at(o, a, 2), 2);
    return v;
}
static inline int32_t a21p_s16(A21P *o, uint32_t a) { return (int16_t)a21p_u16(o, a); }
static inline uint32_t a21p_u32(A21P *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a21p_at(o, a, 4), 4);
    return v;
}
static inline int32_t a21p_s32(A21P *o, uint32_t a) { return (int32_t)a21p_u32(o, a); }
static inline void a21p_w8(A21P *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a21p_at(o, a, 1), &b, 1);
}
static inline void a21p_w16(A21P *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a21p_at(o, a, 2), &h, 2);
}
static inline void a21p_w32(A21P *o, uint32_t a, uint32_t v) { memcpy(a21p_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a21p_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a21p_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A21P_ADD(a, b) em_ee_add_bits((a), (b))
#define A21P_SUB(a, b) em_ee_sub_bits((a), (b))
#define A21P_MUL(a, b) em_ee_mul_bits((a), (b))
#define A21P_DIV(a, b) em_ee_div_bits((a), (b))
#define A21P_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A21P_LE(a, b) em_ee_c_le_bits((a), (b))
#define A21P_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A21P_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))

static inline int a21p_begin(A21P *o, const EmArea21GroupsHooks *h, EmArea21GroupsFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a21p_failed(o) ? -1 : 0;
}

static inline int a21p_end(const A21P *o) { return a21p_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on success, -1 when skipped or failed. */
/* BEGIN GENERATED WRAPPERS */
static inline int a21p_c_001026A0(A21P *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001026A0) return a21p_latch(o, 0x001026A0u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a21p_latch(o, 0x001026A0u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_00102918(A21P *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_00102918) return a21p_latch(o, 0x00102918u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_00102918(o->h->ctx, a0, a1, a2) < 0) return a21p_latch(o, 0x00102918u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_00102948(A21P *o, uint32_t a0, uint32_t a1)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_00102948) return a21p_latch(o, 0x00102948u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a21p_latch(o, 0x00102948u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001029C0(A21P *o, uint32_t a0)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001029C0) return a21p_latch(o, 0x001029C0u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a21p_latch(o, 0x001029C0u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_00102BB0(A21P *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_00102BB0) return a21p_latch(o, 0x00102BB0u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_00102BB0(o->h->ctx, a0, a1, a21p_float(a2)) < 0) return a21p_latch(o, 0x00102BB0u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_00102C58(A21P *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_00102C58) return a21p_latch(o, 0x00102C58u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_00102C58(o->h->ctx, a0, a1, a2) < 0) return a21p_latch(o, 0x00102C58u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_0011DE90(A21P *o, uint32_t a0, uint32_t *out)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_0011DE90) return a21p_latch(o, 0x0011DE90u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DE90(o->h->ctx, a21p_float(a0), &r) < 0) return a21p_latch(o, 0x0011DE90u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    *out = a21p_bits(r);
    return 0;
}

static inline int a21p_c_00122BB8(A21P *o, int32_t *out)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a21p_latch(o, 0x00122BB8u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a21p_latch(o, 0x00122BB8u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001281C0(A21P *o, uint32_t a0, int32_t *out)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001281C0) return a21p_latch(o, 0x001281C0u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001281C0(o->h->ctx, a21p_float(a0), out) < 0) return a21p_latch(o, 0x001281C0u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001AFC10(A21P *o, uint32_t a0)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a21p_latch(o, 0x001AFC10u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a21p_latch(o, 0x001AFC10u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001C6190(A21P *o, uint32_t a0, int32_t *out)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001C6190) return a21p_latch(o, 0x001C6190u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001C6190(o->h->ctx, a0, out) < 0) return a21p_latch(o, 0x001C6190u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001CCF70(A21P *o, uint32_t a0, int32_t *out)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001CCF70) return a21p_latch(o, 0x001CCF70u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001CCF70(o->h->ctx, a0, out) < 0) return a21p_latch(o, 0x001CCF70u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001CD520(A21P *o, int32_t a0, int32_t a1, uint32_t a2, uint64_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001CD520) return a21p_latch(o, 0x001CD520u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001CD520(o->h->ctx, a0, a1, a2, a3, a4, a21p_float(a5), a21p_float(a6), a21p_float(a7)) < 0) return a21p_latch(o, 0x001CD520u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001CFB50(A21P *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001CFB50) return a21p_latch(o, 0x001CFB50u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001CFB50(o->h->ctx, a0, a1, a2, a21p_float(a3), a21p_float(a4), a21p_float(a5), a21p_float(a6), a21p_float(a7)) < 0) return a21p_latch(o, 0x001CFB50u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001CFBE0(A21P *o, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001CFBE0) return a21p_latch(o, 0x001CFBE0u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001CFBE0(o->h->ctx, a0, a1, a2, a3, a4) < 0) return a21p_latch(o, 0x001CFBE0u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_001EFD20(A21P *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_001EFD20) return a21p_latch(o, 0x001EFD20u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_001EFD20(o->h->ctx, a0, a1, out) < 0) return a21p_latch(o, 0x001EFD20u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21p_c_0021B9A0(A21P *o, int32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21p_failed(o)) return -1;
    if (!o->h->w_0021B9A0) return a21p_latch(o, 0x0021B9A0u, EM_AREA21_GROUPS_FAULT_NULL_WORKER);
    if (o->h->w_0021B9A0(o->h->ctx, a0, a21p_float(a1), a21p_float(a2)) < 0) return a21p_latch(o, 0x0021B9A0u, EM_AREA21_GROUPS_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
