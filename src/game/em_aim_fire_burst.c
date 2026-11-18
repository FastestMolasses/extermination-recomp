/* em_aim_fire_burst.c - see em_aim_fire_burst.h. */
#include "game/em_aim_fire_burst.h"

#include <stdio.h>
#include <string.h>

#include "game/em_actor_pool.h"
#include "game/em_area02_misc.h"
#include "game/em_effect_001E3630.h"
#include "game/em_ee_float.h"
#include "game/em_aim_fire_runtime.h"
#include "game/em_level11_port.h"

/* The stack these nodes run on, private to the composition (0x7E000000..,
 * em_aim_fire_runtime's region): their frames hold a packet descriptor and
 * a clip matrix they write before reading. Not game RAM. */
#define BURST_SP 0x7E001400u

typedef struct {
    EmAimFireLive *live;
    uint32_t node;
    uint32_t word34;   /* the node's +0x34 word store (see finish_34) */
    int wrote34;
    uint32_t self;     /* 001E3630: the node's +0x14 (the pool's self word) */
    uint32_t sp;       /* 001E3630: its callees' stack pointer */
} Host;

static uint8_t *view(Host *h, uint32_t a, uint32_t n, int write)
{
    if (a == h->node + 0x34u) {
        if (!write || n != 4) return NULL;
        h->wrote34 = 1;
        return (uint8_t *)&h->word34;
    }
    return em_aim_fire_live_map(h->live, a, n, write);
}

/* Both nodes store a word at +0x34, the empty contact 001E49F0 / 001E4600;
 * its upper half is the record's halfword +0x36 (em_actor_pool.h). */
static void finish_34(Host *h)
{
    if (!h->wrote34) return;
    EmActor *a = em_aim_fire_pool_actor(h->node);
    if (!a) return;
    a->w34 = h->word34;
    a->h36 = (uint16_t)(h->word34 >> 16);
}

/* ---- 001E4CE0: em_area02_misc over the composition ---- */

static uint8_t *misc_view(void *ctx, uint32_t a, uint32_t n, int write) { return view(ctx, a, n, write); }

static int misc_call(void *ctx, EmArea02MiscCall *in)
{
    Host *h = ctx;
    EmAimFireTargetCall c = {0};
    if (in->na > sizeof c.a / sizeof c.a[0] || in->nf > sizeof c.f / sizeof c.f[0]) return -1;
    c.function = in->fn;
    c.sp = in->sp;
    c.na = in->na;
    c.nf = in->nf;
    memcpy(c.a, in->a, in->na * sizeof *c.a);
    memcpy(c.f, in->f, in->nf * sizeof *c.f);
    if (em_aim_fire_live_call(h->live, &c) < 0) return -1;
    in->v0 = c.v0;
    in->f0 = c.f0;
    return 0;
}

/* ---- 001E4610: em_level11_port over the composition ---- */

static uint8_t *l11_view(void *ctx, uint32_t a, uint32_t n) { return view(ctx, a, n, a == ((Host *)ctx)->node + 0x34u); }

static int forward(void *ctx, uint32_t fn, unsigned na, uint64_t a0, uint64_t a1, uint64_t a2, unsigned nf, float f0,
                   uint64_t *v0, uint32_t *f0_out)
{
    Host *h = ctx;
    EmAimFireTargetCall c = {0};
    c.function = fn;
    c.sp = BURST_SP;
    c.na = na;
    c.nf = nf;
    c.a[0] = a0;
    c.a[1] = a1;
    c.a[2] = a2;
    c.f[0] = em_ee_bits(f0);
    if (em_aim_fire_live_call(h->live, &c) < 0) return -1;
    if (v0) *v0 = c.v0;
    if (f0_out) *f0_out = c.f0;
    return 0;
}

#define ADDR(x) ((uint64_t)(int64_t)(int32_t)(x))
static int w_three(void *ctx, uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2)
{ return forward(ctx, fn, 3, ADDR(a0), ADDR(a1), ADDR(a2), 0, 0, NULL, NULL); }
static int w_001026A0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x001026A0u, a0, a1, a2); }
static int w_001028D0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x001028D0u, a0, a1, a2); }
static int w_00102918(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x00102918u, a0, a1, a2); }
static int w_00102C58(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return w_three(c, 0x00102C58u, a0, a1, a2); }
static int w_001029C0(void *c, uint32_t a0) { return forward(c, 0x001029C0u, 1, ADDR(a0), 0, 0, 0, 0, NULL, NULL); }
static int w_001AFC10(void *c, uint32_t a0) { return forward(c, 0x001AFC10u, 1, ADDR(a0), 0, 0, 0, 0, NULL, NULL); }
static int w_f1(void *c, uint32_t fn, float f0, float *result)
{
    uint32_t bits = 0;
    if (forward(c, fn, 0, 0, 0, 0, 1, f0, NULL, &bits) < 0) return -1;
    if (result) *result = em_ee_float(bits);
    return 0;
}
static int w_0011DE90(void *c, float f0, float *result) { return w_f1(c, 0x0011DE90u, f0, result); }
static int w_0011E2A8(void *c, float f0, float *result) { return w_f1(c, 0x0011E2A8u, f0, result); }
static int w_00128250(void *c, float f0, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x00128250u, 0, 0, 0, 0, 1, f0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}
static int w_001B17A0(void *c, uint32_t a0, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001B17A0u, 1, ADDR(a0), 0, 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}
static int w_001CB5F0(void *c, uint32_t a0, int32_t n1, int32_t n2, uint32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CB5F0u, 3, ADDR(a0), ADDR(n1), ADDR(n2), 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (uint32_t)v0;
    return 0;
}
static int w_001CB900(void *c, uint32_t a0, int32_t n1, int32_t n2)
{ return forward(c, 0x001CB900u, 3, ADDR(a0), ADDR(n1), ADDR(n2), 0, 0, NULL, NULL); }
static int w_001CB950(void *c, uint32_t a0, int32_t n1, uint64_t q2, uint32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CB950u, 3, ADDR(a0), ADDR(n1), q2, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (uint32_t)v0;
    return 0;
}
static int w_001CCF70(void *c, uint32_t a0, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CCF70u, 1, ADDR(a0), 0, 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}
static int w_001CD370(void *c, int32_t n0, uint32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CD370u, 1, ADDR(n0), 0, 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (uint32_t)v0;
    return 0;
}
static int w_001CF470(void *c, uint32_t a0, uint32_t a1, int32_t *result)
{
    uint64_t v0 = 0;
    if (forward(c, 0x001CF470u, 2, ADDR(a0), ADDR(a1), 0, 0, 0, &v0, NULL) < 0) return -1;
    if (result) *result = (int32_t)v0;
    return 0;
}

/* ---- 001E3630: the flamethrower burst, em_effect_001E3630 over the
 * composition. Its +0x14 is the pool's self word (001AFA90 writes the
 * record's own address); its +0x34 store goes through finish_34. ---- */

static uint8_t *flame_view(void *ctx, uint32_t a, uint32_t n, int write)
{
    Host *h = ctx;
    if (!write && a >= h->node + 0x14u && n <= 4u && a - (h->node + 0x14u) <= 4u - n)
        return (uint8_t *)&h->self + (a - (h->node + 0x14u));
    return view(h, a, n, write);
}

static int fwd(void *ctx, uint32_t fn, unsigned na, const uint64_t *a, unsigned nf, const uint32_t *f,
               uint64_t *v0, uint32_t *f0)
{
    Host *h = ctx;
    EmAimFireTargetCall c = {0};
    c.function = fn;
    c.sp = h->sp;
    c.na = na;
    c.nf = nf;
    for (unsigned k = 0; k < na; ++k) c.a[k] = a[k];
    for (unsigned k = 0; k < nf; ++k) c.f[k] = f[k];
    if (em_aim_fire_live_call(h->live, &c) < 0) return -1;
    if (v0) *v0 = c.v0;
    if (f0) *f0 = c.f0;
    return 0;
}
static int fl_2(void *c, uint32_t fn, uint32_t a0, uint32_t a1)
{
    const uint64_t a[2] = {ADDR(a0), ADDR(a1)};
    return fwd(c, fn, 2, a, 0, NULL, NULL, NULL);
}
static int fl_00102948(void *c, uint32_t a0, uint32_t a1) { return fl_2(c, 0x00102948u, a0, a1); }
static int fl_00102958(void *c, uint32_t a0, uint32_t a1) { return fl_2(c, 0x00102958u, a0, a1); }
static int fl_001CD390(void *c, uint32_t a0, uint32_t a1) { return fl_2(c, 0x001CD390u, a0, a1); }
static int fl_001EFD20(void *c, uint32_t a0, uint32_t a1) { return fl_2(c, 0x001EFD20u, a0, a1); }
static int fl_001F0460(void *c, int32_t a0, uint32_t a1) { return fl_2(c, 0x001F0460u, (uint32_t)a0, a1); }
static int fl_3(void *c, uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2)
{
    const uint64_t a[3] = {ADDR(a0), ADDR(a1), ADDR(a2)};
    return fwd(c, fn, 3, a, 0, NULL, NULL, NULL);
}
static int fl_001026A0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return fl_3(c, 0x001026A0u, a0, a1, a2); }
static int fl_001028D0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return fl_3(c, 0x001028D0u, a0, a1, a2); }
static int fl_00102918(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return fl_3(c, 0x00102918u, a0, a1, a2); }
static int fl_0011E748(void *c, uint32_t f12, uint32_t *f0) { return fwd(c, 0x0011E748u, 0, NULL, 1, &f12, NULL, f0); }
static int fl_00122BB8(void *c, int32_t *v0)
{
    uint64_t v = 0;
    if (fwd(c, 0x00122BB8u, 0, NULL, 0, NULL, &v, NULL) < 0) return -1;
    *v0 = (int32_t)v;
    return 0;
}
static int fl_0019A570(void *c, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *v0)
{
    const uint64_t a[4] = {ADDR(a0), ADDR(a1), ADDR(a2), ADDR(a3)};
    uint64_t v = 0;
    if (fwd(c, 0x0019A570u, 4, a, 0, NULL, &v, NULL) < 0) return -1;
    *v0 = (int32_t)v;
    return 0;
}
static int fl_0019B6C0(void *c, uint32_t a0, uint32_t a1, int32_t *v0)
{
    const uint64_t a[2] = {ADDR(a0), ADDR(a1)};
    uint64_t v = 0;
    if (fwd(c, 0x0019B6C0u, 2, a, 0, NULL, &v, NULL) < 0) return -1;
    *v0 = (int32_t)v;
    return 0;
}
static int fl_001AFC10(void *c, uint32_t a0) { const uint64_t a[1] = {ADDR(a0)}; return fwd(c, 0x001AFC10u, 1, a, 0, NULL, NULL, NULL); }
static int fl_001B17A0(void *c, uint32_t a0) { const uint64_t a[1] = {ADDR(a0)}; return fwd(c, 0x001B17A0u, 1, a, 0, NULL, NULL, NULL); }
static int fl_001CCF70(void *c, uint32_t a0, int32_t *v0)
{
    const uint64_t a[1] = {ADDR(a0)};
    uint64_t v = 0;
    if (fwd(c, 0x001CCF70u, 1, a, 0, NULL, &v, NULL) < 0) return -1;
    *v0 = (int32_t)v;
    return 0;
}
static int fl_001CFA60(void *c, uint32_t a0, uint32_t a1, uint32_t f12, uint32_t f13)
{
    const uint64_t a[2] = {ADDR(a0), ADDR(a1)};
    const uint32_t f[2] = {f12, f13};
    return fwd(c, 0x001CFA60u, 2, a, 2, f, NULL, NULL);
}
static int fl_001CFBE0(void *c, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t t0)
{
    const uint64_t a[5] = {ADDR(a0), ADDR(a1), ADDR(a2), ADDR(a3), ADDR(t0)};
    return fwd(c, 0x001CFBE0u, 5, a, 0, NULL, NULL, NULL);
}
static int fl_001F0190(void *c, uint32_t f12, uint32_t f13)
{
    const uint32_t f[2] = {f12, f13};
    return fwd(c, 0x001F0190u, 0, NULL, 2, f, NULL, NULL);
}
static int fl_001F0290(void *c) { return fwd(c, 0x001F0290u, 0, NULL, 0, NULL, NULL, NULL); }

static int flame_run(Host *h)
{
    EmEffect001E3630Hooks w;
    memset(&w, 0, sizeof w);
    w.ctx = h;
    w.bytes = flame_view;
    w.w_00102948 = fl_00102948;
    w.w_00102958 = fl_00102958;
    w.w_001026A0 = fl_001026A0;
    w.w_001028D0 = fl_001028D0;
    w.w_00102918 = fl_00102918;
    w.w_0011E748 = fl_0011E748;
    w.w_00122BB8 = fl_00122BB8;
    w.w_0019A570 = fl_0019A570;
    w.w_0019B6C0 = fl_0019B6C0;
    w.w_001AFC10 = fl_001AFC10;
    w.w_001B17A0 = fl_001B17A0;
    w.w_001CCF70 = fl_001CCF70;
    w.w_001CD390 = fl_001CD390;
    w.w_001CFA60 = fl_001CFA60;
    w.w_001CFBE0 = fl_001CFBE0;
    w.w_001EFD20 = fl_001EFD20;
    w.w_001F0190 = fl_001F0190;
    w.w_001F0290 = fl_001F0290;
    w.w_001F0460 = fl_001F0460;
    /* The node's frame below the composition's private stack (the burst's
     * BURST_SP); its callees run below that frame. */
    h->self = h->node;
    h->sp = BURST_SP - EM_EFFECT_001E3630_FRAME;
    EmEffect001E3630Fault fault = {0, 0};
    int rc = em_effect_001E3630(&w, h->node, BURST_SP, &fault);
    if (rc < 0)
        fprintf(stderr, "aim/fire: 001E3630 fault code %d at %08X\n", (int)fault.code, (unsigned)fault.address);
    return rc;
}

int em_aim_fire_burst_call(EmAimFireLive *live, EmAimFireTargetCall *f)
{
    if (f->function != EM_AIM_FIRE_BURST_CALLBACK && f->function != EM_AIM_FIRE_RING_CALLBACK &&
        f->function != EM_EFFECT_001E3630_CALLBACK)
        return 0;
    Host h = {live, (uint32_t)f->a[0], 0, 0, 0, 0};
    int rc;
    if (f->function == EM_EFFECT_001E3630_CALLBACK) {
        rc = flame_run(&h);
    } else if (f->function == EM_AIM_FIRE_BURST_CALLBACK) {
        EmArea02Misc m;
        memset(&m, 0, sizeof m);
        m.call = misc_call;
        m.ctx = &h;
        m.sp = BURST_SP;
        rc = em_area02_misc_view_001E4CE0(&m, misc_view, h.node);
        if (rc < 0)
            fprintf(stderr, "aim/fire: 001E4CE0 fault code %d at %08X\n", (int)m.fault, (unsigned)m.fault_address);
    } else {
        EmLevel11PortHooks w;
        memset(&w, 0, sizeof w);
        w.ctx = &h;
        w.bytes = l11_view;
        w.w_001026A0 = w_001026A0;
        w.w_001028D0 = w_001028D0;
        w.w_00102918 = w_00102918;
        w.w_001029C0 = w_001029C0;
        w.w_00102C58 = w_00102C58;
        w.w_0011DE90 = w_0011DE90;
        w.w_0011E2A8 = w_0011E2A8;
        w.w_00128250 = w_00128250;
        w.w_001AFC10 = w_001AFC10;
        w.w_001B17A0 = w_001B17A0;
        w.w_001CB5F0 = w_001CB5F0;
        w.w_001CB900 = w_001CB900;
        w.w_001CB950 = w_001CB950;
        w.w_001CCF70 = w_001CCF70;
        w.w_001CD370 = w_001CD370;
        w.w_001CF470 = w_001CF470;
        EmLevel11PortFault fault = {0, 0};
        rc = em_level11_port_001E4610(&w, h.node, BURST_SP, &fault);
        if (rc < 0)
            fprintf(stderr, "aim/fire: 001E4610 fault code %d at %08X\n", (int)fault.code, (unsigned)fault.address);
    }
    if (rc < 0) return -1;
    finish_34(&h);
    return 1;
}
