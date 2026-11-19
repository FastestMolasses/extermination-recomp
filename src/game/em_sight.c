/* em_sight.c - see em_sight.h. Every address in a comment is the original
 * instruction translated there; float arithmetic is em_ee_float.h's, one
 * operation per original instruction with its operands in the
 * instruction's order. */
#include "game/em_sight.h"

#include "game/em_ee_float.h"

#include <stddef.h>

/* Float constants as the instructions build them. */
#define F_0       UINT32_C(0x00000000)
#define F_0_5     UINT32_C(0x3F000000)
#define F_0_8     UINT32_C(0x3F4CCCCD)
#define F_1       UINT32_C(0x3F800000)
#define F_1_5     UINT32_C(0x3FC00000)
#define F_2       UINT32_C(0x40000000)
#define F_3       UINT32_C(0x40400000)
#define F_6       UINT32_C(0x40C00000)
#define F_M6      UINT32_C(0xC0C00000)
#define F_16      UINT32_C(0x41800000)
#define F_24      UINT32_C(0x41C00000)
#define F_M24     UINT32_C(0xC1C00000)
#define F_85      UINT32_C(0x42AA0000)
#define F_119     UINT32_C(0x42EE0000)
#define F_M119    UINT32_C(0xC2EE0000)
#define F_122     UINT32_C(0x42F40000)
#define F_M122    UINT32_C(0xC2F40000)
#define F_128     UINT32_C(0x43000000)
#define F_142     UINT32_C(0x430E0000)
#define F_180     UINT32_C(0x43340000)
#define F_195     UINT32_C(0x43430000)
#define F_200     UINT32_C(0x43480000)
#define F_203     UINT32_C(0x434B0000)
#define F_205     UINT32_C(0x434D0000)
#define F_222     UINT32_C(0x435E0000)
#define F_256     UINT32_C(0x43800000)
#define F_360     UINT32_C(0x43B40000)
#define F_90      UINT32_C(0x42B40000)
#define F_768     UINT32_C(0x44400000)
#define F_M568    UINT32_C(0xC40E0000)
#define F_2048    UINT32_C(0x45000000)
#define F_2400    UINT32_C(0x45160000)
#define F_29056   UINT32_C(0x46E30000)
#define F_32768   UINT32_C(0x47000000)
#define F_PI      UINT32_C(0x40490FDB)
#define F_PI_32   UINT32_C(0x3DC90FDB)
#define F_13_23   UINT32_C(0x3F10B216)   /* 0.5652174 */
#define F_96      UINT32_C(0x42C00000)
#define F_340     UINT32_C(0x43AA0000)
#define F_1792    UINT32_C(0x44E00000)
#define F_1936    UINT32_C(0x44F20000)
#define F_40000   UINT32_C(0x471C4000)
#define F_2PI     UINT32_C(0x40C90FDB)
#define F_MPI     UINT32_C(0xC0490FDB)
#define F_RX      UINT32_C(0x3EC99999)   /* 0.39374998, the blip's x scale */
#define F_RZ      UINT32_C(0x3EE66666)   /* 0.45, its z scale */
#define F_10_23   UINT32_C(0x3EDE9BD4)   /* 0.43478262 */

/* Original data (the ELF's .data). */
#define D_253240  UINT32_C(0x00253240)   /* 001DB830's colour quadword */
#define D_253250  UINT32_C(0x00253250)   /* 001DB9D0's block: centre x, y */
#define D_25325C  UINT32_C(0x0025325C)   /*   its radius term */
#define D_253260  UINT32_C(0x00253260)   /*   the inner vertex template */
#define D_253270  UINT32_C(0x00253270)   /*   the outer vertex template */
#define D_253280  UINT32_C(0x00253280)   /* 001DBCB0's colour quadword */
#define D_253290  UINT32_C(0x00253290)   /* 001DC610's four line templates */
#define D_2532D0  UINT32_C(0x002532D0)
#define D_253310  UINT32_C(0x00253310)
#define D_253350  UINT32_C(0x00253350)
#define D_253390  UINT32_C(0x00253390)   /* 001DC960's line template */
#define D_275670  UINT32_C(0x00275670)   /* the render context's address */
#define D_275B90  UINT32_C(0x00275B90)   /* the class-2 list cursor */
#define D_275B98  UINT32_C(0x00275B98)   /* its count (halfword) */
#define D_26A990  UINT32_C(0x0026A990)   /* 0022E7F0's heading fan corners */
#define D_26A9A0  UINT32_C(0x0026A9A0)
#define D_26A9B0  UINT32_C(0x0026A9B0)
#define D_26A9C0  UINT32_C(0x0026A9C0)   /* its three 002082B0 ring records */
#define D_26AA20  UINT32_C(0x0026AA20)
#define D_26AA80  UINT32_C(0x0026AA80)
#define D_275B54  UINT32_C(0x00275B54)   /* the radar list cursor */
#define D_275B58  UINT32_C(0x00275B58)   /* its count (halfword) */
#define S_36A0    UINT32_C(0x700036A0)
#define S_36D0    UINT32_C(0x700036D0)
#define S_36E0    UINT32_C(0x700036E0)
#define S_38C0    UINT32_C(0x700038C0)
#define S_3600    UINT32_C(0x70003600)
#define S_3610    UINT32_C(0x70003610)
#define S_3620    UINT32_C(0x70003620)
#define S_3630    UINT32_C(0x70003630)
#define S_38A0    UINT32_C(0x700038A0)
#define S_38B0    UINT32_C(0x700038B0)
#define S_38D0    UINT32_C(0x700038D0)
#define S_3AC0    UINT32_C(0x70003AC0)

/* The text runs' strings (D_0026E6F0..). */
#define T_6F0 UINT32_C(0x0026E6F0)
#define T_700 UINT32_C(0x0026E700)
#define T_718 UINT32_C(0x0026E718)
#define T_720 UINT32_C(0x0026E720)
#define T_730 UINT32_C(0x0026E730)
#define T_758 UINT32_C(0x0026E758)
#define T_760 UINT32_C(0x0026E760)
#define T_778 UINT32_C(0x0026E778)
#define T_780 UINT32_C(0x0026E780)
#define T_790 UINT32_C(0x0026E790)
#define T_798 UINT32_C(0x0026E798)
#define T_7A0 UINT32_C(0x0026E7A0)
#define T_7A8 UINT32_C(0x0026E7A8)
#define T_7B0 UINT32_C(0x0026E7B0)
#define T_7C0 UINT32_C(0x0026E7C0)
#define T_7D0 UINT32_C(0x0026E7D0)
#define T_7E8 UINT32_C(0x0026E7E8)
#define T_7F8 UINT32_C(0x0026E7F8)
#define T_800 UINT32_C(0x0026E800)
#define T_808 UINT32_C(0x0026E808)

/* Callees by original address. */
#define FN_FTOI   UINT32_C(0x001281C0)   /* float_to_int(f12) -> v0 */
#define FN_E2A8   UINT32_C(0x0011E2A8)
#define FN_DE90   UINT32_C(0x0011DE90)
#define FN_SQRT   UINT32_C(0x0011E748)
#define FN_FABS   UINT32_C(0x0011DF78)
#define FN_COPY   UINT32_C(0x00102948)   /* 16-byte copy (dst, src) */
#define FN_QDIV   UINT32_C(0x00102850)   /* (dst, src, f12) */
#define FN_ADD    UINT32_C(0x001028B8)   /* (dst, a, b) */
#define FN_SUB    UINT32_C(0x001028D0)   /* (dst, a, b) */
#define FN_APPLY  UINT32_C(0x001026A0)   /* (dst, matrix, v) */
#define FN_1F80   UINT32_C(0x001D1F80)
#define FN_64A0   UINT32_C(0x001D64A0)
#define FN_RECT   UINT32_C(0x00207F80)
#define FN_TEXT   UINT32_C(0x001CBA50)
#define FN_ACCEPT UINT32_C(0x00183B80)
#define FN_WHERE  UINT32_C(0x00183C40)
#define FN_MARK   UINT32_C(0x001DD170)
#define FN_IDENT  UINT32_C(0x001029C0)   /* (m): identity */
#define FN_NORM   UINT32_C(0x00102760)   /* (dst, src) */
#define FN_ROTY   UINT32_C(0x00102BB0)   /* (dst, src, f12) */
#define FN_MMUL   UINT32_C(0x001026D0)   /* (dst, a, b) */
#define FN_31E0   UINT32_C(0x001031E0)   /* (dst, src) */
#define FN_ATAN2  UINT32_C(0x0011E620)   /* (f12, f13) -> f0 */
#define FN_WRAP   UINT32_C(0x001B1470)   /* (f12) -> f0 */
#define FN_BLEND  UINT32_C(0x00207D00)   /* (slot, mode) */
#define FN_SPRITE UINT32_C(0x00207E40)   /* (slot, x, y, w, h, rgba, tex0) */
#define FN_FAN    UINT32_C(0x00208040)   /* (slot, a, b, c, rgba) */

#define ADD em_ee_add_bits
#define SUB em_ee_sub_bits
#define MUL em_ee_mul_bits
#define DIV em_ee_div_bits
#define CVT em_ee_cvt_s_w_bits
#define LT em_ee_c_lt_bits
#define LE em_ee_c_le_bits

/* ---- the host ------------------------------------------------------------ */

typedef struct {
    EmSight *h;
    uint32_t entry;
} Run;

static void fault(Run *r, int code, uint32_t address)
{
    if (!r->h->fault) {
        r->h->fault = code;
        r->h->fault_function = r->entry;
        r->h->fault_address = address;
    }
}

static int failed(const Run *r) { return r->h->fault != 0; }

static uint8_t *memory(Run *r, uint32_t address, uint32_t size, int write)
{
    if (r->h->fault) return NULL;
    if (!r->h->map) {
        fault(r, 1, address);
        return NULL;
    }
    uint8_t *p = r->h->map(r->h->context, address, size, write);
    if (!p) fault(r, 2, address);
    return p;
}

static uint64_t ldn(Run *r, uint32_t a, unsigned n)
{
    const uint8_t *p = memory(r, a, n, 0);
    uint64_t v = 0;
    if (p)
        for (unsigned i = 0; i < n; ++i) v |= (uint64_t)p[i] << (8 * i);
    return v;
}

static uint32_t lw(Run *r, uint32_t a) { return (uint32_t)ldn(r, a, 4); }
static uint32_t lbu(Run *r, uint32_t a) { return (uint32_t)ldn(r, a, 1); }
static int32_t lh(Run *r, uint32_t a) { return (int16_t)(uint16_t)ldn(r, a, 2); }

static void stn(Run *r, uint32_t a, uint64_t v, unsigned n)
{
    uint8_t *p = memory(r, a, n, 1);
    if (!p) return;
    for (unsigned i = 0; i < n; ++i) p[i] = (uint8_t)(v >> (8 * i));
    if (r->h->store) r->h->store(r->h->context, a, n);
}

static void sb(Run *r, uint32_t a, uint32_t v) { stn(r, a, v, 1); }
static void sh(Run *r, uint32_t a, uint32_t v) { stn(r, a, v, 2); }
static void sw(Run *r, uint32_t a, uint32_t v) { stn(r, a, v, 4); }
static void sd(Run *r, uint32_t a, uint64_t v) { stn(r, a, v, 8); }

/* A quadword store of zero (the quadword stores and loads ignore the low four address
 * bits). */
static void sq0(Run *r, uint32_t a)
{
    a &= ~15u;
    uint8_t *p = memory(r, a, 16, 1);
    if (!p) return;
    for (unsigned i = 0; i < 16; ++i) p[i] = 0;
    if (r->h->store) r->h->store(r->h->context, a, 16);
}

/* Four quadword loads from src, then four quadword stores to dst (a named
 * 0x40-byte struct copy; one quadword when n is 1). */
static void copyq(Run *r, uint32_t dst, uint32_t src, unsigned n)
{
    uint8_t q[64];
    dst &= ~15u;
    src &= ~15u;
    const uint8_t *s = memory(r, src, 16 * n, 0);
    if (!s) return;
    for (unsigned i = 0; i < 16 * n; ++i) q[i] = s[i];
    for (unsigned k = 0; k < n; ++k) {
        uint8_t *d = memory(r, dst + 16 * k, 16, 1);
        if (!d) return;
        for (unsigned i = 0; i < 16; ++i) d[i] = q[16 * k + i];
        if (r->h->store) r->h->store(r->h->context, dst + 16 * k, 16);
    }
}

static uint64_t sx(uint32_t v) { return (uint64_t)(int64_t)(int32_t)v; }
static int32_t sra(int32_t v, unsigned n) { return v < 0 ? (int32_t)~(~(uint32_t)v >> n) : (int32_t)((uint32_t)v >> n); }

/* A callee: integer argument registers a0..a3, t0..t2 (na of them) and
 * f12 (nf 0 or 1), as the instructions leave them at the jal. */
static EmAimFireTargetCall callv(Run *r, uint32_t sp, uint32_t fn, unsigned na, unsigned nf, uint32_t a0,
                                 uint32_t a1, uint32_t a2, uint32_t a3, uint32_t t0, uint32_t t1, uint32_t t2,
                                 uint32_t f12)
{
    EmAimFireTargetCall c = {fn, sp, {sx(a0), sx(a1), sx(a2), sx(a3), sx(t0), sx(t1), sx(t2)},
                             {f12, 0, 0, 0, 0, 0, 0, 0}, na, nf, 0, 0};
    for (unsigned i = na; i < 7; ++i) c.a[i] = 0;
    if (!nf) c.f[0] = 0;
    if (r->h->fault) return c;
    if (!r->h->call) fault(r, 1, fn);
    else if (r->h->call(r->h->context, &c) < 0) fault(r, 3, fn);
    return c;
}

static int32_t ftoi(Run *r, uint32_t sp, uint32_t x)
{
    return (int32_t)(uint32_t)callv(r, sp, FN_FTOI, 0, 1, 0, 0, 0, 0, 0, 0, 0, x).v0;
}
static uint32_t fcall(Run *r, uint32_t sp, uint32_t fn, uint32_t x)
{
    return callv(r, sp, fn, 0, 1, 0, 0, 0, 0, 0, 0, 0, x).f0;
}
static void c1f80(Run *r, uint32_t sp, uint32_t slot)
{
    (void)callv(r, sp, FN_1F80, 3, 0, slot, 0, 1, 0, 0, 0, 0, 0);
}
static void rect(Run *r, uint32_t sp, uint32_t slot, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                 uint32_t rgba)
{
    (void)callv(r, sp, FN_RECT, 6, 0, slot, x0, y0, x1, y1, rgba, 0, 0);
}
/* 001CBA50(1, x, y, n, n, text, style) */
static void text(Run *r, uint32_t sp, uint32_t x, uint32_t y, uint32_t n, uint32_t string, uint32_t style)
{
    (void)callv(r, sp, FN_TEXT, 7, 0, 1, x, y, n, n, string, style, 0);
}
/* 001D64A0(slot, a1, a2, a3, 0x40) */
static void sprite(Run *r, uint32_t sp, uint32_t slot, uint32_t a1, uint32_t a2, uint32_t a3)
{
    (void)callv(r, sp, FN_64A0, 5, 0, slot, a1, a2, a3, 0x40, 0, 0, 0);
}
static void call2(Run *r, uint32_t sp, uint32_t fn, uint32_t a0, uint32_t a1)
{
    (void)callv(r, sp, fn, 2, 0, a0, a1, 0, 0, 0, 0, 0, 0);
}
static void call3(Run *r, uint32_t sp, uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2)
{
    (void)callv(r, sp, fn, 3, 0, a0, a1, a2, 0, 0, 0, 0, 0);
}
static void call1(Run *r, uint32_t sp, uint32_t fn, uint32_t a0)
{
    (void)callv(r, sp, fn, 1, 0, a0, 0, 0, 0, 0, 0, 0, 0);
}
/* 0011E620(f12, f13) -> f0 */
static uint32_t atan2c(Run *r, uint32_t sp, uint32_t f12, uint32_t f13)
{
    EmAimFireTargetCall c = {FN_ATAN2, sp, {0, 0, 0, 0, 0, 0, 0}, {f12, f13, 0, 0, 0, 0, 0, 0}, 0, 2, 0, 0};
    if (r->h->fault) return 0;
    if (!r->h->call) fault(r, 1, FN_ATAN2);
    else if (r->h->call(r->h->context, &c) < 0) fault(r, 3, FN_ATAN2);
    return c.f0;
}

/* The style record the text runs share: word rgb at +0, bytes 0x80 and 0
 * at +4 / +5 (a word store, then two byte stores). */
static void style(Run *r, uint32_t at, uint32_t rgb)
{
    sw(r, at, rgb);
    sb(r, at + 4, 0x80);
    sb(r, at + 5, 0);
}

/* ======================================================================
 * The functions (internal forms take the caller's sp; each subtracts its
 * own frame)
 * ====================================================================== */

static void f2082b0(Run *r, uint32_t sp, uint32_t slot, uint32_t rec);
static void f2081a0(Run *r, uint32_t sp, uint32_t slot, uint32_t buf, int32_t prim, int32_t count,
                    uint32_t rgba);

/* 001DB830 */
static void fdb830(Run *r, uint32_t sp, uint32_t slot, uint32_t a1)
{
    static const uint32_t corner[4][4] = {{0x6FF0, 0x78F0, 0x9010, 0x7B20},
                                          {0x6FF0, 0x8710, 0x9010, 0x84E0},
                                          {0x77F0, 0x7B20, 0x8810, 0x7BA0},
                                          {0x77F0, 0x84E0, 0x8810, 0x8460}};
    sp -= 0x90;
    copyq(r, sp + 0x80, D_253240, 1);                                /* 001DB854 */
    sw(r, sp + 0x8C, a1);                                            /* 001DB868 */
    c1f80(r, sp, slot);                                              /* 001DB86C */
    for (unsigned i = 0; i < 4; ++i) {                               /* 001DB874.. */
        sw(r, sp + 0x60, corner[i][0]);
        sw(r, sp + 0x64, corner[i][1]);
        sw(r, sp + 0x68, 0x0FFFFFFFu);
        sw(r, sp + 0x70, corner[i][2]);
        sw(r, sp + 0x74, corner[i][3]);
        sw(r, sp + 0x78, 0x0FFFFFFFu);
        sprite(r, sp, slot, sp + 0x60, sp + 0x70, sp + 0x80);
    }
}

/* 001DB9D0 (NEARMISS C; the instructions followed) */
static void fdb9d0(Run *r, uint32_t sp, uint32_t slot, uint32_t dt)
{
    sp -= 0x50;
    /* the upper doubleword of the $ra save at sp + 0x40 (em_sight.h) */
    sd(r, sp + 0x48, 0);                                             /* 001DB9D8 */
    uint32_t f0 = MUL(F_M568, dt);                                   /* 001DB9F8 */
    uint32_t f20 = lw(r, D_25325C);                                  /* 001DBA10 */
    uint32_t f24 = ADD(F_768, f0);                                   /* 001DBA14: denom */
    const uint32_t f21 = fcall(r, sp, FN_E2A8, F_0);                 /* 001DBA1C */
    uint32_t f22 = fcall(r, sp, FN_DE90, F_0);                       /* 001DBA28 */
    uint32_t f25 = fcall(r, sp, FN_E2A8, F_PI_32);                   /* 001DBA3C */
    f0 = fcall(r, sp, FN_DE90, F_PI_32);                             /* 001DBA50 */
    f20 = DIV(f20, f24);                                             /* 001DBA58: scale */
    f22 = MUL(f24, f22);                                             /* 001DBA6C: x */
    uint32_t f23 = MUL(f24, f21);                                    /* 001DBA70: y */
    uint32_t acc = em_ee_mula_bits(f22, f25);                        /* 001DBA78 */
    f24 = em_ee_madd_bits(acc, f0, f23);                             /* 001DBA7C: nx */
    const uint32_t k = MUL(F_2, f25);                                /* 001DBA80 */
    acc = em_ee_mula_bits(f23, f25);                                 /* 001DBA84 */
    f25 = em_ee_msub_bits(acc, f0, f22);                             /* 001DBA8C: ny */
    c1f80(r, sp, slot);                                              /* 001DBA88 */
    const uint32_t cell = lw(r, D_275670) + (slot << 2);             /* 001DBA90 */
    sb(r, lw(r, cell + 0x10) + 3, 0x10);                             /* 001DBABC */
    sw(r, lw(r, cell + 0x10) + 4, 0);                                /* 001DBAD4 */
    sh(r, lw(r, cell + 0x10), 0x86);                                 /* 001DBADC */
    const uint32_t t = lw(r, cell + 0x10);                           /* 001DBAE0 */
    sw(r, cell + 0x10, t + 0x870);                                   /* 001DBAE8 */
    sq0(r, t + 0x10);                                                /* 001DBAF0 */
    sw(r, t + 0x1C, 0x50000085u);
    sd(r, t + 0x20, UINT64_C(0x4026400000008021));
    sd(r, t + 0x28, 0x4141);
    uint32_t p = t + 0x30;
    for (int32_t i = 0; i < 0x21; ++i) {                             /* 001DBB04 */
        if (failed(r)) return;
        uint32_t f2 = MUL(f22, f20);                                 /* 001DBB1C */
        sw(r, p + 0x00, lw(r, D_253260 + 0x0));
        f2 = MUL(F_0_8, f2);                                         /* 001DBB34 */
        sw(r, p + 0x04, lw(r, D_253260 + 0x4));
        sw(r, p + 0x08, lw(r, D_253260 + 0x8));
        sw(r, p + 0x0C, lw(r, D_253260 + 0xC));
        sw(r, p + 0x20, lw(r, D_253270 + 0x0));
        sw(r, p + 0x24, lw(r, D_253270 + 0x4));
        sw(r, p + 0x28, lw(r, D_253270 + 0x8));
        sw(r, p + 0x2C, lw(r, D_253270 + 0xC));
        uint32_t f1 = ADD(lw(r, D_253250), f2);                      /* 001DBB8C */
        sw(r, p + 0x10, (uint32_t)ftoi(r, sp, MUL(F_16, f1)));       /* 001DBB90 */
        const uint32_t f3 = MUL(f23, f20);                           /* 001DBB98 */
        f2 = MUL(F_0_5, f3);                                         /* 001DBBB4 */
        f1 = ADD(lw(r, D_253250 + 4), f2);                           /* 001DBBB8 */
        sw(r, p + 0x14, (uint32_t)ftoi(r, sp, MUL(F_16, f1)));       /* 001DBBC0 */
        sw(r, p + 0x18, 0xFFFFFFu);
        sw(r, p + 0x1C, 0);
        f1 = MUL(F_0_8, f22);                                        /* 001DBBF0 */
        f1 = ADD(f1, lw(r, D_253250));                               /* 001DBBF8 */
        sw(r, p + 0x30, (uint32_t)ftoi(r, sp, MUL(F_16, f1)));       /* 001DBC00 */
        f1 = MUL(F_0_5, f23);                                        /* 001DBC1C */
        f1 = ADD(f1, lw(r, D_253250 + 4));                           /* 001DBC24 */
        const int32_t v = ftoi(r, sp, MUL(F_16, f1));                /* 001DBC2C */
        f0 = MUL(k, f24);                                            /* 001DBC34 */
        sw(r, p + 0x34, (uint32_t)v);
        sw(r, p + 0x38, 0xFFFFFFu);
        f22 = SUB(f22, f0);                                          /* 001DBC48: x -= k nx */
        sw(r, p + 0x3C, 0);
        f0 = MUL(k, f25);
        p += 0x40;
        f23 = SUB(f23, f0);                                          /* y -= k ny */
        f24 = ADD(f24, MUL(k, f22));                                 /* nx += k x */
        f25 = ADD(f25, MUL(k, f23));                                 /* ny += k y */
    }
}

/* 001DBCB0 */
static void fdbcb0(Run *r, uint32_t sp, uint32_t slot, uint32_t f12)
{
    f12 = MUL(f12, F_2400);                                          /* 001DBCBC */
    sp -= 0x40;
    sw(r, sp + 0x20, 0x7180);
    sw(r, sp + 0x24, 0x7A90);
    sw(r, sp + 0x28, 0xFFFFFFu);
    f12 = ADD(F_29056, f12);                                         /* 001DBCF0 */
    sw(r, sp + 0x2C, 0);
    const int32_t x1 = ftoi(r, sp, f12);                             /* 001DBCF4 */
    sw(r, sp + 0x30, (uint32_t)x1);
    sw(r, sp + 0x34, 0x7AD0);
    sw(r, sp + 0x38, 0xFFFFFFu);
    sw(r, sp + 0x3C, 0);
    sprite(r, sp, slot, sp + 0x20, sp + 0x30, D_253280);            /* 001DBD2C */
}

/* 001DCD40 */
static void fdcd40(Run *r, uint32_t sp, uint32_t slot, uint32_t c)
{
    static const uint16_t bar[16][4] = {
        {0x72D0, 0x7BA0, 0x7730, 0x7BF0}, {0x72D0, 0x7BF0, 0x7320, 0x7EA0}, {0x7440, 0x7CA0, 0x75F0, 0x7CC0},
        {0x7440, 0x7CC0, 0x7480, 0x7EA0}, {0x88D0, 0x7BA0, 0x8D30, 0x7BF0}, {0x8CE0, 0x7BF0, 0x8D30, 0x7EA0},
        {0x8A10, 0x7CA0, 0x8BC0, 0x7CC0}, {0x8B80, 0x7CC0, 0x8BC0, 0x7EA0}, {0x72D0, 0x83E0, 0x7730, 0x8430},
        {0x72D0, 0x8140, 0x7320, 0x83E0}, {0x7440, 0x8320, 0x75F0, 0x8330}, {0x7440, 0x8140, 0x7480, 0x8320},
        {0x88D0, 0x83E0, 0x8D30, 0x8430}, {0x8CE0, 0x8140, 0x8D30, 0x83E0}, {0x8A10, 0x8320, 0x8BC0, 0x8330},
        {0x8B80, 0x8140, 0x8BC0, 0x8320}};
    sp -= 0x30;
    c1f80(r, sp, slot);                                              /* 001DCD5C */
    for (unsigned i = 0; i < 16; ++i) rect(r, sp, slot, bar[i][0], bar[i][1], bar[i][2], bar[i][3], c);
}

/* 001DCF40 */
static void fdcf40(Run *r, uint32_t sp, uint32_t slot, uint32_t c)
{
    static const uint16_t bar[4][4] = {
        {0x7730, 0x7F10, 0x7770, 0x80D0}, {0x7A70, 0x7F90, 0x7AB0, 0x8050},
        {0x8550, 0x7F90, 0x8590, 0x8050}, {0x8890, 0x7F10, 0x88D0, 0x80D0}};
    sp -= 0x30;
    c1f80(r, sp, slot);                                              /* 001DCF5C */
    for (unsigned i = 0; i < 4; ++i) rect(r, sp, slot, bar[i][0], bar[i][1], bar[i][2], bar[i][3], c);
}

/* 001DCFF0 */
static void fdcff0(Run *r, uint32_t sp, uint32_t slot, uint32_t c)
{
    sp -= 0x90;
    sw(r, sp + 0x30, F_32768);                                       /* 001DD004 */
    sw(r, sp + 0x34, F_32768);
    sw(r, sp + 0x38, 0);
    sw(r, sp + 0x3C, F_360);
    sw(r, sp + 0x40, F_200);
    sw(r, sp + 0x44, F_200);
    sw(r, sp + 0x48, F_203);
    sw(r, sp + 0x4C, F_203);
    /* each byte (masked to 0..255, never negative: the unsigned branch at
     * 001DD048 / 084 / 0BC / 0F4 never runs) converted from int */
    for (unsigned i = 0; i < 4; ++i) sw(r, sp + 0x50 + 4 * i, CVT((c >> (8 * i)) & 0xFFu));
    call2(r, sp, FN_COPY, sp + 0x60, sp + 0x50);                     /* 001DD118 */
    call2(r, sp, FN_COPY, sp + 0x70, sp + 0x50);
    call2(r, sp, FN_COPY, sp + 0x80, sp + 0x50);
    c1f80(r, sp, slot);                                              /* 001DD140 */
    f2082b0(r, sp, slot, sp + 0x30);                                 /* 001DD14C */
}

/* 001DBD50: the night vision scope */
static void fdbd50(Run *r, uint32_t sp, uint32_t slot)
{
    sp -= 0x30;
    fdcd40(r, sp, slot, 0x80B9E1A5u);                                /* 001DBD6C */
    fdcf40(r, sp, slot, 0x80B9E1A5u);                                /* 001DBD84 */
    style(r, sp + 0x28, 0xB9E1A5u);                                  /* 001DBD94 */
    text(r, sp, 0x863, 0x843, 0x10, T_6F0, sp + 0x28);               /* 001DBDC0 */
    text(r, sp, 0x72D, 0x7B3, 0x6, T_700, sp + 0x28);
    text(r, sp, 0x72D, 0x7B6, 0x6, T_718, sp + 0x28);
}

/* 001DBE20: the missile launcher */
static void fdbe20(Run *r, uint32_t sp, uint32_t slot)
{
    sp -= 0x30;
    fdcd40(r, sp, slot, 0x8091CDFFu);                                /* 001DBE3C */
    rect(r, sp, slot, 0x7B40, 0x7BA0, 0x84C0, 0x7BD0, 0x8091CDFFu);  /* 001DBE5C */
    rect(r, sp, slot, 0x7B40, 0x8410, 0x84C0, 0x8430, 0x8091CDFFu);
    style(r, sp + 0x28, 0x91CDFFu);                                  /* 001DBE8C */
    text(r, sp, 0x863, 0x84E, 0x10, T_720, sp + 0x28);               /* 001DBEB8 */
    text(r, sp, 0x7A0, 0x7B1, 0x6, T_730, sp + 0x28);
}

/* 001DBF00: the DELTA auto sight */
static void fdbf00(Run *r, uint32_t sp, uint32_t slot)
{
    sp -= 0x30;
    fdcff0(r, sp, slot, 0x801996FFu);                                /* 001DBF1C */
    rect(r, sp, slot, 0x7BA0, 0x7A80, 0x8460, 0x7A90, 0x801996FFu);  /* 001DBF3C */
    rect(r, sp, slot, 0x7190, 0x7AF0, 0x8E70, 0x7B20, 0x801996FFu);
    rect(r, sp, slot, 0x7190, 0x84C0, 0x8E70, 0x84F0, 0x801996FFu);
    style(r, sp + 0x28, 0x1996FFu);                                  /* 001DBF8C */
    text(r, sp, 0x723, 0x7A6, 0x10, T_758, sp + 0x28);               /* 001DBFB8 */
    text(r, sp, 0x868, 0x85D, 0x6, T_760, sp + 0x28);
    text(r, sp, 0x868, 0x860, 0x6, T_778, sp + 0x28);
}

/* 001DC020's corner to a GS coordinate: x + 0x700 and (y >> 1) + 0x790,
 * converted and times 16, through float_to_int. */
static uint32_t gs_x(Run *r, uint32_t sp, int32_t x)
{
    return (uint32_t)ftoi(r, sp, MUL(F_16, CVT((uint32_t)x + 0x700u)));
}
static uint32_t gs_y(Run *r, uint32_t sp, int32_t y)
{
    return (uint32_t)ftoi(r, sp, MUL(F_16, CVT((uint32_t)sra(y, 1) + 0x790u)));
}

/* One of 001DC020's boxes: the four 2-pixel bars of (l, top)..(r, bot). */
static void box(Run *r, uint32_t sp, uint32_t slot, int32_t l, int32_t top, int32_t rt, int32_t bot)
{
    const uint32_t c = 0x801996FFu;
    uint32_t x0, y0, x1, y1;
    x0 = gs_x(r, sp, l); y0 = gs_y(r, sp, top); x1 = gs_x(r, sp, rt); y1 = gs_y(r, sp, (int32_t)((uint32_t)top + 2));
    rect(r, sp, slot, x0, y0, x1, y1, c);
    x0 = gs_x(r, sp, l); y0 = gs_y(r, sp, bot); x1 = gs_x(r, sp, rt); y1 = gs_y(r, sp, (int32_t)((uint32_t)bot - 2));
    rect(r, sp, slot, x0, y0, x1, y1, c);
    x0 = gs_x(r, sp, l); y0 = gs_y(r, sp, top); x1 = gs_x(r, sp, (int32_t)((uint32_t)l + 2)); y1 = gs_y(r, sp, bot);
    rect(r, sp, slot, x0, y0, x1, y1, c);
    x0 = gs_x(r, sp, rt); y0 = gs_y(r, sp, top); x1 = gs_x(r, sp, (int32_t)((uint32_t)rt - 2)); y1 = gs_y(r, sp, bot);
    rect(r, sp, slot, x0, y0, x1, y1, c);
}

/* 001DC020: the DELTA auto sight's zoom boxes */
static void fdc020(Run *r, uint32_t sp, uint32_t slot, uint32_t zoom)
{
    sp -= 0xA0;
    const uint32_t t = ADD(F_13_23, MUL(F_10_23, zoom));             /* 001DC06C */
    int32_t l = ftoi(r, sp, ADD(F_256, MUL(F_M122, t)));             /* 001DC094 */
    int32_t top = ftoi(r, sp, ADD(F_222, MUL(F_M119, t)));
    int32_t rt = ftoi(r, sp, ADD(F_256, MUL(F_122, t)));
    int32_t bot = ftoi(r, sp, ADD(F_222, MUL(F_119, t)));
    box(r, sp, slot, l, top, rt, bot);                               /* 001DC10C.. */
    l = ftoi(r, sp, ADD(F_256, MUL(F_M24, t)));                      /* 001DC33C */
    top = ftoi(r, sp, ADD(F_222, MUL(F_M6, t)));
    rt = ftoi(r, sp, ADD(F_256, MUL(F_24, t)));
    bot = ftoi(r, sp, ADD(F_222, MUL(F_6, t)));
    box(r, sp, slot, l, top, rt, bot);                               /* 001DC3B4.. */
}

/* 001DC610: the interceptor unit */
static void fdc610(Run *r, uint32_t sp, uint32_t slot)
{
    sp -= 0x130;
    copyq(r, sp + 0x20, D_253290, 4);                                /* 001DC624 */
    copyq(r, sp + 0x60, D_2532D0, 4);
    copyq(r, sp + 0xA0, D_253310, 4);
    copyq(r, sp + 0xE0, D_253350, 4);
    fdcff0(r, sp, slot, 0x80CDC355u);                                /* 001DC6DC */
    rect(r, sp, slot, 0x7190, 0x85D0, 0x7910, 0x8640, 0x80CDC355u);  /* 001DC6FC */
    rect(r, sp, slot, 0x86F0, 0x85D0, 0x8E70, 0x8640, 0x80CDC355u);
    rect(r, sp, slot, 0x7FE0, 0x7FB0, 0x8010, 0x8020, 0x80CDC355u);
    rect(r, sp, slot, 0x7F90, 0x7FE0, 0x8060, 0x7FF0, 0x80CDC355u);
    style(r, sp + 0x128, 0xCDC355u);                                 /* 001DC76C */
    text(r, sp, 0x714, 0x79C, 0x10, T_780, sp + 0x128);              /* 001DC798 */
    text(r, sp, 0x778, 0x7B4, 0x6, T_790, sp + 0x128);
    text(r, sp, 0x778, 0x7B7, 0x6, T_798, sp + 0x128);
    text(r, sp, 0x778, 0x7BA, 0x6, T_7A0, sp + 0x128);
    f2081a0(r, sp, slot, sp + 0x20, 2, 4, 0x80CDC355u);              /* 001DC820 */
    f2081a0(r, sp, slot, sp + 0x60, 2, 4, 0x80CDC355u);
    f2081a0(r, sp, slot, sp + 0xA0, 2, 4, 0x80CDC355u);
    f2081a0(r, sp, slot, sp + 0xE0, 2, 4, 0x80CDC355u);
}

/* 001DC890: the interceptor unit's zoom ring */
static void fdc890(Run *r, uint32_t sp, uint32_t slot, uint32_t zoom)
{
    uint32_t f0 = MUL(F_0_5, zoom);                                  /* 001DC89C */
    sp -= 0x90;
    uint32_t f2 = ADD(F_0_5, f0);                                    /* 001DC8B0 */
    sw(r, sp + 0x30, F_32768);
    sw(r, sp + 0x34, F_32768);
    f2 = MUL(f2, F_142);                                             /* 001DC8C8 */
    sw(r, sp + 0x38, 0);
    sw(r, sp + 0x3C, F_360);
    sw(r, sp + 0x40, f2);
    f0 = ADD(F_3, f2);                                               /* 001DC8E4 */
    sw(r, sp + 0x44, f2);
    sw(r, sp + 0x48, f0);
    sw(r, sp + 0x4C, f0);
    sw(r, sp + 0x50, F_85);
    sw(r, sp + 0x54, F_195);
    sw(r, sp + 0x58, F_205);
    sw(r, sp + 0x5C, F_128);
    call2(r, sp, FN_COPY, sp + 0x60, sp + 0x50);                     /* 001DC91C */
    call2(r, sp, FN_COPY, sp + 0x70, sp + 0x50);
    call2(r, sp, FN_COPY, sp + 0x80, sp + 0x50);
    f2082b0(r, sp, slot, sp + 0x30);                                 /* 001DC940 */
}

/* 001DC960: the zoom scope */
static void fdc960(Run *r, uint32_t sp, uint32_t slot)
{
    static const uint16_t bar[14][4] = {
        {0x8630, 0x7A10, 0x8EA0, 0x7A40}, {0x7140, 0x7A40, 0x7B10, 0x7A50}, {0x7140, 0x7A60, 0x7B10, 0x7A90},
        {0x7140, 0x7A90, 0x7160, 0x7AE0}, {0x72D0, 0x7A90, 0x72F0, 0x7AB0}, {0x7460, 0x7A90, 0x7480, 0x7AB0},
        {0x75F0, 0x7A90, 0x7610, 0x7AB0}, {0x7780, 0x7A90, 0x77A0, 0x7AB0}, {0x7910, 0x7A90, 0x7930, 0x7AB0},
        {0x7AF0, 0x7A90, 0x7B10, 0x7AE0}, {0x7140, 0x79A0, 0x7380, 0x7A30}, {0x78D0, 0x79A0, 0x7B10, 0x7A30},
        {0x7FE0, 0x7FB0, 0x8010, 0x8020}, {0x7F90, 0x7FE0, 0x8060, 0x7FF0}};
    sp -= 0x90;
    copyq(r, sp + 0x40, D_253390, 4);                                /* 001DC97C */
    fdcff0(r, sp, slot, 0x8073AA0Au);                                /* 001DC9B0 */
    fdcf40(r, sp, slot, 0x8073AA0Au);                                /* 001DC9C8 */
    for (unsigned i = 0; i < 14; ++i)                                /* 001DC9E8.. */
        rect(r, sp, slot, bar[i][0], bar[i][1], bar[i][2], bar[i][3], 0x8073AA0Au);
    style(r, sp + 0x88, 0x73AA0Au);                                  /* 001DCB98 */
    text(r, sp, 0x89B, 0x798, 0x10, T_7A8, sp + 0x88);               /* 001DCBCC */
    text(r, sp, 0x8A6, 0x7A7, 0x6, T_7B0, sp + 0x88);
    text(r, sp, 0x8A6, 0x7AA, 0x6, T_7C0, sp + 0x88);
    text(r, sp, 0x885, 0x85A, 0x6, T_7D0, sp + 0x88);
    text(r, sp, 0x885, 0x85D, 0x6, T_7E8, sp + 0x88);
    text(r, sp, 0x712, 0x7AE, 0x8, T_7F8, sp + 0x88);
    text(r, sp, 0x791, 0x7AE, 0x8, T_7A8, sp + 0x88);
    style(r, sp + 0x88, 0xEBFFC8u);                                  /* 001DCCB4 */
    text(r, sp, 0x714, 0x79A, 0x10, T_800, sp + 0x88);               /* 001DCCE0 */
    text(r, sp, 0x78D, 0x79A, 0x10, T_808, sp + 0x88);
    f2081a0(r, sp, slot, sp + 0x40, 2, 4, 0x8073AA0Au);              /* 001DCD20 */
}

/* 00199770: the missile launcher's lock marks */
static void f199770(Run *r, uint32_t sp, uint32_t pl)
{
    sp -= 0x60;
    call2(r, sp, FN_COPY, S_38A0, lw(r, pl + 0x20) + 0xA0);          /* 0019979C */
    int32_t n = lh(r, D_275B98);                                     /* 001997A4 */
    uint32_t pp = lw(r, D_275B90);
    if (failed(r) || n == 0) return;
    do {
        if (failed(r)) return;
        const uint32_t e = lw(r, pp);                                /* 001997B0 */
        n = (int32_t)((uint32_t)n - 1u);
        const uint32_t used = lbu(r, e);
        pp += 4;
        if (failed(r) || used == 0) continue;
        if (callv(r, sp, FN_ACCEPT, 1, 0, e, 0, 0, 0, 0, 0, 0, 0).v0 == 0) continue;   /* 001997C4 */
        if (failed(r) || lh(r, e + 0x34) == 0 || failed(r)) continue;                  /* 001997D4 */
        call2(r, sp, FN_WHERE, e, S_38B0);                                             /* 001997E8 */
        uint32_t f4 = lw(r, S_38B0);
        const uint32_t f5 = lw(r, pl + 0xA0), f3a = lw(r, pl + 0xA4), f1a = lw(r, pl + 0xA8);
        const uint32_t f2a = lw(r, S_38B0 + 4);
        f4 = SUB(f5, f4);                                            /* 0019980C: dx */
        const uint32_t f0a = lw(r, S_38B0 + 8);
        const uint32_t f3 = SUB(f3a, f2a);                           /* dy */
        const uint32_t f2 = SUB(f1a, f0a);                           /* dz */
        const uint32_t acc = em_ee_adda_bits(MUL(f4, f4), MUL(f3, f3));          /* 00199828 */
        const uint32_t dist = fcall(r, sp, FN_SQRT, em_ee_madd_bits(acc, f2, f2)); /* 0019982C */
        if (failed(r) || !LT(dist, F_360)) continue;                 /* 00199844 */
        call2(r, sp, FN_COPY, S_38D0, S_38B0);                       /* 00199860 */
        sw(r, S_38D0 + 0xC, F_1);                                    /* 00199870 */
        call3(r, sp, FN_APPLY, S_38D0, S_3AC0, S_38D0);              /* 00199888 */
        const uint32_t w = lw(r, S_38D0 + 0xC);
        const uint32_t q = DIV(F_16, w);                             /* 001998A8 */
        if (failed(r) || LT(q, F_0)) continue;                       /* 001998B4 */
        uint32_t y = lw(r, S_38D0 + 4), x = lw(r, S_38D0);
        y = MUL(y, q);                                               /* 001998E4 */
        x = MUL(x, q);
        x = DIV(x, F_16);                                            /* 001998F0 */
        sw(r, S_38D0, SUB(x, F_2048));
        y = DIV(y, F_16);                                            /* 001998FC */
        y = SUB(y, F_2048);
        sw(r, S_38D0 + 4, y);
        sw(r, S_38D0 + 4, MUL(F_1_5, y));                            /* 00199910 */
        if (failed(r) || !LT(fcall(r, sp, FN_FABS, lw(r, S_38D0)), F_180)) continue;     /* 00199920 */
        if (failed(r) || !LT(fcall(r, sp, FN_FABS, lw(r, S_38D0 + 4)), F_90)) continue;  /* 00199948 */
        const int32_t d = ftoi(r, sp, dist);                         /* 0019996C */
        (void)callv(r, sp, FN_MARK, 5, 0, 1, S_38B0, 1, 0x80808080u, (uint32_t)d, 0, 0, 0);  /* 0019998C */
    } while (n != 0);
}

/* 0022E630: one radar blip (the player a0, the entry a1) */
static void f22e630(Run *r, uint32_t sp, uint32_t pl, uint32_t e)
{
    sp -= 0x30;
    const uint32_t f4 = lw(r, e + 0xB0), f3 = lw(r, pl + 0xA0);     /* 0022E648 */
    const uint32_t f2 = lw(r, e + 0xB8), f1 = lw(r, pl + 0xA8);
    const uint32_t dx = SUB(f4, f3), dz = SUB(f2, f1);               /* 0022E668 */
    const uint32_t d2 = em_ee_madd_bits(em_ee_mula_bits(dx, dx), dz, dz);   /* 0022E670 */
    if (failed(r) || !LE(d2, F_40000)) return;                       /* 0022E678: within 200 */
    call1(r, sp, FN_IDENT, S_36A0);                                  /* 0022E688 */
    const uint32_t yaw = ADD(F_PI, lw(r, pl + 0xC4));                /* 0022E6A4 */
    const uint32_t a = fcall(r, sp, FN_WRAP, SUB(F_2PI, yaw));       /* 0022E6B0 */
    (void)callv(r, sp, FN_ROTY, 2, 1, S_36A0, S_36A0, 0, 0, 0, 0, 0, a);   /* 0022E6C8 */
    sw(r, S_38A0, dx);                                               /* 0022E6D4 */
    sw(r, S_38A0 + 4, 0);
    sw(r, S_38A0 + 8, dz);
    sw(r, S_38A0 + 0xC, F_1);
    call3(r, sp, FN_APPLY, S_38B0, S_36A0, S_38A0);                  /* 0022E708 */
    uint32_t x = MUL(F_RX, lw(r, S_38B0));                           /* 0022E730 */
    x = ADD(F_1792, ADD(F_96, x));                                   /* 0022E738 / 40 */
    const int32_t ix = ftoi(r, sp, MUL(F_16, x));                    /* 0022E74C */
    uint32_t z = MUL(F_RZ, lw(r, S_38B0 + 8));                           /* 0022E778 */
    z = ADD(F_1936, DIV(ADD(F_340, z), F_2));                        /* 0022E780 .. 94 */
    const int32_t iz = ftoi(r, sp, MUL(F_16, z));                    /* 0022E7A4 */
    EmAimFireTargetCall c = {FN_SPRITE, sp, {1, sx((uint32_t)ix), sx((uint32_t)iz), 8, 8, sx(0x80008080u),
                                             UINT64_C(0x20041605113222CD)},
                             {0, 0, 0, 0, 0, 0, 0, 0}, 7, 0, 0, 0};
    if (failed(r)) return;
    if (!r->h->call) fault(r, 1, FN_SPRITE);                         /* 0022E7D0 */
    else if (r->h->call(r->h->context, &c) < 0) fault(r, 3, FN_SPRITE);
}

static void f2082b0(Run *r, uint32_t sp, uint32_t slot, uint32_t rec);

/* 0022E7F0: the radar of D_00810CA5 == 6 (the player) */
static void f22e7f0(Run *r, uint32_t sp, uint32_t pl)
{
    sp -= 0xB0;
    call2(r, sp, FN_BLEND, 1, 0);                                    /* 0022E810 */
    int32_t n = lh(r, D_275B58);                                     /* 0022E818 */
    uint32_t list = lw(r, D_275B54);
    if (failed(r)) return;
    if (n != 0) {
        do {
            const uint32_t e = lw(r, list);                          /* 0022E824 */
            n = (int32_t)((uint32_t)n - 1u);
            const uint32_t used = lbu(r, e);
            list += 4;
            if (failed(r)) return;
            if (used) f22e630(r, sp, pl, e);                         /* 0022E838 */
            if (failed(r)) return;
        } while (n != 0);
    }
    call1(r, sp, FN_IDENT, S_36E0);                                  /* 0022E84C */
    sw(r, S_36E0, F_0_8);                                            /* 0022E860 */
    sw(r, S_36E0 + 0x14, F_1);
    sw(r, S_36E0 + 0x28, F_0_5);
    sw(r, S_36E0 + 0x3C, 0);
    call2(r, sp, FN_COPY, sp + 0x60, lw(r, pl + 0x12C) + 0x90);      /* 0022E88C */
    sw(r, sp + 0x64, 0);                                             /* 0022E8A0 */
    call2(r, sp, FN_NORM, sp + 0x60, sp + 0x60);
    const uint32_t a0 = atan2c(r, sp, lw(r, sp + 0x68), lw(r, sp + 0x60));   /* 0022E8A8 */
    call2(r, sp, FN_COPY, sp + 0x50, pl + 0xF0);                     /* 0022E8B8 */
    sw(r, sp + 0x54, 0);                                             /* 0022E8CC */
    call2(r, sp, FN_NORM, sp + 0x50, sp + 0x50);
    const uint32_t a1 = atan2c(r, sp, lw(r, sp + 0x58), lw(r, sp + 0x50));   /* 0022E8D4 */
    uint32_t da = em_ee_neg_bits(SUB(a0, a1));                       /* 0022E8DC / F0 */
    if (LE(da, F_MPI)) {                                             /* 0022E8F4 */
        do {
            if (failed(r)) return;
            da = ADD(da, F_2PI);                                     /* 0022E910 */
        } while (LE(da, F_MPI));
    }
    if (!LE(da, F_PI)) {                                             /* 0022E940 */
        do {
            if (failed(r)) return;
            da = SUB(da, F_2PI);                                     /* 0022E95C */
        } while (!LE(da, F_PI));
    }
    call1(r, sp, FN_IDENT, sp + 0x70);                               /* 0022E97C */
    (void)callv(r, sp, FN_ROTY, 2, 1, sp + 0x70, sp + 0x70, 0, 0, 0, 0, 0, da);   /* 0022E98C */
    call3(r, sp, FN_MMUL, S_36E0, sp + 0x70, S_36E0);                /* 0022E9A4 */
    call1(r, sp, FN_IDENT, S_36A0);                                  /* 0022E9B0 */
    call3(r, sp, FN_MMUL, S_36A0, S_36E0, S_36A0);                   /* 0022E9CC */
    sw(r, S_38A0, F_96);                                             /* 0022E9DC */
    sw(r, S_38A0 + 4, 0);
    sw(r, S_38A0 + 8, F_340);
    sw(r, S_38A0 + 0xC, F_1);
    call2(r, sp, FN_31E0, S_36D0, S_38A0);                           /* 0022EA0C */
    call2(r, sp, FN_COPY, S_38A0, D_26A990);                         /* 0022EA20 */
    call2(r, sp, FN_COPY, S_38B0, D_26A9A0);
    call2(r, sp, FN_COPY, S_38C0, D_26A9B0);
    call3(r, sp, FN_APPLY, S_38A0, S_36A0, S_38A0);                  /* 0022EA64 */
    call3(r, sp, FN_APPLY, S_38B0, S_36A0, S_38B0);
    call3(r, sp, FN_APPLY, S_38C0, S_36A0, S_38C0);
    /* 0022EAA8..0022EB88: x = 16 (1792 + x), z = 16 (1936 + z / 2) for the three corners */
    uint32_t f0 = lw(r, S_38A0), f1 = lw(r, S_38B0), f2;
    f2 = ADD(F_1792, f0);                                            /* 0022EACC */
    f0 = lw(r, S_38C0);
    f2 = MUL(F_16, f2);
    sw(r, S_38A0, f2);                                               /* 0022EAEC */
    f2 = lw(r, S_38A0 + 8);
    f1 = ADD(F_1792, f1);                                            /* 0022EAFC */
    f2 = DIV(f2, F_2);                                               /* 0022EB08 */
    f2 = ADD(F_1936, f2);
    f2 = MUL(F_16, f2);
    sw(r, S_38A0 + 8, f2);                                           /* 0022EB30 */
    f1 = MUL(F_16, f1);
    f0 = ADD(F_1792, f0);                                            /* 0022EB3C */
    sw(r, S_38B0, f1);
    f1 = lw(r, S_38B0 + 8);
    f0 = MUL(F_16, f0);                                              /* 0022EB4C */
    sw(r, S_38C0, f0);
    f0 = lw(r, S_38C0 + 8);
    f1 = DIV(f1, F_2);                                               /* 0022EB60 */
    f0 = DIV(f0, F_2);
    f1 = ADD(F_1936, f1);
    f0 = ADD(F_1936, f0);
    f1 = MUL(F_16, f1);                                              /* 0022EB74 */
    sw(r, S_38B0 + 8, f1);
    f0 = MUL(F_16, f0);
    sw(r, S_38C0 + 8, f0);                                           /* 0022EB88 */
    (void)callv(r, sp, FN_FAN, 5, 0, 1, S_38A0, S_38B0, S_38C0, 0x8000FFFFu, 0, 0, 0);   /* 0022EB84 */
    f2082b0(r, sp, 1, D_26A9C0);                                     /* 0022EB94 */
    f2082b0(r, sp, 1, D_26AA20);
    f2082b0(r, sp, 1, D_26AA80);
}

/* 002082B0: the ring builder */
static void f2082b0(Run *r, uint32_t sp, uint32_t slot, uint32_t rec)
{
    sp -= 0xB0;
    uint32_t f1 = lw(r, rec + 0xC);                                  /* 002082E8 */
    uint32_t f22 = lw(r, rec + 8);
    f1 = SUB(f1, f22);                                               /* 00208300 */
    uint32_t span = DIV(f1, F_16);                                   /* 00208304 */
    int32_t v0 = ftoi(r, sp, span);                                  /* 00208310 */
    if (v0 < 0) v0 = (int32_t)(0u - (uint32_t)v0);                   /* 00208320 */
    const int32_t count = v0;
    v0 = ftoi(r, sp, span);                                          /* 00208328 */
    span = SUB(span, CVT((uint32_t)v0));                             /* 00208348 */
    span = MUL(span, F_16);                                          /* 00208358 */
    call3(r, sp, FN_SUB, S_3600, rec + 0x40, rec + 0x20);            /* 00208354 */
    call3(r, sp, FN_SUB, S_3610, rec + 0x50, rec + 0x30);            /* 00208368 */
    const uint32_t n = (uint32_t)count + 2u;
    (void)callv(r, sp, FN_QDIV, 2, 1, S_3600, S_3600, 0, 0, 0, 0, 0, CVT(n));   /* 00208388 */
    (void)callv(r, sp, FN_QDIV, 2, 1, S_3610, S_3610, 0, 0, 0, 0, 0, CVT(n));   /* 002083A8 */
    call2(r, sp, FN_COPY, S_3620, rec + 0x20);                       /* 002083B8 */
    call2(r, sp, FN_COPY, S_3630, rec + 0x30);                       /* 002083C8 */
    const uint32_t nloop = (n << 1) + 4u;                            /* 002083E0 */
    const uint32_t cell = lw(r, D_275670) + (slot << 2);             /* 002083D0 */
    sb(r, lw(r, cell + 0x10) + 3, 0x10);                             /* 002083F8 */
    sw(r, lw(r, cell + 0x10) + 4, 0);                                /* 00208418 */
    sh(r, lw(r, cell + 0x10), nloop);                                /* 00208430 */
    const uint32_t at = lw(r, cell + 0x10);                          /* 00208440 */
    sw(r, cell + 0x10, at + ((nloop + 1u) << 4));                    /* 0020845C */
    sq0(r, at + 0x10);
    sw(r, at + 0x1C, (nloop - 1u) | 0x50000000u);
    sd(r, at + 0x20, UINT64_C(0x1000000000008001));
    sd(r, at + 0x28, 0xE);
    sd(r, at + 0x30, 0x14C);
    sd(r, at + 0x38, 0);
    sd(r, at + 0x40, sx(n) | UINT64_C(0x4400000000008000));
    sd(r, at + 0x48, 0x4141);
    if (failed(r) || !((int32_t)n > 0)) return;                      /* 00208488 */
    uint32_t v = at + 0x50;
    int32_t i = 0;
    do {                                                             /* 00208490 */
        if (failed(r)) return;
        const uint32_t ang = DIV(MUL(F_PI, f22), F_180);             /* 002084B0 */
        uint32_t s = fcall(r, sp, FN_E2A8, ang);                     /* 002084BC */
        uint32_t f3 = MUL(F_16, lw(r, rec + 0x10));                  /* 002084DC */
        uint32_t f0 = MUL(F_0_8, MUL(f3, s));                        /* 002084E0 / EC */
        const uint32_t a = (uint32_t)ftoi(r, sp, ADD(lw(r, rec + 0), f0));          /* 002084F0 */
        s = fcall(r, sp, FN_DE90, ang);                              /* 002084FC */
        f3 = MUL(F_16, lw(r, rec + 0x14));
        f0 = MUL(F_0_5, MUL(f3, s));
        const uint32_t b = (uint32_t)ftoi(r, sp, ADD(lw(r, rec + 4), f0));          /* 0020852C */
        s = fcall(r, sp, FN_E2A8, ang);                              /* 00208538 */
        f3 = MUL(F_16, lw(r, rec + 0x18));
        f0 = MUL(F_0_8, MUL(f3, s));
        const uint32_t c = (uint32_t)ftoi(r, sp, ADD(lw(r, rec + 0), f0));          /* 0020856C */
        s = fcall(r, sp, FN_DE90, ang);                              /* 00208578 */
        f3 = MUL(F_16, lw(r, rec + 0x1C));
        f0 = MUL(F_0_5, MUL(f3, s));
        const uint32_t d = (uint32_t)ftoi(r, sp, ADD(lw(r, rec + 4), f0));          /* 002085A8 */
        uint32_t col = (uint32_t)ftoi(r, sp, lw(r, S_3620));                        /* 002085B8 */
        col |= (uint32_t)ftoi(r, sp, lw(r, S_3620 + 4)) << 8;
        col |= (uint32_t)ftoi(r, sp, lw(r, S_3620 + 8)) << 16;
        col |= (uint32_t)ftoi(r, sp, lw(r, S_3620 + 12)) << 24;
        sd(r, v + 0x00, sx(col));                                    /* 00208608 */
        col = (uint32_t)ftoi(r, sp, lw(r, S_3630));                  /* 00208610 */
        col |= (uint32_t)ftoi(r, sp, lw(r, S_3630 + 4)) << 8;
        col |= (uint32_t)ftoi(r, sp, lw(r, S_3630 + 8)) << 16;
        col |= (uint32_t)ftoi(r, sp, lw(r, S_3630 + 12)) << 24;
        sd(r, v + 0x10, sx(col));                                    /* 00208660 */
        const uint64_t hi = (uint64_t)0x00FFFFFFu << 32;
        sd(r, v + 0x08, sx(a | b << 16) | hi);                       /* 00208694 */
        sd(r, v + 0x18, sx(c | d << 16) | hi);                       /* 0020869C */
        call3(r, sp, FN_ADD, S_3620, S_3620, S_3600);                /* 002086B4 */
        call3(r, sp, FN_ADD, S_3630, S_3630, S_3610);                /* 002086D0 */
        f22 = i == count ? ADD(f22, span) : ADD(f22, F_16);          /* 002086D8 */
        ++i;
        v += 0x20;
    } while (i < (int32_t)((uint32_t)count + 2u));                   /* 00208700 */
}

/* 002081A0: the line-list builder */
static void f2081a0(Run *r, uint32_t sp, uint32_t slot, uint32_t buf, int32_t prim, int32_t count,
                    uint32_t rgba)
{
    (void)sp;                                                        /* a leaf: no frame */
    const uint32_t n = (uint32_t)sra(count, 1) + 4u;                 /* 002081B0 */
    const uint32_t cell = lw(r, D_275670) + (slot << 2);             /* 002081B8 */
    sb(r, lw(r, cell + 0x10) + 3, 0x10);                             /* 002081E4 */
    sw(r, lw(r, cell + 0x10) + 4, 0);                                /* 00208200 */
    sh(r, lw(r, cell + 0x10), n);                                    /* 0020821C */
    const uint32_t at = lw(r, cell + 0x10);                          /* 00208220 */
    sw(r, cell + 0x10, at + ((n + 1u) << 4));                        /* 00208238 */
    sq0(r, at + 0x10);
    sw(r, at + 0x1C, (n - 1u) | 0x50000000u);
    sd(r, at + 0x20, UINT64_C(0x2400000000008001));
    sd(r, at + 0x28, 0x10);
    sd(r, at + 0x30, sx((uint32_t)prim) | 0x140u);
    sd(r, at + 0x38, (uint64_t)rgba);
    sd(r, at + 0x40, sx((uint32_t)count) | UINT64_C(0x1400000000008000));
    sd(r, at + 0x48, 4);
    if (failed(r) || !(count > 0)) return;                           /* 00208260 */
    uint32_t q = at + 0x50;
    int32_t i = 0;
    do {                                                             /* 00208274 */
        if (failed(r)) return;
        const uint32_t hi = lw(r, buf + 4), lo = lw(r, buf + 0);
        ++i;
        sd(r, q, sx(lo | hi << 16) | (uint64_t)0x00FFFFFFu << 32);   /* 00208298 */
        q += 8;
        buf += 0x10;
    } while (i < count);
}

/* ======================================================================
 * Entries
 * ====================================================================== */

#define BEGIN(fn) Run run_ = {h, (fn)}, *r = &run_; \
    if (!h || h->fault) return -1
#define END() return failed(r) ? -1 : 0

int em_sight_001DB830(EmSight *h, int32_t slot, uint32_t a1)
{
    BEGIN(0x001DB830u);
    fdb830(r, h->sp, (uint32_t)slot, a1);
    END();
}

int em_sight_001DB9D0(EmSight *h, int32_t slot, uint32_t dt)
{
    BEGIN(0x001DB9D0u);
    fdb9d0(r, h->sp, (uint32_t)slot, dt);
    END();
}

int em_sight_001DBCB0(EmSight *h, int32_t slot, uint32_t zoom)
{
    BEGIN(0x001DBCB0u);
    fdbcb0(r, h->sp, (uint32_t)slot, zoom);
    END();
}

int em_sight_001DBD50(EmSight *h, int32_t slot)
{
    BEGIN(0x001DBD50u);
    fdbd50(r, h->sp, (uint32_t)slot);
    END();
}

int em_sight_001DBE20(EmSight *h, int32_t slot)
{
    BEGIN(0x001DBE20u);
    fdbe20(r, h->sp, (uint32_t)slot);
    END();
}

int em_sight_001DBF00(EmSight *h, int32_t slot)
{
    BEGIN(0x001DBF00u);
    fdbf00(r, h->sp, (uint32_t)slot);
    END();
}

int em_sight_001DC020(EmSight *h, int32_t slot, uint32_t zoom)
{
    BEGIN(0x001DC020u);
    fdc020(r, h->sp, (uint32_t)slot, zoom);
    END();
}

int em_sight_001DC610(EmSight *h, int32_t slot)
{
    BEGIN(0x001DC610u);
    fdc610(r, h->sp, (uint32_t)slot);
    END();
}

int em_sight_001DC890(EmSight *h, int32_t slot, uint32_t zoom)
{
    BEGIN(0x001DC890u);
    fdc890(r, h->sp, (uint32_t)slot, zoom);
    END();
}

int em_sight_001DC960(EmSight *h, int32_t slot)
{
    BEGIN(0x001DC960u);
    fdc960(r, h->sp, (uint32_t)slot);
    END();
}

int em_sight_001DCD40(EmSight *h, int32_t slot, uint32_t colour)
{
    BEGIN(0x001DCD40u);
    fdcd40(r, h->sp, (uint32_t)slot, colour);
    END();
}

int em_sight_001DCF40(EmSight *h, int32_t slot, uint32_t colour)
{
    BEGIN(0x001DCF40u);
    fdcf40(r, h->sp, (uint32_t)slot, colour);
    END();
}

int em_sight_001DCFF0(EmSight *h, int32_t slot, uint32_t colour)
{
    BEGIN(0x001DCFF0u);
    fdcff0(r, h->sp, (uint32_t)slot, colour);
    END();
}

int em_sight_00199770(EmSight *h, uint32_t player)
{
    BEGIN(0x00199770u);
    f199770(r, h->sp, player);
    END();
}

int em_sight_0022E7F0(EmSight *h, uint32_t player)
{
    BEGIN(0x0022E7F0u);
    f22e7f0(r, h->sp, player);
    END();
}

int em_sight_0022E630(EmSight *h, uint32_t player, uint32_t entry)
{
    BEGIN(0x0022E630u);
    f22e630(r, h->sp, player, entry);
    END();
}

int em_sight_002082B0(EmSight *h, int32_t slot, uint32_t ring)
{
    BEGIN(0x002082B0u);
    f2082b0(r, h->sp, (uint32_t)slot, ring);
    END();
}

int em_sight_002081A0(EmSight *h, int32_t slot, uint32_t buf, int32_t prim, int32_t count, uint32_t rgba)
{
    BEGIN(0x002081A0u);
    f2081a0(r, h->sp, (uint32_t)slot, buf, prim, count, rgba);
    END();
}

int em_sight_call(EmSight *h, const EmAimFireTargetCall *c)
{
    if (!c) return -1;
    const int32_t a0 = (int32_t)(uint32_t)c->a[0];
    const uint32_t a1 = (uint32_t)c->a[1];
    switch (c->function) {
    case 0x001DB830u: case 0x001DB9D0u: case 0x001DBCB0u: case 0x001DBD50u: case 0x001DBE20u:
    case 0x001DBF00u: case 0x001DC020u: case 0x001DC610u: case 0x001DC890u: case 0x001DC960u:
    case 0x001DCD40u: case 0x001DCF40u: case 0x001DCFF0u: case 0x00199770u: case 0x002082B0u:
    case 0x002081A0u: case 0x0022E7F0u: case 0x0022E630u:
        break;
    default:
        return 1;
    }
    if (!h) return -1;
    h->sp = c->sp;
    switch (c->function) {
    case 0x001DB830u: return em_sight_001DB830(h, a0, a1);
    case 0x001DB9D0u: return em_sight_001DB9D0(h, a0, c->f[0]);
    case 0x001DBCB0u: return em_sight_001DBCB0(h, a0, c->f[0]);
    case 0x001DBD50u: return em_sight_001DBD50(h, a0);
    case 0x001DBE20u: return em_sight_001DBE20(h, a0);
    case 0x001DBF00u: return em_sight_001DBF00(h, a0);
    case 0x001DC020u: return em_sight_001DC020(h, a0, c->f[0]);
    case 0x001DC610u: return em_sight_001DC610(h, a0);
    case 0x001DC890u: return em_sight_001DC890(h, a0, c->f[0]);
    case 0x001DC960u: return em_sight_001DC960(h, a0);
    case 0x001DCD40u: return em_sight_001DCD40(h, a0, a1);
    case 0x001DCF40u: return em_sight_001DCF40(h, a0, a1);
    case 0x001DCFF0u: return em_sight_001DCFF0(h, a0, a1);
    case 0x00199770u: return em_sight_00199770(h, (uint32_t)c->a[0]);
    case 0x002082B0u: return em_sight_002082B0(h, a0, a1);
    case 0x0022E7F0u: return em_sight_0022E7F0(h, (uint32_t)c->a[0]);
    case 0x0022E630u: return em_sight_0022E630(h, (uint32_t)c->a[0], a1);
    default:          return em_sight_002081A0(h, a0, a1, (int32_t)(uint32_t)c->a[2], (int32_t)(uint32_t)c->a[3],
                                               (uint32_t)c->a[4]);
    }
}
