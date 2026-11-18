/* The AREA11 security gun's return-visit lifecycles 4 and 1, bound live
 * (em_area11_gun_live.h; docs/SECURITY_GUN.md section 5.1). */
#include "game/em_area11_gun_live.h"

#include <stdio.h>
#include <string.h>

#include "game/em_aim_fire_binding.h"
#include "game/em_aim_fire_live.h"
#include "game/em_aim_fire_runtime.h"
#include "game/em_area02_misc.h"
#include "game/em_area11_boxes.h"
#include "game/em_area11_interaction_host.h"
#include "game/em_coll_probe_original.h"
#include "game/em_collision_world.h"
#include "game/em_ee_float.h"
#include "game/em_effect_original.h"
#include "game/em_indicator_bind_live.h"
#include "game/em_module_loader.h"
#include "game/em_owner_services_original.h"
#include "game/em_player_recovery.h"
#include "game/em_random.h"
#include "game/em_sdk_math_original.h"
#include "game/em_sdk_vu0.h"
#include "game/em_security_gun_rest.h"
#include "game/em_sfx.h"

#define TRY(x) do { if ((x) < 0) return -1; } while (0)
#define LOCAL_BASE UINT32_C(0x7F001000) /* em_aim_fire_live's call temporaries */
#define AREA11_OVERLAY_ID 9u            /* the MWo3 header word at 0x823504 */

/* Scratchpad words the gun writes before it reads them in the same call
 * and that no other port owner holds (the composition's map is asked
 * first). The scratchpad persists, so these do too. */
static struct {
    uint8_t s38A0[0x40]; /* 0x700038A0..0x700038DF */
    uint8_t s3910[0x10]; /* 0x70003910..0x7000391F */
    uint8_t s3A20[4];    /* 0x70003A20..0x70003A23 */
    uint8_t s3680[8];    /* 0x70003680..0x70003687 */
    uint8_t s3600[0x20]; /* 0x70003600..0x7000361F */
} S;

typedef struct {
    const EmArea11GunLive *g;
    uint32_t gun;              /* the gun's original address */
    uint32_t d275B40;          /* gun + 0x110 */
    uint32_t slot[4];          /* gun +0x110..+0x11F */
    uint32_t method;           /* gun +0x4C */
    EmActor *lamp;
    uint32_t lamp_address;
    uint32_t lamp_slot[4];     /* lamp +0x110..+0x11F */
    uint8_t lamp_stage[64];    /* the 00102958 into a lamp slot's +0x90 */
    int lamp_stage_k;
    uint32_t shot_node;
    uint8_t shot_100[16];
    EmActor *spawned[4];
    unsigned nspawned;
    uint32_t shadow_d0, shadow_d4;
    int has_d0, has_d4;
    uint32_t fault;
} Live;

static int fail(Live *l, uint32_t address, const char *why)
{
    if (!l->fault) {
        l->fault = address ? address : 0x00825940u;
        fprintf(stderr, "em_area11 gun live: %08X: %s\n", (unsigned)l->fault, why);
    }
    return -1;
}

static uint8_t *span(uint32_t a, uint32_t n, uint32_t base, uint32_t size, void *p)
{
    return p && n && a >= base && (uint64_t)a + n <= (uint64_t)base + size ? (uint8_t *)p + (a - base) : NULL;
}

static uint8_t *bone_field(EmOwnerBone *b, uint32_t a, uint32_t n, uint32_t base)
{
    uint8_t *p;
    if (!b) return NULL;
    if ((p = span(a, n, base + 0x70u, 12, b->rot))) return p;
    if ((p = span(a, n, base + 0x7Cu, 12, b->trans))) return p;
    return span(a, n, base + 0x90u, 64, b->world);
}

static const uint8_t *overlay(uint32_t a, uint32_t n)
{
    const EmModuleLoader *ml = em_module_loader_live();
    const uint8_t *h = ml ? em_module_loader_memory(ml, 0x00823500u, 12) : NULL;
    uint32_t id, base;
    if (!h || memcmp(h, "MWo3", 4)) return NULL;
    memcpy(&id, h + 4, 4);
    memcpy(&base, h + 8, 4);
    return id == AREA11_OVERLAY_ID && base == 0x00823500u ? em_module_loader_memory(ml, a, n) : NULL;
}

static uint8_t *map(Live *l, uint32_t a, uint32_t n, int write)
{
    uint8_t *p;
    if (l->fault || !n) return NULL;
    if (!write && (p = span(a, n, 0x00275B40u, 4, &l->d275B40))) return p;
    if (a >= l->gun && (uint64_t)a + n <= (uint64_t)l->gun + EM_ACTOR_RECORD_SIZE) {
        if ((p = span(a, n, l->gun + 0x28u, 4, l->g->h28))) return p;
        if ((p = span(a, n, l->gun + 0x1F0u, EM_ACTOR_SCRATCH_SIZE, l->g->gun->scratch))) return p;
        if (!write && (p = span(a, n, l->gun + 0x110u, 16, l->slot))) return p;
        if (!write && (p = span(a, n, l->gun + 0x4Cu, 4, &l->method))) return p;
        return em_aim_fire_binding_bytes(a, n, write); /* the pool's fields */
    }
    if (l->lamp && a >= l->lamp_address && (uint64_t)a + n <= (uint64_t)l->lamp_address + EM_ACTOR_RECORD_SIZE) {
        if (a >= l->lamp_address + 0xA0u && (uint64_t)a + n <= (uint64_t)l->lamp_address + 0xB0u) {
            float *q = l->g->node_a0 ? l->g->node_a0(l->lamp) : NULL;
            return q ? (uint8_t *)q + (a - l->lamp_address - 0xA0u) : NULL;
        }
        return write ? NULL : span(a, n, l->lamp_address + 0x110u, 16, l->lamp_slot);
    }
    for (unsigned k = 0; k < 4; ++k)
        if (l->slot[k] && a >= l->slot[k] && (uint64_t)a + n <= (uint64_t)l->slot[k] + 0xD0u)
            return bone_field(em_area11_boxes_owner_slot(l->g->gun, k, NULL), a, n, l->slot[k]);
    for (unsigned k = 0; k < 4; ++k)
        if (l->lamp_slot[k] && write && a == l->lamp_slot[k] + 0x90u && n == 64) {
            l->lamp_stage_k = (int)k;
            return l->lamp_stage;
        }
    if (l->shot_node && write && (p = span(a, n, l->shot_node + 0x100u, 16, l->shot_100))) return p;
    if (!write && (a == EM_GUN_REST_SIGHT_QUAD || a == EM_GUN_REST_SHOT_QUAD) && n == 16)
        return (uint8_t *)(uintptr_t)overlay(a, n);
    if (a == 0x700031D0u && n == 4 && (write || l->has_d0)) { l->has_d0 = 1; return (uint8_t *)&l->shadow_d0; }
    if (a == 0x700031D4u && n == 4 && (write || l->has_d4)) { l->has_d4 = 1; return (uint8_t *)&l->shadow_d4; }
    if ((p = em_aim_fire_binding_bytes(a, n, write))) return p;
    if ((p = span(a, n, 0x700038A0u, sizeof S.s38A0, S.s38A0))) return p;
    if ((p = span(a, n, 0x70003910u, sizeof S.s3910, S.s3910))) return p;
    if ((p = span(a, n, 0x70003A20u, sizeof S.s3A20, S.s3A20))) return p;
    if ((p = span(a, n, 0x70003680u, sizeof S.s3680, S.s3680))) return p;
    return span(a, n, 0x70003600u, sizeof S.s3600, S.s3600);
}

static const uint8_t *load(void *ctx, uint32_t a, uint32_t n)
{
    Live *l = ctx;
    const uint8_t *p = map(l, a, n, 0);
    if (!p) fail(l, a, "an unmapped read");
    return p;
}

static uint8_t *store(void *ctx, uint32_t a, uint32_t n)
{
    Live *l = ctx;
    uint8_t *p = map(l, a, n, 1);
    if (!p) fail(l, a, "an unmapped write");
    return p;
}

/* ---- the composition's callees (original addresses for every pointer) -- */

/* `p` (16 bytes) into the depth-0 call temporaries, slot k. */
static uint32_t local(unsigned k, const uint8_t *p)
{
    EmAimFireLive *h = em_aim_fire_binding_host();
    if (!h || h->depth || !p || 16u * (k + 1u) > sizeof h->temporary) return 0;
    memcpy(h->temporary + 16u * k, p, 16);
    return LOCAL_BASE + 16u * k;
}

static int bus(Live *l, uint32_t fn, unsigned na, const uint64_t *a, unsigned nf, const uint32_t *f,
               EmAimFireTargetCall *out)
{
    EmAimFireTargetCall c;
    memset(&c, 0, sizeof c);
    c.function = fn;
    c.na = na;
    c.nf = nf;
    for (unsigned i = 0; i < na; ++i) c.a[i] = a[i];
    for (unsigned i = 0; i < nf; ++i) c.f[i] = f[i];
    if (em_aim_fire_binding_frame(&c) < 0) return fail(l, fn, "the aim / fire composition faulted");
    if (out) *out = c;
    return 0;
}

static int w_0019AA80(void *ctx, const uint8_t *from, const uint8_t *to, int32_t a2, int32_t *result)
{
    Live *l = ctx;
    uint64_t a[3] = {local(0, from), local(1, to), (uint32_t)a2};
    EmAimFireTargetCall c;
    if (!a[0] || !a[1]) return fail(l, 0x0019AA80u, "no call temporaries");
    l->has_d0 = l->has_d4 = 0;
    TRY(bus(l, 0x0019AA80u, 3, a, 0, NULL, &c));
    *result = (int32_t)c.v0;
    return 0;
}

static int w_0019A570(void *ctx, const uint8_t *from, const uint8_t *to, int32_t a2, int32_t a3,
                      int32_t *result)
{
    Live *l = ctx;
    uint64_t a[4] = {local(0, from), local(1, to), (uint32_t)a2, (uint32_t)a3};
    EmAimFireTargetCall c;
    if (!a[0] || !a[1]) return fail(l, 0x0019A570u, "no call temporaries");
    l->has_d0 = l->has_d4 = 0;
    TRY(bus(l, 0x0019A570u, 4, a, 0, NULL, &c));
    *result = (int32_t)c.v0;
    return 0;
}

static int w_0019B6C0(void *ctx, const uint8_t *a0, const uint8_t *a1, int32_t *result)
{
    Live *l = ctx;
    uint64_t a[2] = {local(0, a0), local(1, a1)};
    EmAimFireTargetCall c;
    if (!a[0] || !a[1]) return fail(l, 0x0019B6C0u, "no call temporaries");
    l->has_d0 = l->has_d4 = 0;
    TRY(bus(l, 0x0019B6C0u, 2, a, 0, NULL, &c));
    *result = (int32_t)c.v0;
    return 0;
}

static int w_001EFD90(void *ctx, uint32_t id, const uint8_t *point, const uint8_t *at, uint32_t *node)
{
    Live *l = ctx;
    uint64_t a[3] = {id, point ? local(0, point) : 0, local(1, at)};
    EmAimFireTargetCall c;
    if ((point && !a[1]) || !a[2]) return fail(l, 0x001EFD90u, "no call temporaries");
    TRY(bus(l, 0x001EFD90u, 3, a, 0, NULL, &c));
    *node = (uint32_t)c.v0;
    return 0;
}

static int w_001CD520(void *ctx, int32_t a0, int32_t a1, const uint8_t *point, uint64_t a3, uint32_t t0,
                      uint32_t f12, uint32_t f13, uint32_t f14)
{
    Live *l = ctx;
    uint64_t a[5] = {(uint64_t)(int64_t)a0, (uint64_t)(int64_t)a1, local(0, point), a3, t0};
    uint32_t f[3] = {f12, f13, f14};
    if (!a[2]) return fail(l, 0x001CD520u, "no call temporaries");
    return bus(l, 0x001CD520u, 5, a, 3, f, NULL);
}

static int w_001E2BA0(void *ctx, const uint8_t *a0, const uint8_t *a1, const uint8_t *a2, uint32_t f12)
{
    Live *l = ctx;
    uint64_t a[3] = {local(0, a0), local(1, a1), local(2, a2)};
    if (!a[0] || !a[1] || !a[2]) return fail(l, 0x001E2BA0u, "no call temporaries");
    return bus(l, 0x001E2BA0u, 3, a, 1, &f12, NULL);
}

/* ---- the owner services (as the first-visit binding, tick_gun) --------- */

static int check_self(Live *l, uint32_t actor, uint32_t fn)
{
    return actor == l->gun ? 0 : fail(l, fn, "an owner service on a record other than the gun");
}

static int w_001C6380(void *ctx, uint32_t actor)
{
    Live *l = ctx;
    TRY(check_self(l, actor, 0x001C6380u));
    return em_area11_boxes_owner_001C6380(l->g->gun, NULL) < 0 ? fail(l, 0x001C6380u, "001C6380 failed") : 0;
}

static int w_001A2370(void *ctx, uint32_t actor, const uint8_t *matrix)
{
    Live *l = ctx;
    float m[16];
    TRY(check_self(l, actor, 0x001A2370u));
    memcpy(m, matrix, sizeof m);
    return em_collision_world_retransform_001A2370(l->g->gun, m) < 0 ? fail(l, 0x001A2370u, "001A2370 failed") : 0;
}

/* 001B17A0(self) through the interaction host's services (the placed
 * prop's view; tick_gun's gun_001B17A0). */
static int w_001B17A0(void *ctx, uint32_t actor)
{
    Live *l = ctx;
    EmActor *a = l->g->gun;
    EmOwnerServicesOwner view;
    TRY(check_self(l, actor, 0x001B17A0u));
    memset(&view, 0, sizeof view);
    view.cls = a->cls;
    view.kind = a->model;
    view.model_id = a->param;
    view.flags2 = a->flags2;
    memcpy(view.pos, a->pos, sizeof view.pos);
    return em_area11_interaction_host_offer_001B17A0(a, &view) < 0 ? fail(l, 0x001B17A0u, "001B17A0 failed") : 0;
}

static int w_draw_4C(void *ctx, uint32_t actor, uint32_t callback)
{
    Live *l = ctx;
    TRY(check_self(l, actor, callback));
    if (callback != l->method) return fail(l, callback, "+0x4C is not the bound draw method");
    return em_area11_boxes_owner_draw(l->g->gun) < 0 ? fail(l, callback, "the +0x4C draw failed") : 0;
}

static int w_001FBD50(void *ctx, uint32_t actor, int32_t cue, int32_t a2, uint32_t range)
{
    Live *l = ctx;
    TRY(check_self(l, actor, 0x001FBD50u));
    if (a2 != 0) return fail(l, 0x001FBD50u, "001FBD50 with a nonzero a2 (the flat cue)");
    em_sfx_play_at((unsigned)cue, l->g->gun->pos, em_ee_float(range));
    return 0;
}

static int w_00122BB8(void *ctx, int32_t *value)
{
    (void)ctx;
    *value = (int32_t)em_random_next();
    return 0;
}

/* 001AFA90(0xC): the shot node 0x827400 allocates; em_aim_fire_runtime
 * adopts it (its bytes become em_aim_fire_flash's) and binds it once its
 * +0x10 holds 001F5040 (after the call). A refused alloc answers 0. */
static int w_001AFA90(void *ctx, uint8_t cls, uint32_t *node)
{
    Live *l = ctx;
    *node = 0;
    if (cls != 0x0Cu || l->nspawned == sizeof l->spawned / sizeof l->spawned[0])
        return fail(l, 0x001AFA90u, "an allocation other than the shot node's");
    EmActor *a = em_actor_pool_alloc_001AFA90(l->g->pool, l->g->scene, cls);
    if (!a) return em_scene_faulted(l->g->scene) ? fail(l, 0x001AFA90u, "the pool faulted") : 0;
    if (em_aim_fire_runtime_adopt_flash(a) < 0) return fail(l, 0x001AFA90u, "the shot node was not adopted");
    l->spawned[l->nspawned++] = a;
    l->shot_node = *node = em_actor_pool_address(l->g->pool, a);
    memset(l->shot_100, 0, sizeof l->shot_100);
    return 0;
}

/* ---- the SDK leaves -------------------------------------------------- */

static EmSdkMathContext *sdk(Live *l, uint32_t fn)
{
    EmSdkMathContext *s = em_collision_world_sdk();
    if (!s) fail(l, fn, "no SDK math context");
    return s;
}

static int w_0011E2A8(void *ctx, uint32_t x, uint32_t *result)
{
    Live *l = ctx;
    EmSdkMathContext *s = sdk(l, 0x0011E2A8u);
    float r;
    if (!s || em_sdk_math_original_w_0011E2A8(s, em_ee_float(x), &r) < 0 || s->fault)
        return fail(l, 0x0011E2A8u, "0011E2A8 failed");
    *result = em_ee_bits(r);
    return 0;
}

static int w_0011DF78(void *ctx, uint32_t x, uint32_t *result)
{
    (void)ctx;
    *result = em_ee_bits(em_sdk_math_original_0011DF78(em_ee_float(x)));
    return 0;
}

static int w_0011DBB8(void *ctx, uint32_t x, uint32_t *result)
{
    Live *l = ctx;
    EmSdkMathContext *s = sdk(l, 0x0011DBB8u);
    if (!s) return -1;
    *result = em_ee_bits(em_sdk_math_original_float_0011DBB8(s, em_ee_float(x)));
    return s->fault ? fail(l, 0x0011DBB8u, "0011DBB8 failed") : 0;
}

static int w_0011E748(void *ctx, uint32_t x, uint32_t *result)
{
    Live *l = ctx;
    EmSdkMathContext *s = sdk(l, 0x0011E748u);
    if (!s) return -1;
    *result = em_ee_bits(em_sdk_math_original_float_0011E748(s, em_ee_float(x)));
    return s->fault ? fail(l, 0x0011E748u, "0011E748 failed") : 0;
}

/* 0011E520 (asinf) through em_area02_misc's translation (as
 * em_area01_gun_aux binds it): D_0026C5D0 is the SDK window's word; its
 * 0011DF78 / 0011CB90 / 0011E080 run on em_sdk_math_original. The |x| > 1
 * error path addresses the stack and four further workers, none mapped
 * here: it faults (the gun's argument lies in [0, 0.74]). */
typedef struct {
    Live *l;
    EmSdkMathContext *sdk;
} Asin;

static uint8_t *asin_view(void *ctx, uint32_t a, uint32_t n, int write)
{
    Asin *s = ctx;
    if (!write && a >= 0x0026C5D0u && (uint64_t)a + n <= 0x0026C5D4u && s->sdk->world.d26C5D0)
        return (uint8_t *)(uintptr_t)s->sdk->world.d26C5D0 + (a - 0x0026C5D0u);
    fail(s->l, a, "0011E520 reached memory outside D_0026C5D0");
    return NULL;
}

static int asin_call(void *ctx, EmArea02MiscCall *c)
{
    Asin *s = ctx;
    if (c->nf == 1) {
        const float x = em_ee_float(c->f[0]);
        if (c->fn == 0x0011DF78u) { c->f0 = em_ee_bits(em_sdk_math_original_0011DF78(x)); return 0; }
        if (c->fn == 0x0011CB90u) { c->f0 = em_ee_bits(em_sdk_math_original_0011CB90(x)); return 0; }
        if (c->fn == 0x0011E080u) {
            c->v0 = (uint64_t)(int64_t)em_sdk_math_original_0011E080(x);
            return 0;
        }
    }
    return fail(s->l, c->fn, "0011E520 reached an unbound callee (its |x| > 1 path)");
}

static int w_0011E520(void *ctx, uint32_t x, uint32_t *result)
{
    Live *l = ctx;
    Asin s = {l, sdk(l, 0x0011E520u)};
    if (!s.sdk) return -1;
    EmArea02Misc misc;
    memset(&misc, 0, sizeof misc);
    misc.call = asin_call;
    misc.ctx = &s;
    misc.sp = LOCAL_BASE + 0x400u;
    if (em_area02_misc_view_0011E520(&misc, asin_view, x, result) < 0 || misc.fault)
        return fail(l, misc.fault_address ? misc.fault_address : 0x0011E520u, "0011E520 faulted");
    return 0;
}

static int w_001B1470(void *ctx, uint32_t x, uint32_t *result)
{
    Live *l = ctx;
    return em_player_recovery_wrap(x, result) < 0 ? fail(l, 0x001B1470u, "001B1470 failed") : 0;
}

static int w_00102948(void *ctx, uint8_t *dst, const uint8_t *src)
{
    (void)ctx;
    uint8_t q[16];
    memcpy(q, src, 16);
    em_sdk_vu0_00102948(dst, q);
    return 0;
}

/* 00102958: all four row loads precede the stores (em_aim_fire_sdk_memory);
 * into a lamp slot's +0x90 it is em_indicator_bind_live_set_node. */
static int w_00102958(void *ctx, uint8_t *dst, const uint8_t *src)
{
    Live *l = ctx;
    float in[16], out[16];
    memcpy(in, src, sizeof in);
    em_owner_services_copy_qw4_00102958(out, in);
    memcpy(dst, out, sizeof out);
    if (dst == l->lamp_stage) {
        if (!l->lamp || em_indicator_bind_live_set_node(l->lamp, (unsigned)l->lamp_stage_k, out) < 0)
            return fail(l, 0x00102958u, "the lamp's slot copy failed");
    }
    return 0;
}

static int w_001026A0(void *ctx, uint8_t *dst, const uint8_t *m, const uint8_t *v)
{
    (void)ctx;
    float mm[16], vv[4], out[4];
    memcpy(mm, m, sizeof mm);
    memcpy(vv, v, sizeof vv);
    em_effect_original_001026A0(out, mm, vv);
    memcpy(dst, out, sizeof out);
    return 0;
}

static int addsub(Live *l, uint32_t fn, uint8_t *dst, const uint8_t *a, const uint8_t *b)
{
    float x[4], y[4], out[4];
    memcpy(x, a, sizeof x);
    memcpy(y, b, sizeof y);
    if ((fn == 0x001028B8u ? em_coll_probe_sdk_add(out, x, y) : em_coll_probe_sdk_sub(out, x, y)) < 0)
        return fail(l, fn, "a vector leaf failed");
    memcpy(dst, out, sizeof out);
    return 0;
}

static int w_001028B8(void *ctx, uint8_t *dst, const uint8_t *a, const uint8_t *b)
{ return addsub(ctx, 0x001028B8u, dst, a, b); }

static int w_001028D0(void *ctx, uint8_t *dst, const uint8_t *a, const uint8_t *b)
{ return addsub(ctx, 0x001028D0u, dst, a, b); }

static int w_00102738(void *ctx, const uint8_t *a, const uint8_t *b, uint32_t *f0)
{
    Live *l = ctx;
    float x[4], y[4], out;
    memcpy(x, a, sizeof x);
    memcpy(y, b, sizeof y);
    if (em_coll_probe_sdk_dot(&out, x, y) < 0) return fail(l, 0x00102738u, "00102738 failed");
    *f0 = em_ee_bits(out);
    return 0;
}

static int w_00102760(void *ctx, uint8_t *dst, const uint8_t *src)
{
    (void)ctx;
    float in[4], out[4];
    memcpy(in, src, sizeof in);
    em_effect_original_00102760(out, in);
    memcpy(dst, out, sizeof out);
    return 0;
}

/* 001031E0: three word copies in order (a forward overlap propagates). */
static int w_001031E0(void *ctx, uint8_t *dst, const uint8_t *src)
{
    (void)ctx;
    for (unsigned i = 0; i < 3; ++i) memmove(dst + 4 * i, src + 4 * i, 4);
    return 0;
}

int em_area11_gun_live_tick(const EmArea11GunLive *g, EmGunFault *fault)
{
    if (!g || !g->pool || !g->scene || !g->gun || !g->h28 || !fault) return -1;
    if (!em_aim_fire_binding_ready()) {
        fault->address = 0x00825940u;
        fault->code = EM_GUN_FAULT_NULL;
        fprintf(stderr, "em_area11 gun live: the aim / fire composition is not configured\n");
        return -1;
    }
    Live l;
    memset(&l, 0, sizeof l);
    l.g = g;
    l.gun = em_actor_pool_address(g->pool, g->gun);
    l.d275B40 = l.gun + 0x110u;
    for (unsigned k = 0; k < 4; ++k) (void)em_area11_boxes_owner_slot(g->gun, k, &l.slot[k]);
    uint32_t anim = 0, model = 0;
    (void)em_area11_boxes_owner_fields(g->gun, &anim, &model, &l.method);
    memcpy(&l.lamp_address, g->gun->scratch + (0x220u - 0x1F0u), 4);
    l.lamp = l.lamp_address ? em_aim_fire_pool_actor(l.lamp_address) : NULL;
    if (l.lamp && !l.lamp->allocated) l.lamp = NULL;
    if (l.lamp) {
        EmIndicatorBindRecord rec;
        if (em_indicator_bind_live_record(l.lamp, &rec) == 1)
            memcpy(l.lamp_slot, rec.slot, sizeof l.lamp_slot);
    }
    const EmGunRestMem mem = {&l, load, store};
    EmGunRestWorkers w;
    memset(&w, 0, sizeof w);
    w.ctx = &l;
    w.w_001C6380 = w_001C6380;
    w.w_001A2370 = w_001A2370;
    w.w_001B17A0 = w_001B17A0;
    w.w_draw_4C = w_draw_4C;
    w.w_001AFA90 = w_001AFA90;
    w.w_001FBD50 = w_001FBD50;
    w.w_00122BB8 = w_00122BB8;
    w.w_0011E2A8 = w_0011E2A8;
    w.w_0011DF78 = w_0011DF78;
    w.w_0011DBB8 = w_0011DBB8;
    w.w_0011E748 = w_0011E748;
    w.w_0011E520 = w_0011E520;
    w.w_001B1470 = w_001B1470;
    w.w_00102948 = w_00102948;
    w.w_00102958 = w_00102958;
    w.w_001026A0 = w_001026A0;
    w.w_001028B8 = w_001028B8;
    w.w_001028D0 = w_001028D0;
    w.w_00102738 = w_00102738;
    w.w_00102760 = w_00102760;
    w.w_001031E0 = w_001031E0;
    w.w_0019AA80 = w_0019AA80;
    w.w_0019A570 = w_0019A570;
    w.w_0019B6C0 = w_0019B6C0;
    w.w_001EFD90 = w_001EFD90;
    w.w_001CD520 = w_001CD520;
    w.w_001E2BA0 = w_001E2BA0;
    /* 001B0FD0, 001AFC10 and the cable / strip workers belong to the other
     * lifecycles and the effect nodes: unbound here (fail-stop). */
    const int r = em_gun_rest_tick(l.gun, &mem, &w, fault);
    if (r < 0 || l.fault) {
        if (fault->code == EM_GUN_FAULT_NONE) {
            fault->address = l.fault;
            fault->code = EM_GUN_FAULT_WORKER_FAILED;
        }
        return -1;
    }
    for (unsigned k = 0; k < l.nspawned; ++k) {
        EmActor *a = l.spawned[k];
        if (!a->allocated || a->callback != EM_GUN_REST_SHOT_HANDLER || em_aim_fire_runtime_bind_flash(a) < 0) {
            fault->address = 0x00827400u;
            fault->code = EM_GUN_FAULT_WORKER_FAILED;
            fprintf(stderr, "em_area11 gun live: the shot node was not bound to 001F5040\n");
            return -1;
        }
    }
    return 1;
}
