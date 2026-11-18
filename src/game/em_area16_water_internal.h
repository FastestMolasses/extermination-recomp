/* Memory and fail-stop plumbing (em_area17_creature_internal.h's design)
 * and the translations' shared declarations. */
#ifndef EM_AREA16_WATER_INTERNAL_H
#define EM_AREA16_WATER_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area16_water.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea16WaterHooks *h;
    EmArea16WaterFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A16W;

static inline int a16w_latch(A16W *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA16_WATER_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a16w_failed(const A16W *o) { return o->fault->code != EM_AREA16_WATER_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a16w_at(A16W *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a16w_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a16w_latch(o, address, EM_AREA16_WATER_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a16w_u8(A16W *o, uint32_t a) { return *a16w_at(o, a, 1); }
static inline uint32_t a16w_u16(A16W *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a16w_at(o, a, 2), 2);
    return v;
}
static inline int32_t a16w_s16(A16W *o, uint32_t a) { return (int16_t)a16w_u16(o, a); }
static inline uint32_t a16w_u32(A16W *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a16w_at(o, a, 4), 4);
    return v;
}
static inline int32_t a16w_s32(A16W *o, uint32_t a) { return (int32_t)a16w_u32(o, a); }
static inline int32_t a16w_s8(A16W *o, uint32_t a) { return (int8_t)a16w_u8(o, a); }
static inline void a16w_w8(A16W *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a16w_at(o, a, 1), &b, 1);
}
static inline void a16w_w16(A16W *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a16w_at(o, a, 2), &h, 2);
}
static inline void a16w_w32(A16W *o, uint32_t a, uint32_t v) { memcpy(a16w_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a16w_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a16w_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a16w_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A16W_ADD(a, b) em_ee_add_bits((a), (b))
#define A16W_SUB(a, b) em_ee_sub_bits((a), (b))
#define A16W_MUL(a, b) em_ee_mul_bits((a), (b))
#define A16W_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A16W_LE(a, b) em_ee_c_le_bits((a), (b))
#define A16W_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A16W_DIV(a, b) em_ee_div_bits((a), (b))
#define A16W_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A16W_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a16w_u64(A16W *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a16w_at(o, a, 8), 8);
    return v;
}
static inline void a16w_w64(A16W *o, uint32_t a, uint64_t v) { memcpy(a16w_at(o, a, 8), &v, 8); }

static inline int a16w_begin(A16W *o, const EmArea16WaterHooks *h, EmArea16WaterFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a16w_failed(o) ? -1 : 0;
}

static inline int a16w_end(const A16W *o) { return a16w_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
/* BEGIN GENERATED WRAPPERS */
static inline int a16w_c_001026A0(A16W *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001026A0) return a16w_latch(o, 0x001026A0u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a16w_latch(o, 0x001026A0u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_00102948(A16W *o, uint32_t a0, uint32_t a1)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_00102948) return a16w_latch(o, 0x00102948u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a16w_latch(o, 0x00102948u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_00122BB8(A16W *o, int32_t *out)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a16w_latch(o, 0x00122BB8u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a16w_latch(o, 0x00122BB8u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001281C0(A16W *o, uint32_t a0, int32_t *out)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001281C0) return a16w_latch(o, 0x001281C0u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001281C0(o->h->ctx, a16w_float(a0), out) < 0) return a16w_latch(o, 0x001281C0u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001CAAC0(A16W *o, uint32_t a0, uint32_t a1)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001CAAC0) return a16w_latch(o, 0x001CAAC0u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001CAAC0(o->h->ctx, a0, a1) < 0) return a16w_latch(o, 0x001CAAC0u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001CD370(A16W *o, int32_t a0, uint32_t *out)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001CD370) return a16w_latch(o, 0x001CD370u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001CD370(o->h->ctx, a0, out) < 0) return a16w_latch(o, 0x001CD370u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001D1F20(A16W *o, int32_t a0)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001D1F20) return a16w_latch(o, 0x001D1F20u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001D1F20(o->h->ctx, a0) < 0) return a16w_latch(o, 0x001D1F20u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001D1FF0(A16W *o, int32_t a0, int32_t a1)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001D1FF0) return a16w_latch(o, 0x001D1FF0u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001D1FF0(o->h->ctx, a0, a1) < 0) return a16w_latch(o, 0x001D1FF0u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001D6B10(A16W *o, int32_t a0, int32_t a1, int32_t a2, int32_t a3)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001D6B10) return a16w_latch(o, 0x001D6B10u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001D6B10(o->h->ctx, a0, a1, a2, a3) < 0) return a16w_latch(o, 0x001D6B10u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001D6BA0(A16W *o, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001D6BA0) return a16w_latch(o, 0x001D6BA0u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001D6BA0(o->h->ctx, a0, a1, a2, a3, a4, a5) < 0) return a16w_latch(o, 0x001D6BA0u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a16w_c_001D6C90(A16W *o, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    if (a16w_failed(o)) return -1;
    if (!o->h->w_001D6C90) return a16w_latch(o, 0x001D6C90u, EM_AREA16_WATER_FAULT_NULL_WORKER);
    if (o->h->w_001D6C90(o->h->ctx, a0, a1, a2, a3, a4, a5, a6, a7) < 0) return a16w_latch(o, 0x001D6C90u, EM_AREA16_WATER_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

/* The translations (one per original function; they call each other
 * directly, as the originals do). `sp` is the original stack pointer at
 * the function's entry; `rows` are VU0 registers vf24..vf27 as 001DB480
 * loads them (lane bits, x..w). */
uint32_t a16w_001DAFA0(A16W *o);
uint32_t a16w_001DB240(A16W *o, int32_t stage);
uint32_t a16w_001DB040(A16W *o, uint32_t grid);
uint32_t a16w_001DB250(A16W *o);
uint32_t a16w_001DB480(A16W *o, uint32_t sp);
uint32_t a16w_001DAE10(A16W *o, uint32_t dst, uint32_t pos, uint32_t uv, uint32_t shade, uint32_t sp,
                       const uint32_t rows[4][4]);
uint32_t a16w_001DAC90(A16W *o, uint32_t dst, uint32_t screen, uint32_t uv, uint32_t shade);

#endif
