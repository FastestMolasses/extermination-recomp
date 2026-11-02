/* Memory and fail-stop plumbing (the design of em_level17_boot_internal.h)
 * and the translations' shared declarations. */
#ifndef EM_AREA21_CREATURE_B_INTERNAL_H
#define EM_AREA21_CREATURE_B_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area21_creature_b.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea21CreatureBHooks *h;
    EmArea21CreatureBFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A21B;

static inline int a21b_latch(A21B *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA21_CREATURE_B_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a21b_failed(const A21B *o) { return o->fault->code != EM_AREA21_CREATURE_B_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a21b_at(A21B *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a21b_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a21b_latch(o, address, EM_AREA21_CREATURE_B_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a21b_u8(A21B *o, uint32_t a) { return *a21b_at(o, a, 1); }
static inline uint32_t a21b_u16(A21B *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a21b_at(o, a, 2), 2);
    return v;
}
static inline int32_t a21b_s16(A21B *o, uint32_t a) { return (int16_t)a21b_u16(o, a); }
static inline uint32_t a21b_u32(A21B *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a21b_at(o, a, 4), 4);
    return v;
}
static inline int32_t a21b_s32(A21B *o, uint32_t a) { return (int32_t)a21b_u32(o, a); }
static inline int32_t a21b_s8(A21B *o, uint32_t a) { return (int8_t)a21b_u8(o, a); }
static inline void a21b_w8(A21B *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a21b_at(o, a, 1), &b, 1);
}
static inline void a21b_w16(A21B *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a21b_at(o, a, 2), &h, 2);
}
static inline void a21b_w32(A21B *o, uint32_t a, uint32_t v) { memcpy(a21b_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a21b_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a21b_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a21b_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A21B_ADD(a, b) em_ee_add_bits((a), (b))
#define A21B_SUB(a, b) em_ee_sub_bits((a), (b))
#define A21B_MUL(a, b) em_ee_mul_bits((a), (b))
#define A21B_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A21B_LE(a, b) em_ee_c_le_bits((a), (b))
#define A21B_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A21B_DIV(a, b) em_ee_div_bits((a), (b))
#define A21B_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A21B_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a21b_u64(A21B *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a21b_at(o, a, 8), 8);
    return v;
}
static inline void a21b_w64(A21B *o, uint32_t a, uint64_t v) { memcpy(a21b_at(o, a, 8), &v, 8); }

static inline int a21b_begin(A21B *o, const EmArea21CreatureBHooks *h, EmArea21CreatureBFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a21b_failed(o) ? -1 : 0;
}

static inline int a21b_end(const A21B *o) { return a21b_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a21b_method(A21B *o, uint32_t self)
{
    if (a21b_failed(o)) return -1;
    const uint32_t fn = a21b_u32(o, self + 0x4C);
    if (a21b_failed(o)) return -1;
    if (!o->h->w_callback) return a21b_latch(o, fn, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a21b_latch(o, fn, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

/* BEGIN GENERATED WRAPPERS */
static inline int a21b_c_001026A0(A21B *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001026A0) return a21b_latch(o, 0x001026A0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a21b_latch(o, 0x001026A0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001028B8(A21B *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001028B8) return a21b_latch(o, 0x001028B8u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001028B8(o->h->ctx, a0, a1, a2) < 0) return a21b_latch(o, 0x001028B8u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_00102948(A21B *o, uint32_t a0, uint32_t a1)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_00102948) return a21b_latch(o, 0x00102948u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a21b_latch(o, 0x00102948u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_00102958(A21B *o, uint32_t a0, uint32_t a1)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_00102958) return a21b_latch(o, 0x00102958u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_00102958(o->h->ctx, a0, a1) < 0) return a21b_latch(o, 0x00102958u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001029C0(A21B *o, uint32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001029C0) return a21b_latch(o, 0x001029C0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001029C0(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001029C0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_00102BB0(A21B *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_00102BB0) return a21b_latch(o, 0x00102BB0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_00102BB0(o->h->ctx, a0, a1, a21b_float(a2)) < 0) return a21b_latch(o, 0x00102BB0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_0011DE90(A21B *o, uint32_t a0, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_0011DE90) return a21b_latch(o, 0x0011DE90u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DE90(o->h->ctx, a21b_float(a0), &r) < 0) return a21b_latch(o, 0x0011DE90u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_0011DF78(A21B *o, uint32_t a0, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_0011DF78) return a21b_latch(o, 0x0011DF78u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011DF78(o->h->ctx, a21b_float(a0), &r) < 0) return a21b_latch(o, 0x0011DF78u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_0011E2A8(A21B *o, uint32_t a0, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_0011E2A8) return a21b_latch(o, 0x0011E2A8u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E2A8(o->h->ctx, a21b_float(a0), &r) < 0) return a21b_latch(o, 0x0011E2A8u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_00122BB8(A21B *o, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a21b_latch(o, 0x00122BB8u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a21b_latch(o, 0x00122BB8u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001283D0(A21B *o, uint32_t a0, int32_t a1, uint32_t a2, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001283D0) return a21b_latch(o, 0x001283D0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001283D0(o->h->ctx, a0, a1, a2, out) < 0) return a21b_latch(o, 0x001283D0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_0014D1E0(A21B *o, uint32_t a0, uint32_t a1, int32_t a2)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_0014D1E0) return a21b_latch(o, 0x0014D1E0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_0014D1E0(o->h->ctx, a0, a1, a2) < 0) return a21b_latch(o, 0x0014D1E0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001AFC10(A21B *o, uint32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a21b_latch(o, 0x001AFC10u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001AFC10u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001B10B0(A21B *o, uint32_t a0, int32_t a1, int32_t a2, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a21b_latch(o, 0x001B10B0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2, out) < 0) return a21b_latch(o, 0x001B10B0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001B1190(A21B *o, int32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B1190) return a21b_latch(o, 0x001B1190u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001B1190(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001B1190u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001B1240(A21B *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B1240) return a21b_latch(o, 0x001B1240u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1240(o->h->ctx, a0, a21b_float(a1), a21b_float(a2), &r) < 0) return a21b_latch(o, 0x001B1240u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_001B12B0(A21B *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B12B0) return a21b_latch(o, 0x001B12B0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B12B0(o->h->ctx, a21b_float(a0), a21b_float(a1), a21b_float(a2), &r) < 0) return a21b_latch(o, 0x001B12B0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_001B1470(A21B *o, uint32_t a0, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B1470) return a21b_latch(o, 0x001B1470u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a21b_float(a0), &r) < 0) return a21b_latch(o, 0x001B1470u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_001B15D0(A21B *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B15D0) return a21b_latch(o, 0x001B15D0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B15D0(o->h->ctx, a0, a1, &r) < 0) return a21b_latch(o, 0x001B15D0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    *out = a21b_bits(r);
    return 0;
}

static inline int a21b_c_001B17A0(A21B *o, uint32_t a0, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a21b_latch(o, 0x001B17A0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0, out) < 0) return a21b_latch(o, 0x001B17A0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001B1EA0(A21B *o, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B1EA0) return a21b_latch(o, 0x001B1EA0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001B1EA0(o->h->ctx, a0, a1, a2, a3, out) < 0) return a21b_latch(o, 0x001B1EA0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001B4810(A21B *o, uint32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B4810) return a21b_latch(o, 0x001B4810u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001B4810(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001B4810u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001B4CF0(A21B *o, uint32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001B4CF0) return a21b_latch(o, 0x001B4CF0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001B4CF0(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001B4CF0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001C63E0(A21B *o, uint32_t a0, int32_t a1)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a21b_latch(o, 0x001C63E0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a21b_latch(o, 0x001C63E0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001C64F0(A21B *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a21b_latch(o, 0x001C64F0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a21b_float(a1), out) < 0) return a21b_latch(o, 0x001C64F0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001C67E0(A21B *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001C67E0) return a21b_latch(o, 0x001C67E0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001C67E0(o->h->ctx, a0, a1, a21b_float(a2), a21b_float(a3)) < 0) return a21b_latch(o, 0x001C67E0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001C68C0(A21B *o, uint32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a21b_latch(o, 0x001C68C0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001C68C0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001EFD90(A21B *o, int32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001EFD90) return a21b_latch(o, 0x001EFD90u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001EFD90(o->h->ctx, a0, a1, a2) < 0) return a21b_latch(o, 0x001EFD90u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001EFE00(A21B *o, int32_t a0, uint32_t a1, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001EFE00) return a21b_latch(o, 0x001EFE00u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001EFE00(o->h->ctx, a0, a1, out) < 0) return a21b_latch(o, 0x001EFE00u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001F91C0(A21B *o, uint32_t a0)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001F91C0) return a21b_latch(o, 0x001F91C0u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001F91C0(o->h->ctx, a0) < 0) return a21b_latch(o, 0x001F91C0u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_001FBD50(A21B *o, uint32_t a0, int32_t a1, int32_t a2, uint32_t a3, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_001FBD50) return a21b_latch(o, 0x001FBD50u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_001FBD50(o->h->ctx, a0, a1, a2, a21b_float(a3), out) < 0) return a21b_latch(o, 0x001FBD50u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21b_c_0021BE40(A21B *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a21b_failed(o)) return -1;
    if (!o->h->w_0021BE40) return a21b_latch(o, 0x0021BE40u, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER);
    if (o->h->w_0021BE40(o->h->ctx, a0, a1, out) < 0) return a21b_latch(o, 0x0021BE40u, EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

/* The translations (one per original function; they call each other
 * directly, as the originals do). */
uint32_t a21b_0014EC00(A21B *o, uint32_t self);
uint32_t a21b_0014ED10(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_0014EDD0(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_0014EEF0(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_0014F390(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_0014FBC0(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00150190(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00150260(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_001504D0(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00150600(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00150820(A21B *o, uint32_t self);
uint32_t a21b_00150850(A21B *o, uint32_t self);
uint32_t a21b_00150900(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_001509A0(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00150DA0(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00150F40(A21B *o, uint32_t self, uint32_t ctl);
uint32_t a21b_00151200(A21B *o, uint32_t self, uint32_t ctl);
int32_t a21b_001513B0(A21B *o, uint32_t self, uint32_t ctl);

#endif
