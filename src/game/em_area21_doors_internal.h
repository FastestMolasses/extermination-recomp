/* Memory and fail-stop plumbing of em_area21_doors (the design of
 * em_area14_overlay_internal.h). */
#ifndef EM_AREA21_DOORS_INTERNAL_H
#define EM_AREA21_DOORS_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area21_doors.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea21DoorsHooks *h;
    EmArea21DoorsFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A21D;

static inline int a21d_latch(A21D *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA21_DOORS_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a21d_failed(const A21D *o) { return o->fault->code != EM_AREA21_DOORS_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a21d_at(A21D *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a21d_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a21d_latch(o, address, EM_AREA21_DOORS_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a21d_u8(A21D *o, uint32_t a) { return *a21d_at(o, a, 1); }
static inline uint32_t a21d_u16(A21D *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a21d_at(o, a, 2), 2);
    return v;
}
static inline int32_t a21d_s16(A21D *o, uint32_t a) { return (int16_t)a21d_u16(o, a); }
static inline uint32_t a21d_u32(A21D *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a21d_at(o, a, 4), 4);
    return v;
}
static inline int32_t a21d_s32(A21D *o, uint32_t a) { return (int32_t)a21d_u32(o, a); }
static inline void a21d_w8(A21D *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a21d_at(o, a, 1), &b, 1);
}
static inline void a21d_w16(A21D *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a21d_at(o, a, 2), &h, 2);
}
static inline void a21d_w32(A21D *o, uint32_t a, uint32_t v) { memcpy(a21d_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a21d_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a21d_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A21D_ADD(a, b) em_ee_add_bits((a), (b))
#define A21D_SUB(a, b) em_ee_sub_bits((a), (b))
#define A21D_MUL(a, b) em_ee_mul_bits((a), (b))
#define A21D_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A21D_LE(a, b) em_ee_c_le_bits((a), (b))
#define A21D_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A21D_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))

static inline int a21d_begin(A21D *o, const EmArea21DoorsHooks *h, EmArea21DoorsFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a21d_failed(o) ? -1 : 0;
}

static inline int a21d_end(const A21D *o) { return a21d_failed(o) ? -1 : 0; }

/* A node's +0x4C method, called with the node (the function word is read
 * first, as the original loads it before its register call). */
static inline int a21d_method(A21D *o, uint32_t self)
{
    if (a21d_failed(o)) return -1;
    const uint32_t fn = a21d_u32(o, self + 0x4C);
    if (a21d_failed(o)) return -1;
    if (!o->h->w_callback) return a21d_latch(o, fn, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a21d_latch(o, fn, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on success, -1 when skipped or failed. */
/* BEGIN GENERATED WRAPPERS */
static inline int a21d_c_00102760(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_00102760) return a21d_latch(o, 0x00102760u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_00102760(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x00102760u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001028B8(A21D *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001028B8) return a21d_latch(o, 0x001028B8u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001028B8(o->h->ctx, a0, a1, a2) < 0) return a21d_latch(o, 0x001028B8u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_00102948(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_00102948) return a21d_latch(o, 0x00102948u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x00102948u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_00103230(A21D *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_00103230) return a21d_latch(o, 0x00103230u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_00103230(o->h->ctx, a0, a1, a21d_float(a2)) < 0) return a21d_latch(o, 0x00103230u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_0011DF78(A21D *o, uint32_t a0, uint32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_0011DF78) return a21d_latch(o, 0x0011DF78u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DF78(o->h->ctx, a21d_float(a0), &r) < 0) return a21d_latch(o, 0x0011DF78u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    *out = a21d_bits(r);
    return 0;
}

static inline int a21d_c_00122BB8(A21D *o, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a21d_latch(o, 0x00122BB8u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a21d_latch(o, 0x00122BB8u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_0015AC00(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_0015AC00) return a21d_latch(o, 0x0015AC00u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_0015AC00(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x0015AC00u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_0015AE20(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_0015AE20) return a21d_latch(o, 0x0015AE20u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_0015AE20(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x0015AE20u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_0019A570(A21D *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_0019A570) return a21d_latch(o, 0x0019A570u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_0019A570(o->h->ctx, a0, a1, a2, a3, out) < 0) return a21d_latch(o, 0x0019A570u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001A2370(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001A2370) return a21d_latch(o, 0x001A2370u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001A2370(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x001A2370u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001AFC10(A21D *o, uint32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a21d_latch(o, 0x001AFC10u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001AFC10u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001B0FD0(A21D *o, uint32_t a0, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a21d_latch(o, 0x001B0FD0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a21d_latch(o, 0x001B0FD0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001B1190(A21D *o, int32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001B1190) return a21d_latch(o, 0x001B1190u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001B1190(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001B1190u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001B1470(A21D *o, uint32_t a0, uint32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001B1470) return a21d_latch(o, 0x001B1470u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a21d_float(a0), &r) < 0) return a21d_latch(o, 0x001B1470u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    *out = a21d_bits(r);
    return 0;
}

static inline int a21d_c_001B17A0(A21D *o, uint32_t a0, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a21d_latch(o, 0x001B17A0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0, out) < 0) return a21d_latch(o, 0x001B17A0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001B1B70(A21D *o, uint32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001B1B70) return a21d_latch(o, 0x001B1B70u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001B1B70(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001B1B70u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001B41F0(A21D *o, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001B41F0) return a21d_latch(o, 0x001B41F0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001B41F0(o->h->ctx, a0, a1, a2, a3, a4, a5) < 0) return a21d_latch(o, 0x001B41F0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BA1A0(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BA1A0) return a21d_latch(o, 0x001BA1A0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BA1A0(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x001BA1A0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BBDA0(A21D *o, uint32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BBDA0) return a21d_latch(o, 0x001BBDA0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BBDA0(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001BBDA0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BBE40(A21D *o, uint32_t a0, uint32_t a1, int32_t a2, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BBE40) return a21d_latch(o, 0x001BBE40u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BBE40(o->h->ctx, a0, a1, a2, out) < 0) return a21d_latch(o, 0x001BBE40u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BC0E0(A21D *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BC0E0) return a21d_latch(o, 0x001BC0E0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BC0E0(o->h->ctx, a0, a1, out) < 0) return a21d_latch(o, 0x001BC0E0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BC240(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BC240) return a21d_latch(o, 0x001BC240u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BC240(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x001BC240u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BC290(A21D *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BC290) return a21d_latch(o, 0x001BC290u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BC290(o->h->ctx, a0, a1, out) < 0) return a21d_latch(o, 0x001BC290u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001BC300(A21D *o, uint32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001BC300) return a21d_latch(o, 0x001BC300u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001BC300(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001BC300u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001C6120(A21D *o, int32_t a0, int32_t a1, int32_t *out)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001C6120) return a21d_latch(o, 0x001C6120u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001C6120(o->h->ctx, a0, a1, out) < 0) return a21d_latch(o, 0x001C6120u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001C62C0(A21D *o, uint32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001C62C0) return a21d_latch(o, 0x001C62C0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001C62C0(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001C62C0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001C6380(A21D *o, uint32_t a0)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001C6380) return a21d_latch(o, 0x001C6380u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a21d_latch(o, 0x001C6380u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001CA6E0(A21D *o, uint32_t a0, int32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001CA6E0) return a21d_latch(o, 0x001CA6E0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001CA6E0(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x001CA6E0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001CD390(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001CD390) return a21d_latch(o, 0x001CD390u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001CD390(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x001CD390u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001CEEE0(A21D *o, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, uint32_t a4, uint32_t a5, uint32_t a6)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001CEEE0) return a21d_latch(o, 0x001CEEE0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001CEEE0(o->h->ctx, a0, a1, a2, a3, a21d_float(a4), a21d_float(a5), a21d_float(a6)) < 0) return a21d_latch(o, 0x001CEEE0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001EFD20(A21D *o, uint32_t a0, uint32_t a1)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001EFD20) return a21d_latch(o, 0x001EFD20u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001EFD20(o->h->ctx, a0, a1) < 0) return a21d_latch(o, 0x001EFD20u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001EFD90(A21D *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001EFD90) return a21d_latch(o, 0x001EFD90u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001EFD90(o->h->ctx, a0, a1, a2) < 0) return a21d_latch(o, 0x001EFD90u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001F02C0(A21D *o, uint32_t a0, int32_t a1, uint32_t a2)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001F02C0) return a21d_latch(o, 0x001F02C0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001F02C0(o->h->ctx, a0, a1, a21d_float(a2)) < 0) return a21d_latch(o, 0x001F02C0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21d_c_001FC3C0(A21D *o, uint32_t a0, uint32_t a1, int32_t a2, uint32_t a3, uint32_t a4)
{
    if (a21d_failed(o)) return -1;
    if (!o->h->w_001FC3C0) return a21d_latch(o, 0x001FC3C0u, EM_AREA21_DOORS_FAULT_NULL_WORKER);
    if (o->h->w_001FC3C0(o->h->ctx, a0, a1, a2, a21d_float(a3), a21d_float(a4)) < 0) return a21d_latch(o, 0x001FC3C0u, EM_AREA21_DOORS_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
