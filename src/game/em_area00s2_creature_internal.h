/* Memory and fail-stop plumbing (the design of em_level17_boot_internal.h)
 * and the translations' shared declarations. */
#ifndef EM_AREA00S2_CREATURE_INTERNAL_H
#define EM_AREA00S2_CREATURE_INTERNAL_H

#include <stdint.h>
#include <string.h>

#include "em_area00s2_creature.h"
#include "em_ee_float.h"

typedef struct {
    const EmArea00s2CreatureHooks *h;
    EmArea00s2CreatureFault *fault;
    uint8_t sink[16]; /* reads/writes after a fault land here */
} A0S2;

static inline int a0s2_latch(A0S2 *o, uint32_t address, int32_t code)
{
    if (o->fault->code == EM_AREA00S2_CREATURE_FAULT_NONE) {
        o->fault->address = address;
        o->fault->code = code;
    }
    return -1;
}

static inline int a0s2_failed(const A0S2 *o) { return o->fault->code != EM_AREA00S2_CREATURE_FAULT_NONE; }

/* The native bytes of [address, address + size). Each pointer is used
 * before the next request or hook call (the test relies on that to see
 * each access in order). After a fault: a zeroed sink, `bytes` not called. */
static inline uint8_t *a0s2_at(A0S2 *o, uint32_t address, uint32_t size)
{
    uint8_t *p = NULL;
    if (!a0s2_failed(o)) {
        p = o->h->bytes ? o->h->bytes(o->h->ctx, address, size) : NULL;
        if (!p) a0s2_latch(o, address, EM_AREA00S2_CREATURE_FAULT_BAD_ADDRESS);
    }
    if (!p) {
        memset(o->sink, 0, sizeof o->sink);
        p = o->sink;
    }
    return p;
}

static inline uint32_t a0s2_u8(A0S2 *o, uint32_t a) { return *a0s2_at(o, a, 1); }
static inline uint32_t a0s2_u16(A0S2 *o, uint32_t a)
{
    uint16_t v;
    memcpy(&v, a0s2_at(o, a, 2), 2);
    return v;
}
static inline int32_t a0s2_s16(A0S2 *o, uint32_t a) { return (int16_t)a0s2_u16(o, a); }
static inline uint32_t a0s2_u32(A0S2 *o, uint32_t a)
{
    uint32_t v;
    memcpy(&v, a0s2_at(o, a, 4), 4);
    return v;
}
static inline int32_t a0s2_s32(A0S2 *o, uint32_t a) { return (int32_t)a0s2_u32(o, a); }
static inline int32_t a0s2_s8(A0S2 *o, uint32_t a) { return (int8_t)a0s2_u8(o, a); }
static inline void a0s2_w8(A0S2 *o, uint32_t a, uint32_t v)
{
    uint8_t b = (uint8_t)v;
    memcpy(a0s2_at(o, a, 1), &b, 1);
}
static inline void a0s2_w16(A0S2 *o, uint32_t a, uint32_t v)
{
    uint16_t h = (uint16_t)v;
    memcpy(a0s2_at(o, a, 2), &h, 2);
}
static inline void a0s2_w32(A0S2 *o, uint32_t a, uint32_t v) { memcpy(a0s2_at(o, a, 4), &v, 4); }

/* A float as its bits, and back (no conversion). */
static inline uint32_t a0s2_bits(float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    return b;
}
static inline float a0s2_float(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

/* The EE's arithmetic right shift of a 32-bit register. */
static inline int32_t a0s2_sra(uint32_t v, unsigned n)
{
    return (int32_t)((v >> n) | ((v & 0x80000000u) && n ? ~(0xFFFFFFFFu >> n) : 0u));
}

/* EE arithmetic on bit patterns (em_ee_float.h). */
#define A0S2_ADD(a, b) em_ee_add_bits((a), (b))
#define A0S2_SUB(a, b) em_ee_sub_bits((a), (b))
#define A0S2_MUL(a, b) em_ee_mul_bits((a), (b))
#define A0S2_LT(a, b) em_ee_c_lt_bits((a), (b))
#define A0S2_LE(a, b) em_ee_c_le_bits((a), (b))
#define A0S2_EQ(a, b) em_ee_c_eq_bits((a), (b))
#define A0S2_DIV(a, b) em_ee_div_bits((a), (b))
#define A0S2_CVT_S_W(w) em_ee_cvt_s_w_bits((uint32_t)(w))
#define A0S2_NEG(a) em_ee_neg_bits((a))

/* Doubleword accesses (the originals' quadword and doubleword moves are
 * two or one of these). */
static inline uint64_t a0s2_u64(A0S2 *o, uint32_t a)
{
    uint64_t v;
    memcpy(&v, a0s2_at(o, a, 8), 8);
    return v;
}
static inline void a0s2_w64(A0S2 *o, uint32_t a, uint64_t v) { memcpy(a0s2_at(o, a, 8), &v, 8); }

static inline int a0s2_begin(A0S2 *o, const EmArea00s2CreatureHooks *h, EmArea00s2CreatureFault *fault)
{
    if (!h || !fault) return -1;
    o->h = h;
    o->fault = fault;
    memset(o->sink, 0, sizeof o->sink);
    return a0s2_failed(o) ? -1 : 0;
}

static inline int a0s2_end(const A0S2 *o) { return a0s2_failed(o) ? -1 : 0; }

/* One wrapper per hook. Arguments are evaluated by the caller before the
 * wrapper runs, so a fault latched while reading an argument stops the
 * call. Each returns 0 on
 * success, -1 when skipped or failed. */
static inline int a0s2_method(A0S2 *o, uint32_t self)
{
    if (a0s2_failed(o)) return -1;
    const uint32_t fn = a0s2_u32(o, self + 0x4C);
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_callback) return a0s2_latch(o, fn, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_callback(o->h->ctx, fn, self) < 0) return a0s2_latch(o, fn, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}


/* BEGIN GENERATED WRAPPERS */
static inline int a0s2_c_001026A0(A0S2 *o, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001026A0) return a0s2_latch(o, 0x001026A0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001026A0(o->h->ctx, a0, a1, a2) < 0) return a0s2_latch(o, 0x001026A0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_00102760(A0S2 *o, uint32_t a0, uint32_t a1)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_00102760) return a0s2_latch(o, 0x00102760u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_00102760(o->h->ctx, a0, a1) < 0) return a0s2_latch(o, 0x00102760u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_00102948(A0S2 *o, uint32_t a0, uint32_t a1)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_00102948) return a0s2_latch(o, 0x00102948u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_00102948(o->h->ctx, a0, a1) < 0) return a0s2_latch(o, 0x00102948u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_00102958(A0S2 *o, uint32_t a0, uint32_t a1)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_00102958) return a0s2_latch(o, 0x00102958u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_00102958(o->h->ctx, a0, a1) < 0) return a0s2_latch(o, 0x00102958u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_0011E0A8(A0S2 *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_0011E0A8) return a0s2_latch(o, 0x0011E0A8u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E0A8(o->h->ctx, a0, a0s2_float(a1), &r) < 0) return a0s2_latch(o, 0x0011E0A8u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    *out = a0s2_bits(r);
    return 0;
}

static inline int a0s2_c_0011E620(A0S2 *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_0011E620) return a0s2_latch(o, 0x0011E620u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_0011E620(o->h->ctx, a0s2_float(a0), a0s2_float(a1), &r) < 0) return a0s2_latch(o, 0x0011E620u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    *out = a0s2_bits(r);
    return 0;
}

static inline int a0s2_c_00122BB8(A0S2 *o, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_00122BB8) return a0s2_latch(o, 0x00122BB8u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_00122BB8(o->h->ctx, out) < 0) return a0s2_latch(o, 0x00122BB8u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001281C0(A0S2 *o, uint32_t a0, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001281C0) return a0s2_latch(o, 0x001281C0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001281C0(o->h->ctx, a0s2_float(a0), out) < 0) return a0s2_latch(o, 0x001281C0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001A7B80(A0S2 *o, uint32_t a0, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001A7B80) return a0s2_latch(o, 0x001A7B80u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001A7B80(o->h->ctx, a0, out) < 0) return a0s2_latch(o, 0x001A7B80u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001AFC10(A0S2 *o, uint32_t a0)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001AFC10) return a0s2_latch(o, 0x001AFC10u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001AFC10(o->h->ctx, a0) < 0) return a0s2_latch(o, 0x001AFC10u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001B10B0(A0S2 *o, uint32_t a0, int32_t a1, int32_t a2, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B10B0) return a0s2_latch(o, 0x001B10B0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001B10B0(o->h->ctx, a0, a1, a2, out) < 0) return a0s2_latch(o, 0x001B10B0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001B1190(A0S2 *o, int32_t a0)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B1190) return a0s2_latch(o, 0x001B1190u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001B1190(o->h->ctx, a0) < 0) return a0s2_latch(o, 0x001B1190u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001B12B0(A0S2 *o, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B12B0) return a0s2_latch(o, 0x001B12B0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B12B0(o->h->ctx, a0s2_float(a0), a0s2_float(a1), a0s2_float(a2), &r) < 0) return a0s2_latch(o, 0x001B12B0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    *out = a0s2_bits(r);
    return 0;
}

static inline int a0s2_c_001B1470(A0S2 *o, uint32_t a0, uint32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B1470) return a0s2_latch(o, 0x001B1470u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B1470(o->h->ctx, a0s2_float(a0), &r) < 0) return a0s2_latch(o, 0x001B1470u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    *out = a0s2_bits(r);
    return 0;
}

static inline int a0s2_c_001B15D0(A0S2 *o, uint32_t a0, uint32_t a1, uint32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B15D0) return a0s2_latch(o, 0x001B15D0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    float r = 0;
    if (o->h->w_001B15D0(o->h->ctx, a0, a1, &r) < 0) return a0s2_latch(o, 0x001B15D0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    *out = a0s2_bits(r);
    return 0;
}

static inline int a0s2_c_001B17A0(A0S2 *o, uint32_t a0, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B17A0) return a0s2_latch(o, 0x001B17A0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001B17A0(o->h->ctx, a0, out) < 0) return a0s2_latch(o, 0x001B17A0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001B4810(A0S2 *o, uint32_t a0)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B4810) return a0s2_latch(o, 0x001B4810u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001B4810(o->h->ctx, a0) < 0) return a0s2_latch(o, 0x001B4810u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001B4CF0(A0S2 *o, uint32_t a0)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B4CF0) return a0s2_latch(o, 0x001B4CF0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001B4CF0(o->h->ctx, a0) < 0) return a0s2_latch(o, 0x001B4CF0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001B55E0(A0S2 *o, uint32_t a0, int32_t a1)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001B55E0) return a0s2_latch(o, 0x001B55E0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001B55E0(o->h->ctx, a0, a1) < 0) return a0s2_latch(o, 0x001B55E0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001C63E0(A0S2 *o, uint32_t a0, int32_t a1)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001C63E0) return a0s2_latch(o, 0x001C63E0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001C63E0(o->h->ctx, a0, a1) < 0) return a0s2_latch(o, 0x001C63E0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001C64F0(A0S2 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001C64F0) return a0s2_latch(o, 0x001C64F0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001C64F0(o->h->ctx, a0, a0s2_float(a1), out) < 0) return a0s2_latch(o, 0x001C64F0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001C67E0(A0S2 *o, uint32_t a0, int32_t a1, uint32_t a2, uint32_t a3)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001C67E0) return a0s2_latch(o, 0x001C67E0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001C67E0(o->h->ctx, a0, a1, a0s2_float(a2), a0s2_float(a3)) < 0) return a0s2_latch(o, 0x001C67E0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001C68C0(A0S2 *o, uint32_t a0)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001C68C0) return a0s2_latch(o, 0x001C68C0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001C68C0(o->h->ctx, a0) < 0) return a0s2_latch(o, 0x001C68C0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001EFD90(A0S2 *o, int32_t a0, uint32_t a1, uint32_t a2)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001EFD90) return a0s2_latch(o, 0x001EFD90u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001EFD90(o->h->ctx, a0, a1, a2) < 0) return a0s2_latch(o, 0x001EFD90u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001EFE00(A0S2 *o, int32_t a0, uint32_t a1, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001EFE00) return a0s2_latch(o, 0x001EFE00u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001EFE00(o->h->ctx, a0, a1, out) < 0) return a0s2_latch(o, 0x001EFE00u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001F91C0(A0S2 *o, uint32_t a0)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001F91C0) return a0s2_latch(o, 0x001F91C0u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001F91C0(o->h->ctx, a0) < 0) return a0s2_latch(o, 0x001F91C0u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_001FBD50(A0S2 *o, uint32_t a0, int32_t a1, int32_t a2, uint32_t a3, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_001FBD50) return a0s2_latch(o, 0x001FBD50u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_001FBD50(o->h->ctx, a0, a1, a2, a0s2_float(a3), out) < 0) return a0s2_latch(o, 0x001FBD50u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}

static inline int a0s2_c_0021BE40(A0S2 *o, uint32_t a0, uint32_t a1, int32_t *out)
{
    if (a0s2_failed(o)) return -1;
    if (!o->h->w_0021BE40) return a0s2_latch(o, 0x0021BE40u, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER);
    if (o->h->w_0021BE40(o->h->ctx, a0, a1, out) < 0) return a0s2_latch(o, 0x0021BE40u, EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED);
    return 0;
}
/* END GENERATED WRAPPERS */

/* The translations (one per original function; they call each other
 * directly, as the originals do). `sp` is the function's entry stack
 * pointer where a translation below it keeps a frame local (0014E890's
 * integral part at sp - 4). */
uint32_t a0s2_0014D260(A0S2 *o, uint32_t self, uint32_t sp);
uint32_t a0s2_0014D3B0(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014D470(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp);
uint32_t a0s2_0014D5F0(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014D7C0(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014DC30(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E050(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E2C0(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E370(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp);
uint32_t a0s2_0014E420(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E4F0(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E640(A0S2 *o, uint32_t self);
uint32_t a0s2_0014E670(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E7A0(A0S2 *o, uint32_t self, uint32_t ctl);
uint32_t a0s2_0014E890(A0S2 *o, uint32_t self, uint32_t ctl, uint32_t sp);
int32_t a0s2_0014E9C0(A0S2 *o, uint32_t self, uint32_t ctl);

#endif
