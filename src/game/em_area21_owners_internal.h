/* Memory and fail-stop plumbing of em_area21_owners (the design of
 * em_area14_overlay_internal.h). */
#ifndef EM_AREA21_OWNERS_INTERNAL_H
#define EM_AREA21_OWNERS_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area21_owners.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea21OwnersHooks *h;
    EmArea21OwnersFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A21W;

static inline int a21w_latch(A21W *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA21_OWNERS_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a21w_failed(const A21W *o) { return o->fault->code != EM_AREA21_OWNERS_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a21w_at(A21W *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a21w_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a21w_latch(o, address, EM_AREA21_OWNERS_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a21w_u8(A21W *o, uint32_t a) { return *a21w_at(o, a, 1); }
static inline uint32_t a21w_u16(A21W *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a21w_at(o, a, 2), 2);
    return v;
}
static inline int32_t a21w_s16(A21W *o, uint32_t a) { return (int16_t)a21w_u16(o, a); }
static inline uint32_t a21w_u32(A21W *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a21w_at(o, a, 4), 4);
    return v;
}
static inline int32_t a21w_s32(A21W *o, uint32_t a) { return (int32_t)a21w_u32(o, a); }
static inline int32_t a21w_s8(A21W *o, uint32_t a) { return (int8_t)a21w_u8(o, a); }
static inline void a21w_w8(A21W *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a21w_at(o, a, 1), &b, 1);
}
static inline void a21w_w16(A21W *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a21w_at(o, a, 2), &h, 2);
}
static inline void a21w_w32(A21W *o, uint32_t a, uint32_t v) { memcpy(a21w_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a21w_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a21w_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a21w_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A21W_ADD(a, b) em_ee_add_bits((a), (b))
#define A21W_SUB(a, b) em_ee_sub_bits((a), (b))
#define A21W_MUL(a, b) em_ee_mul_bits((a), (b))
#define A21W_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A21W_LE(a, b) em_ee_c_le_bits((a), (b))
#define A21W_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A21W_DIV(a, b) em_ee_div_bits((a), (b))
#define A21W_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A21W_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a21w_u64(A21W *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a21w_at(o, a, 8), 8);
    return v;
}
static inline void a21w_w64(A21W *o, uint32_t a, uint64_t v) { memcpy(a21w_at(o, a, 8), &v, 8); }

static inline int a21w_begin(A21W *o, const EmArea21OwnersHooks *h, EmArea21OwnersFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a21w_failed(o) ? -1 : 0;
}

static inline int a21w_end(const A21W *o) { return a21w_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a21w_method(A21W *o, uint32_t self)
{
    if (a21w_failed(o)) return -1;
    const uint32_t fn = a21w_u32(o, self + 0x4C);
    if (a21w_failed(o)) return -1;
    if (!o->h->w_callback) return a21w_latch(o, fn, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a21w_latch(o, fn, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}


/* BEGIN GENERATED WRAPPERS */
static inline int a21w_c_001026A0(A21W *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001026A0) return a21w_latch(o, 0x001026A0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a21w_latch(o, 0x001026A0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001028B8(A21W *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001028B8) return a21w_latch(o, 0x001028B8u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001028B8(o->h->ctx, a0, a1, a2) < 0) return a21w_latch(o, 0x001028B8u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_00102948(A21W *o, uint32_t a0, uint32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_00102948) return a21w_latch(o, 0x00102948u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x00102948u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001281C0(A21W *o, uint32_t a0, int32_t *out)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001281C0) return a21w_latch(o, 0x001281C0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001281C0(o->h->ctx, a21w_float(a0), out) < 0) return a21w_latch(o, 0x001281C0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_0015C700(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_0015C700) return a21w_latch(o, 0x0015C700u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_0015C700(o->h->ctx, a0) < 0) return a21w_latch(o, 0x0015C700u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_0015C750(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_0015C750) return a21w_latch(o, 0x0015C750u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_0015C750(o->h->ctx, a0) < 0) return a21w_latch(o, 0x0015C750u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_0019C6F0(A21W *o, int32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_0019C6F0) return a21w_latch(o, 0x0019C6F0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_0019C6F0(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x0019C6F0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001AEDE0(A21W *o, int32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001AEDE0) return a21w_latch(o, 0x001AEDE0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001AEDE0(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001AEDE0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001AFC10(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a21w_latch(o, 0x001AFC10u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001AFC10u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001B0FD0(A21W *o, uint32_t a0, int32_t *out)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return a21w_latch(o, 0x001B0FD0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return a21w_latch(o, 0x001B0FD0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001B10B0(A21W *o, uint32_t a0, int32_t a1, int32_t a2)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a21w_latch(o, 0x001B10B0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2) < 0) return a21w_latch(o, 0x001B10B0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001B17A0(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a21w_latch(o, 0x001B17A0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001B17A0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001B1B70(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001B1B70) return a21w_latch(o, 0x001B1B70u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001B1B70(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001B1B70u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001B6660(A21W *o, uint32_t a0, int32_t *out)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001B6660) return a21w_latch(o, 0x001B6660u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001B6660(o->h->ctx, a0, out) < 0) return a21w_latch(o, 0x001B6660u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001B81D0(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001B81D0) return a21w_latch(o, 0x001B81D0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001B81D0(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001B81D0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001BA1A0(A21W *o, uint32_t a0, uint32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001BA1A0) return a21w_latch(o, 0x001BA1A0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001BA1A0(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001BA1A0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001BA1C0(A21W *o, uint32_t a0, int32_t a1, int32_t *out)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001BA1C0) return a21w_latch(o, 0x001BA1C0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001BA1C0(o->h->ctx, a0, a1, out) < 0) return a21w_latch(o, 0x001BA1C0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001BA1F0(A21W *o, uint32_t a0, int32_t *out)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001BA1F0) return a21w_latch(o, 0x001BA1F0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001BA1F0(o->h->ctx, a0, out) < 0) return a21w_latch(o, 0x001BA1F0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001BA540(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001BA540) return a21w_latch(o, 0x001BA540u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001BA540(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001BA540u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001BA580(A21W *o, uint32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001BA580) return a21w_latch(o, 0x001BA580u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001BA580(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001BA580u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001BA8E0(A21W *o, uint32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001BA8E0) return a21w_latch(o, 0x001BA8E0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001BA8E0(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001BA8E0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001C6380(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001C6380) return a21w_latch(o, 0x001C6380u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001C6380u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001C63E0(A21W *o, uint32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a21w_latch(o, 0x001C63E0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001C63E0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001C64F0(A21W *o, uint32_t a0, uint32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a21w_latch(o, 0x001C64F0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a21w_float(a1)) < 0) return a21w_latch(o, 0x001C64F0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001C67E0(A21W *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001C67E0) return a21w_latch(o, 0x001C67E0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001C67E0(o->h->ctx, a0, a1, a21w_float(a2), a21w_float(a3)) < 0) return a21w_latch(o, 0x001C67E0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001C68C0(A21W *o, uint32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a21w_latch(o, 0x001C68C0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001C68C0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001CA6F0(A21W *o, uint32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001CA6F0) return a21w_latch(o, 0x001CA6F0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001CA6F0(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001CA6F0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001D2830(A21W *o, int32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001D2830) return a21w_latch(o, 0x001D2830u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001D2830(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001D2830u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001FA790(A21W *o, int32_t a0, int32_t a1)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001FA790) return a21w_latch(o, 0x001FA790u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001FA790(o->h->ctx, a0, a1) < 0) return a21w_latch(o, 0x001FA790u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001FABB0(A21W *o)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001FABB0) return a21w_latch(o, 0x001FABB0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001FABB0(o->h->ctx) < 0) return a21w_latch(o, 0x001FABB0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001FAE70(A21W *o, int32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001FAE70) return a21w_latch(o, 0x001FAE70u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001FAE70(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001FAE70u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001FB0B0(A21W *o, int32_t a0)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001FB0B0) return a21w_latch(o, 0x001FB0B0u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001FB0B0(o->h->ctx, a0) < 0) return a21w_latch(o, 0x001FB0B0u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a21w_c_001FBC50(A21W *o)
{
    if (a21w_failed(o)) return -1;
    if (!o->h->w_001FBC50) return a21w_latch(o, 0x001FBC50u, EM_AREA21_OWNERS_FAULT_NULL_WORKER);
    if (o->h->w_001FBC50(o->h->ctx) < 0) return a21w_latch(o, 0x001FBC50u, EM_AREA21_OWNERS_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
