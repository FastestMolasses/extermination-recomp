/* Memory and fail-stop plumbing, shared design with em_level16_boot_internal.h. */
#ifndef EM_LEVEL17_BOOT_INTERNAL_H
#define EM_LEVEL17_BOOT_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_level17_boot.h"
#include "em_ee_float.h"

typedef struct {
    const EmLevel17BootHooks *h;
    EmLevel17BootFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} L17B;

static inline int l17b_latch(L17B *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_LEVEL17_BOOT_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int l17b_failed(const L17B *o) { return o->fault->code != EM_LEVEL17_BOOT_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *l17b_at(L17B *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!l17b_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) l17b_latch(o, address, EM_LEVEL17_BOOT_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t l17b_u8(L17B *o, uint32_t a) { return *l17b_at(o, a, 1); }
static inline uint32_t l17b_u16(L17B *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, l17b_at(o, a, 2), 2);
    return v;
}
static inline int32_t l17b_s16(L17B *o, uint32_t a) { return (int16_t)l17b_u16(o, a); }
static inline uint32_t l17b_u32(L17B *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, l17b_at(o, a, 4), 4);
    return v;
}
static inline int32_t l17b_s32(L17B *o, uint32_t a) { return (int32_t)l17b_u32(o, a); }
static inline int32_t l17b_s8(L17B *o, uint32_t a) { return (int8_t)l17b_u8(o, a); }
static inline void l17b_w8(L17B *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(l17b_at(o, a, 1), &b, 1);
}
static inline void l17b_w16(L17B *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(l17b_at(o, a, 2), &h, 2);
}
static inline void l17b_w32(L17B *o, uint32_t a, uint32_t v) { memcpy(l17b_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t l17b_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float l17b_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t l17b_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define L17B_ADD(a, b) em_ee_add_bits((a), (b))
#define L17B_SUB(a, b) em_ee_sub_bits((a), (b))
#define L17B_MUL(a, b) em_ee_mul_bits((a), (b))
#define L17B_LT(a, b) em_ee_c_lt_bits((a), (b))
#define L17B_LE(a, b) em_ee_c_le_bits((a), (b))
#define L17B_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define L17B_DIV(a, b) em_ee_div_bits((a), (b))
#define L17B_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))

static inline int l17b_begin(L17B *o, const EmLevel17BootHooks *h, EmLevel17BootFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return l17b_failed(o) ? -1 : 0;
}

static inline int l17b_end(const L17B *o) { return l17b_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int l17b_method(L17B *o, uint32_t self)
{
    if (l17b_failed(o)) return -1;
    const uint32_t fn = l17b_u32(o, self + 0x4C);
    if (l17b_failed(o)) return -1;
    if (!o->h->w_callback) return l17b_latch(o, fn, EM_LEVEL17_BOOT_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return l17b_latch(o, fn, EM_LEVEL17_BOOT_FAULT_WORKER_FAILED);
    return 0;
}

/* BEGIN GENERATED WRAPPERS */
static inline int l17b_c_001AFC10(L17B *o, uint32_t a0)
{
    if (l17b_failed(o)) return -1;
    if (!o->h->w_001AFC10) return l17b_latch(o, 0x001AFC10u, EM_LEVEL17_BOOT_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return l17b_latch(o, 0x001AFC10u, EM_LEVEL17_BOOT_FAULT_WORKER_FAILED);
    return 0;
}

static inline int l17b_c_001B0FD0(L17B *o, uint32_t a0, int32_t *out)
{
    if (l17b_failed(o)) return -1;
    if (!o->h->w_001B0FD0) return l17b_latch(o, 0x001B0FD0u, EM_LEVEL17_BOOT_FAULT_NULL_WORKER);
    if (o->h->w_001B0FD0(o->h->ctx, a0, out) < 0) return l17b_latch(o, 0x001B0FD0u, EM_LEVEL17_BOOT_FAULT_WORKER_FAILED);
    return 0;
}

static inline int l17b_c_001B1B70(L17B *o, uint32_t a0)
{
    if (l17b_failed(o)) return -1;
    if (!o->h->w_001B1B70) return l17b_latch(o, 0x001B1B70u, EM_LEVEL17_BOOT_FAULT_NULL_WORKER);
    if (o->h->w_001B1B70(o->h->ctx, a0) < 0) return l17b_latch(o, 0x001B1B70u, EM_LEVEL17_BOOT_FAULT_WORKER_FAILED);
    return 0;
}

static inline int l17b_c_001C6380(L17B *o, uint32_t a0)
{
    if (l17b_failed(o)) return -1;
    if (!o->h->w_001C6380) return l17b_latch(o, 0x001C6380u, EM_LEVEL17_BOOT_FAULT_NULL_WORKER);
    if (o->h->w_001C6380(o->h->ctx, a0) < 0) return l17b_latch(o, 0x001C6380u, EM_LEVEL17_BOOT_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

#endif
