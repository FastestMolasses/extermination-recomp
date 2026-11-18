/* em_aim_fire_projectile.c - see em_aim_fire_projectile.h. Every address in
 * a comment is the original instruction translated there. */
#include "game/em_aim_fire_projectile.h"

#include "game/em_ee_float.h"

#define F_0      UINT32_C(0x00000000)
#define F_1      UINT32_C(0x3F800000)
#define F_3      UINT32_C(0x40400000)
#define F_4      UINT32_C(0x40800000)
#define F_6      UINT32_C(0x40C00000)
#define F_0_2    UINT32_C(0x3E4CCCCD)
#define F_0_03   UINT32_C(0x3CF5C28F)
#define F_300    UINT32_C(0x43960000)
#define SPAD_3B8D UINT32_C(0x70003B8D)
#define V31B0    UINT32_C(0x700031B0)
#define P31D0    UINT32_C(0x700031D0)
#define P31D4    UINT32_C(0x700031D4)
#define V38A0    UINT32_C(0x700038A0)
#define V38B0    UINT32_C(0x700038B0)

typedef struct {
    EmAimFireTarget *h;
    uint32_t sp;
} Run;

static void fault(Run *r, int code, uint32_t address)
{
    if (!r->h->fault) {
        r->h->fault = code;
        r->h->fault_function = 0x0018AF50u;
        r->h->fault_address = address;
    }
}

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

static uint32_t readn(Run *r, uint32_t a, unsigned n)
{
    const uint8_t *p = memory(r, a, n, 0);
    uint32_t v = 0;
    if (p)
        for (unsigned i = 0; i < n; ++i) v |= (uint32_t)p[i] << (8 * i);
    return v;
}

static void put(Run *r, uint32_t a, uint32_t v, unsigned n)
{
    uint8_t *p = memory(r, a, n, 1);
    if (!p) return;
    for (unsigned i = 0; i < n; ++i) p[i] = (uint8_t)(v >> (8 * i));
    if (r->h->store) r->h->store(r->h->context, a, n);
}

static uint64_t sx(uint32_t v) { return (uint64_t)(int64_t)(int32_t)v; }

/* One callee: na argument words (a0..a3, t0, t1), nf float arguments. */
static EmAimFireTargetCall call6(Run *r, uint32_t fn, unsigned na, unsigned nf, const uint32_t a[6], uint32_t f12)
{
    EmAimFireTargetCall c = {fn, r->sp, {0, 0, 0, 0, 0, 0, 0}, {f12, 0, 0, 0, 0, 0, 0, 0}, na, nf, 0, 0};
    for (unsigned i = 0; i < na; ++i) c.a[i] = sx(a[i]);
    if (r->h->fault) return c;
    if (!r->h->call) fault(r, 1, fn);
    else if (r->h->call(r->h->context, &c) < 0) fault(r, 3, fn);
    return c;
}

static EmAimFireTargetCall call(Run *r, uint32_t fn, unsigned na, unsigned nf, uint32_t a0, uint32_t a1,
                                uint32_t a2, uint32_t a3, uint32_t f12)
{
    const uint32_t a[6] = {a0, a1, a2, a3, 0, 0};
    return call6(r, fn, na, nf, a, f12);
}

/* 0018B060: the homing turn (state 1, +5 == 0). */
static void homing(Run *r, uint32_t e)
{
    int32_t t = (int16_t)readn(r, e + 0x28, 2);                     /* 0018B060 */
    put(r, e + 0x28, (uint32_t)(t - 1), 2);
    if (r->h->fault) return;
    if ((int16_t)(t - 1) == 0) {                                     /* 0018B074 */
        put(r, e + 5, readn(r, e + 5, 1) + 1, 1);                   /* 0018B088 */
        uint32_t y = readn(r, e + 0xC4, 4);                          /* 0018B08C */
        if (r->h->fault) return;
        if (em_ee_c_lt_bits(y, F_0)) {
            uint32_t v = call(r, 0x0011DF78u, 0, 1, 0, 0, 0, 0, y).f0;   /* 0018B0A0 */
            if (r->h->fault) return;
            put(r, e + 0x38, v, 4);
        }
    }
    uint32_t target = readn(r, e + 0x24, 4);                         /* 0018B0AC */
    if (r->h->fault) return;
    if (target) {
        (void)call(r, 0x00183C40u, 2, 0, target, V38B0, 0, 0, 0);   /* 0018B0BC */
        (void)call(r, 0x001028D0u, 3, 0, V38A0, V38B0, e + 0xB0, 0, 0);
        (void)call(r, 0x00102760u, 2, 0, V38A0, V38A0, 0, 0, 0);
        (void)call(r, 0x00103200u, 3, 1, V38B0, V38A0, e + 0xC0, 0, F_0_2);   /* 0018B10C */
        (void)call(r, 0x00102760u, 2, 0, e + 0xC0, V38B0, 0, 0, 0);
        (void)call(r, 0x00103230u, 2, 1, e + 0xC0, e + 0xC0, 0, 0, F_4);      /* 0018B130 */
    }
    for (unsigned i = 0; i < 3; ++i) {                               /* 0018B138 */
        uint32_t v = readn(r, e + 0xC0 + 4 * i, 4);
        uint32_t p = readn(r, e + 0xB0 + 4 * i, 4);
        put(r, e + 0xB0 + 4 * i, em_ee_add_bits(p, v), 4);
    }
}

/* 0018B16C: the fall (state 1, +5 == 1). */
static void fall(Run *r, uint32_t e)
{
    uint32_t v = readn(r, e + 0xC0, 4), p = readn(r, e + 0xB0, 4);  /* 0018B16C */
    put(r, e + 0xB0, em_ee_add_bits(p, em_ee_div_bits(v, F_3)), 4);
    v = readn(r, e + 0xC8, 4);                                       /* 0018B190 */
    p = readn(r, e + 0xB8, 4);
    put(r, e + 0xB8, em_ee_add_bits(p, em_ee_div_bits(v, F_3)), 4);
    uint32_t s = em_ee_add_bits(readn(r, e + 0x38, 4), F_0_03);      /* 0018B1B4 */
    put(r, e + 0x38, s, 4);
    if (!em_ee_c_le_bits(s, F_4)) put(r, e + 0x38, F_4, 4);          /* 0018B1C8 */
    s = readn(r, e + 0x38, 4);                                       /* 0018B1CC */
    p = readn(r, e + 0xB4, 4);
    put(r, e + 0xB4, em_ee_sub_bits(p, s), 4);
}

/* 0018B1E0: the segment, its hit response and the matrix. */
static void collide(Run *r, uint32_t e)
{
    uint32_t hit = (uint32_t)call(r, 0x0019A570u, 4, 0, e + 0xA0, e + 0xB0, 7, 0x20, 0).v0;   /* 0018B1E8 */
    if (r->h->fault) return;
    if (hit) {
        (void)call(r, 0x001031E0u, 2, 0, V38A0, V31B0, 0, 0, 0);    /* 0018B204 */
        put(r, V38A0 + 0xC, F_1, 4);                                 /* 0018B214 */
        (void)call(r, 0x00102948u, 2, 0, e + 0xB0, V38A0, 0, 0, 0); /* 0018B220 */
        if (hit - 1u < 2u) {                                         /* 0018B22C */
            uint32_t q = readn(r, P31D4, 4);                         /* 0018B23C */
            uint32_t kind = q ? readn(r, q, 1) : 0;                  /* 0018B248 */
            if (r->h->fault) return;
            if (kind) {
                uint32_t cls = readn(r, q + 2, 1);                   /* 0018B254 */
                if (r->h->fault) return;
                if ((cls & ~UINT32_C(0xE0)) == 2) {
                    if (kind == 1) {                                 /* 0018B270 */
                        uint32_t node = readn(r, P31D0, 4);          /* 0018B27C */
                        uint32_t word = readn(r, node + 0x1C, 4);    /* 0018B28C */
                        const uint32_t a[6] = {q, V38A0, e + 0xC0, word, 0x2000, 0x50};
                        if (r->h->fault) return;
                        (void)call6(r, 0x001B41F0u, 6, 0, a, 0);     /* 0018B294 */
                    }
                } else {
                    put(r, q + 0x36, 0x50, 2);                       /* 0018B2A8 */
                    (void)call(r, 0x00102948u, 2, 0, q + 0x70, e + 0xC0, 0, 0, 0);   /* 0018B2B0 */
                }
            }
        }
        put(r, e + 4, 2, 1);                                         /* 0018B2C0 */
    } else {
        uint32_t probe = (uint32_t)call(r, 0x0019B6C0u, 2, 0, e + 0xA0, e + 0xB0, 0, 0, 0).v0;   /* 0018B2C8 */
        if (r->h->fault) return;
        if (probe) {
            (void)call(r, 0x001031E0u, 2, 0, e + 0xB0, V31B0, 0, 0, 0);   /* 0018B2E0 */
            uint32_t node = readn(r, P31D0, 4);                      /* 0018B2EC */
            uint32_t surface = readn(r, node + 0x1A, 1);
            if (r->h->fault) return;
            if (surface == 0x5B)                                     /* 0018B2F8 */
                (void)call(r, 0x001E8B90u, 1, 1, e + 0xB0, 0, 0, 0, F_4);
            put(r, e + 4, 2, 1);                                     /* 0018B314 */
        }
    }
    for (unsigned i = 0; i < 4; ++i)                                 /* 0018B318 */
        put(r, e + 0xA0 + 4 * i, readn(r, e + 0xB0 + 4 * i, 4), 4);
    put(r, V38A0 + 0x0, 0, 4);                                       /* 0018B358 */
    put(r, V38A0 + 0x4, 0, 4);
    put(r, V38A0 + 0x8, 0, 4);
    put(r, V38A0 + 0xC, F_1, 4);                                     /* 0018B374 */
    (void)call(r, 0x001C94B0u, 4, 0, e + 0xD0, e + 0xB0, V38A0, V38A0, 0);   /* 0018B370 */
}

int em_aim_fire_projectile_0018AF50(EmAimFireTarget *h, uint32_t node)
{
    if (!h || h->fault) return -1;
    Run run = {h, h->sp - 0x30}, *r = &run;
    const uint32_t e = node;
    const uint32_t state = readn(r, e + 4, 1);                       /* 0018AF60 */
    if (h->fault) return -1;
    switch (state) {
    case 0: {
        put(r, e + 4, state + 1, 1);                                 /* 0018AF9C */
        put(r, e + 0x28, 0x32, 2);
        (void)call(r, 0x00103230u, 2, 1, e + 0xA0, e + 0xC0, 0, 0, F_6);        /* 0018AFB4 */
        (void)call(r, 0x001028D0u, 3, 0, e + 0xA0, e + 0xB0, e + 0xA0, 0, 0);   /* 0018AFC4 */
        (void)call(r, 0x00103230u, 2, 1, e + 0xC0, e + 0xC0, 0, 0, F_4);        /* 0018AFD8 */
        uint32_t trail = (uint32_t)call(r, 0x001EFE00u, 2, 0, 0x80000037u, e, 0, 0, 0).v0;   /* 0018AFE8 */
        put(r, e + 0x20, trail, 4);                                  /* 0018AFF0 */
        put(r, e + 0x38, F_0, 4);
        put(r, e + 0x2A, 0x12C, 2);                                  /* 0018B000 */
        break;
    }
    case 1: {
        if (readn(r, SPAD_3B8D, 1) != 0) {                           /* 0018B008 */
            uint32_t trail = readn(r, e + 0x20, 4);                  /* 0018B014 */
            if (h->fault) return -1;
            put(r, trail + 4, 2, 1);
            put(r, e + 4, 3, 1);                                     /* 0018B020 */
            break;
        }
        int32_t life = (int16_t)readn(r, e + 0x2A, 2);               /* 0018B024 */
        if (h->fault) return -1;
        if (life != 0) put(r, e + 0x2A, (uint32_t)(life - 1), 2);    /* 0018B03C */
        else put(r, e + 4, 2, 1);                                    /* 0018B034 */
        const uint32_t phase = readn(r, e + 5, 1);                   /* 0018B040 */
        if (h->fault) return -1;
        if (phase == 1) fall(r, e);
        else if (phase == 0) homing(r, e);
        if (h->fault) return -1;
        collide(r, e);
        break;
    }
    case 2: {
        put(r, e + 4, state + 1, 1);                                 /* 0018B384 */
        uint32_t trail = readn(r, e + 0x20, 4);                      /* 0018B388 */
        if (h->fault) return -1;
        put(r, trail + 4, 2, 1);                                     /* 0018B39C */
        (void)call(r, 0x001EFD20u, 2, 0, 0x80000069u, e + 0xB0, 0, 0, 0);       /* 0018B398 */
        (void)call(r, 0x001FBD50u, 3, 1, e, 0x5DE, 0, 0, F_300);     /* 0018B3B0 */
        break;
    }
    case 3:
        (void)call(r, 0x001AFC10u, 1, 0, e, 0, 0, 0, 0);             /* 0018B3C0 */
        break;
    default:
        break;
    }
    return h->fault ? -1 : 0;
}
