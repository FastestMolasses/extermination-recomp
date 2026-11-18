/* The second half's overlay owners whose translations exist (lane BOOT2,
 * docs/SECOND_HALF_AREAS.md section 6). Word-equal or near twins of owners
 * already translated, each checked on its own overlay's words:
 *   AREA14 (area byte 0x0E) 0x825AA0 = AREA03's 0x825290 word for word
 *     (test_area03_route_reference, AREA14's text over the same images);
 *   AREA14 0x823FC0 = AREA19's 0x827DD0 but its two callees and the
 *     missed-probe effect (em_area19_route, test_area19_route_reference);
 *     its probe 0x825530 and spawner 0x825A10 = AREA19's 0x829370 /
 *     0x829840 but the overlay data address they copy (em_level10_port /
 *     em_level13_port, their tests rerun on AREA14's overlay);
 *   AREA21 (0x15) 0x823A10 / 0x823AB0 = AREA01's 0x8239C0 but for the rate
 *     immediate, 40.0 / 8.0 (em_level17_boot, test_level17_boot_reference).
 * AREA14's own owners (no twin in any overlay) are em_area14_overlay's
 * (test_area14_overlay_reference): 0x823580, 0x823760, 0x8238E0, 0x823B30,
 * 0x823E30, 0x825FD0 and the op09 callbacks 0x8238C0 / 0x825C40; frames from
 * each function's first instruction.
 * AREA16's own owners (area byte 0x10) are em_area16_overlay's
 * (test_area16_overlay_reference, docs/AREA16_PORT.md): the behaviours its
 * placements and groups name, the two children they spawn, the op09
 * callbacks 0x825630 / 0x826C20 and the five effect behaviours its effect
 * ids 0..4 select (0x8237A0 .. 0x824110); frames from each function's
 * first instruction, widened by the overlay helper an owner calls.
 *
 * AREA20 (0x14, lane PORT-A20, docs/AREA20_PORT.md): 0x825F20 =
 * AREA03's 0x825290 word for word (test_area03_route_reference); the group
 * owner 0x824440 = AREA14's 0x823FC0 but its two callees and the miss
 * effect (kind 8: 0x80000067; em_area19_route, test_area19_route_reference),
 * its probe 0x8259B0 and spawner 0x825E90 = AREA14's 0x825530 / 0x825A10
 * but the overlay data address (em_level10_port / em_level13_port); its own
 * owners are em_area20_overlay's (test_area20_overlay_reference): [32]
 * 0x8239B0, the breakables 0x823D30 and the effect objects 0x823580,
 * 0x823600, 0x823660, 0x823780, 0x823840.
 * Any other overlay address of these areas reaches the host's workers
 * (fail-stop) as before. */
static int a14_owner(uint32_t fn)
{
    return fn == 0x00823580u || fn == 0x00823760u || fn == 0x008238C0u || fn == 0x008238E0u || fn == 0x00823B30u ||
           fn == 0x00823E30u || fn == 0x00825C40u || fn == 0x00825FD0u;
}

static int a20_owner(uint32_t fn)
{
    return fn == 0x00823580u || fn == 0x00823600u || fn == 0x00823660u || fn == 0x00823780u || fn == 0x00823840u ||
           fn == 0x008239B0u || fn == 0x00823D30u;
}

/* AREA17's own owners (area byte 0x11, em_area17_overlay,
 * test_area17_overlay_reference): every overlay function but the init
 * (em_area01_state's); frames from each function's first instruction. */
static const struct { uint32_t fn, frame; unsigned na; } a17_owners[] = {
    {0x00823580u, 0x10u, 1}, {0x008235E0u, 0x30u, 1}, {0x008236F0u, 0x30u, 1}, {0x00823800u, 0x10u, 1},
    {0x00823860u, 0xA0u, 1}, {0x008239D0u, 0x10u, 1}, {0x00823BA0u, 0x30u, 1}, {0x00823F70u, 0x30u, 3},
    {0x00824280u, 0x10u, 0}, {0x008242D0u, 0x20u, 1}, {0x008243E0u, 0x20u, 1}, {0x008245A0u, 0x70u, 1},
    {0x00824700u, 0x70u, 1}, {0x00824CF0u, 0x20u, 1}, {0x00824E40u, 0x20u, 1}, {0x008250E0u, 0x20u, 1},
    {0x008251B0u, 0x40u, 1}, {0x00825470u, 0x20u, 1}, {0x00825560u, 0x30u, 1}, {0x00825EA0u, 0x20u, 1},
    {0x008260C0u, 0x20u, 1}, {0x00826170u, 0x20u, 1}};

static int a17_owner(uint32_t fn)
{
    for (unsigned k = 0; k < sizeof a17_owners / sizeof a17_owners[0]; ++k)
        if (a17_owners[k].fn == fn) return (int)k;
    return -1;
}

static int a17_run(const EmArea17OverlayHooks *h, uint32_t fn, EmArea01Call *c, uint32_t sp, EmArea17OverlayFault *f)
{
    const uint32_t a0 = (uint32_t)c->a[0];
    int32_t result = 0;
    int x;
    switch (fn) {
    case 0x00823580u: return em_area17_overlay_00823580(h, a0, f);
    case 0x008235E0u: return em_area17_overlay_008235E0(h, a0, f);
    case 0x008236F0u: return em_area17_overlay_008236F0(h, a0, f);
    case 0x00823800u: return em_area17_overlay_00823800(h, a0, f);
    case 0x00823860u: return em_area17_overlay_00823860(h, a0, sp, f);
    case 0x008239D0u: return em_area17_overlay_008239D0(h, a0, f);
    case 0x00823BA0u: return em_area17_overlay_00823BA0(h, a0, f);
    case 0x00823F70u:
        x = em_area17_overlay_00823F70(h, a0, (uint32_t)c->a[1], (uint32_t)c->a[2], &result, f);
        if (x >= 0) c->v0 = (uint64_t)(int64_t)result;
        return x;
    case 0x00824280u:
        x = em_area17_overlay_00824280(h, &result, f);
        if (x >= 0) c->v0 = (uint64_t)(int64_t)result;
        return x;
    case 0x008242D0u: return em_area17_overlay_008242D0(h, a0, f);
    case 0x008243E0u: return em_area17_overlay_008243E0(h, a0, f);
    case 0x008245A0u: return em_area17_overlay_008245A0(h, a0, sp, f);
    case 0x00824700u: return em_area17_overlay_00824700(h, a0, sp, f);
    case 0x00824CF0u: return em_area17_overlay_00824CF0(h, a0, f);
    case 0x00824E40u: return em_area17_overlay_00824E40(h, a0, f);
    case 0x008250E0u: return em_area17_overlay_008250E0(h, a0, f);
    case 0x008251B0u: return em_area17_overlay_008251B0(h, a0, f);
    case 0x00825470u: return em_area17_overlay_00825470(h, a0, f);
    case 0x00825560u: return em_area17_overlay_00825560(h, a0, f);
    case 0x00825EA0u: return em_area17_overlay_00825EA0(h, a0, f);
    case 0x008260C0u: return em_area17_overlay_008260C0(h, a0, f);
    default: return em_area17_overlay_00826170(h, a0, f);
    }
}

static const struct { uint32_t fn, frame; unsigned na; } a16_owners[] = {
    {0x00823580u, 0xA0u, 1}, {0x00824440u, 0xB0u, 1}, {0x00824690u, 0x40u, 1}, {0x00824E10u, 0x60u, 1},
    {0x00825260u, 0x20u, 1}, {0x00825310u, 0xA0u, 1}, {0x00825630u, 0x10u, 3}, {0x00825800u, 0xC0u, 1},
    {0x008261A0u, 0x20u, 1}, {0x008263D0u, 0x90u, 1}, {0x00826C20u, 0x10u, 1}, {0x00826C50u, 0x30u, 1},
    {0x00827030u, 0x20u, 1}, {0x008273F0u, 0x40u, 1}, {0x00827B70u, 0x20u, 1}, {0x00827CE0u, 0x30u, 1},
    {0x00827FA0u, 0x30u, 1}, {0x00828200u, 0x60u, 1},
    /* the effect behaviours of effect ids 0..4 (em_area16_overlay_f) */
    {0x008237A0u, 0x50u, 1}, {0x008239B0u, 0xA0u, 1}, {0x00823D20u, 0x70u, 1}, {0x00823F70u, 0x40u, 1},
    {0x00824110u, 0xA0u, 1}};

static int a16_owner(uint32_t fn)
{
    for (unsigned k = 0; k < sizeof a16_owners / sizeof a16_owners[0]; ++k)
        if (a16_owners[k].fn == fn) return (int)k;
    return -1;
}

/* AREA21's own overlay owners (lane PORT-A21, docs/AREA21_PORT.md): the
 * placements', groups' and spawned nodes' behaviours and the scripts' op09
 * callbacks, each translated from the decomp C and checked against the
 * original instructions (make test-area21-overlay-reference). Frames from
 * each function's first instruction; `sp` the entry stack pointer for the
 * seven that keep frame locals their callees read (0x8297C0, 0x823580,
 * 0x8243B0, 0x824FE0, 0x824CF0, 0x825B00, 0x823B40). 0x824FE0, 0x824CF0,
 * 0x825B00 and 0x823B40 have no static reference but are reached: the boss
 * fight's 001EFD20 / 001EFD90 effect nodes (g77, lanes PORT-A21-BOSS,
 * PORT-A21-PHASES and PORT-A21-3B40). */
#include "game/em_area21_use.h"
#include "game/em_haze_ring_live.h"

/* AREA21's Use classifier 001AAC00 (boot code, reached only in area
 * 0x15, from the Use dispatcher 00160220: em_area01_live_classify) with
 * its tests 001AA8D0 / 001AAA90 (em_area21_use; make
 * test-area21-use-reference). Memory is this runtime's bytes; the four
 * callees re-enter this dispatcher by address. */
static uint8_t *a21u_bytes(void *ctx, uint32_t address, uint32_t size, int write)
{
    EmArea01Runtime *r = ctx;
    return r->host.bytes ? r->host.bytes(r->host.ctx, address, size, write) : NULL;
}
static int a21u_float(EmArea01Runtime *r, uint32_t fn, float x, float *result)
{
    EmArea01Call c = {.function = fn, .sp = r->sp, .na = 0, .nf = 1};
    c.f[0] = em_ee_bits(x);
    int rc = em_area01_runtime_call(r, &c);
    if (rc == 0 && result) *result = em_ee_float(c.f0);
    return rc;
}
static int a21u_fabs(void *ctx, float x, float *r) { return a21u_float(ctx, 0x0011DF78u, x, r); }
static int a21u_sqrt(void *ctx, float x, float *r) { return a21u_float(ctx, 0x0011E748u, x, r); }
static int a21u_wrap(void *ctx, float x, float *r) { return a21u_float(ctx, 0x001B1470u, x, r); }
static int a21u_xform(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2)
{
    EmArea01Runtime *r = ctx;
    EmArea01Call c = {.function = 0x001026A0u, .sp = r->sp, .na = 3, .a = {a0, a1, a2}};
    return em_area01_runtime_call(r, &c);
}

static int dispatch_area21(EmArea01Runtime *r, EmArea01Call *c, int *out, int *x, uint32_t *where, int32_t *code)
{
    if (c->function == 0x001AAC00u) {
        if (area_frame(r, c, 3, 0x60u, out) < 0) return -1;
        const EmArea21UseHooks h = {r, a21u_bytes, a21u_xform, a21u_fabs, a21u_sqrt, a21u_wrap};
        EmArea21UseFault f = {0, 0};
        int32_t result = 0;
        *x = em_area21_use_001AAC00(&h, (uint32_t)c->a[0], (uint32_t)c->a[1], (uint32_t)c->a[2],
                                    &result, &f);
        *where = f.address;
        *code = f.code;
        if (*x >= 0) c->v0 = (uint64_t)(int64_t)result;
        return 1;
    }
    if (c->function == 0x001E76A0u) {
        /* the effect node 001E76A0 (001EF9D0's kind table; g77 row 5922):
         * em_haze_ring over this runtime (frame 0x20; sp passed as it is) */
        if (c->na < 1 || c->nf) { *out = fail(r, c->function); return -1; }
        *x = em_haze_ring_live_001E76A0(r, c->sp, (uint32_t)c->a[0], where, code);
        return 1;
    }
    static const struct { uint32_t fn, frame; unsigned na; char module; } owners[] = {
        /* em_area21_overlay: the Gatling, the cannon, its projectile */
        {0x008297C0u, 0x50u, 1, 'g'}, {0x00828700u, 0x40u, 1, 'g'}, {0x00828F30u, 0x30u, 1, 'g'},
        /* em_area21_owners: counters, scripts, the ending request; op09 callbacks */
        {0x008261A0u, 0x20u, 1, 'w'}, {0x008262D0u, 0x10u, 1, 'w'}, {0x008266F0u, 0x20u, 1, 'w'},
        {0x00827040u, 0x30u, 1, 'w'}, {0x00827CD0u, 0x20u, 1, 'w'}, {0x008269A0u, 0x10u, 0, 'w'},
        {0x008269F0u, 0x40u, 3, 'w'}, {0x00827250u, 0x10u, 0, 'w'},
        /* em_area21_doors: doors, the rounds and their code pointer, the effects */
        {0x00826BB0u, 0x30u, 1, 'd'}, {0x00826E60u, 0x30u, 1, 'd'}, {0x0082A1D0u, 0x30u, 1, 'd'},
        {0x00825C70u, 0x40u, 1, 'd'}, {0x00825C60u, 0x00u, 1, 'd'}, {0x008272C0u, 0x20u, 1, 'd'},
        {0x008273C0u, 0x20u, 1, 'd'}, {0x00827640u, 0x20u, 1, 'd'}, {0x00827880u, 0x20u, 1, 'd'},
        {0x00827A30u, 0x30u, 1, 'd'}, {0x00827E70u, 0x30u, 1, 'd'}, {0x00828350u, 0x20u, 1, 'd'},
        /* em_area21_groups: the members of groups 0x82B500 / 0x82BA70 */
        {0x00823580u, 0x40u, 1, 'p'}, {0x008237B0u, 0x20u, 1, 'p'}, {0x00823860u, 0x10u, 1, 'p'},
        {0x008238E0u, 0x20u, 1, 'p'}, {0x00823AA0u, 0x00u, 1, 'p'},
        /* placement [52] (frame 0x120; `sp` for its packet at sp - 0x60) */
        {0x008243B0u, 0x120u, 1, 'p'},
        /* the node of [43] / [44]'s deferred spawn (frame 0xF0; its packet at sp - 0x60) */
        {0x00824FE0u, 0xF0u, 1, 'p'},
        /* an effect node 0014EC00's 001EFD20 starts in the boss fight (frame 0xA0; packet at sp - 0x60) */
        {0x00824CF0u, 0xA0u, 1, 'p'},
        /* the node 0014EC00's 001EFD90 starts (g77 row 5893; frame 0xA0, packet at sp - 0x60) */
        {0x00825B00u, 0xA0u, 1, 'p'},
        /* the node 0x824FE0's 001EFD20 starts (g77 row 12708; frame 0xE0, packet at sp - 0x70) */
        {0x00823B40u, 0xE0u, 1, 'p'}};
    const uint32_t fn = c->function;
    const unsigned n = sizeof owners / sizeof owners[0];
    unsigned k = 0;
    while (k < n && owners[k].fn != fn) ++k;
    if (k == n) return 0;
    const uint32_t sp = c->sp;
    const uint32_t a0 = (uint32_t)c->a[0];
    if (area_frame(r, c, owners[k].na, owners[k].frame, out) < 0) return -1;
    int32_t result = 0;
    int has_result = 0;
    if (owners[k].module == 'g') {
        EmArea21OverlayHooks h;
        b21g_hooks(r, &h);
        EmArea21OverlayFault f = {0, 0};
        *x = fn == 0x008297C0u ? em_area21_overlay_008297C0(&h, a0, sp, &f)
           : fn == 0x00828700u ? em_area21_overlay_00828700(&h, a0, &f) : em_area21_overlay_00828F30(&h, a0, &f);
        *where = f.address;
        *code = f.code;
    } else if (owners[k].module == 'w') {
        EmArea21OwnersHooks h;
        b21w_hooks(r, &h);
        EmArea21OwnersFault f = {0, 0};
        switch (fn) {
        case 0x008261A0u: *x = em_area21_owners_008261A0(&h, a0, &f); break;
        case 0x008262D0u: *x = em_area21_owners_008262D0(&h, a0, &f); break;
        case 0x008266F0u: *x = em_area21_owners_008266F0(&h, a0, &f); break;
        case 0x00827040u: *x = em_area21_owners_00827040(&h, a0, &f); break;
        case 0x00827CD0u: *x = em_area21_owners_00827CD0(&h, a0, &f); break;
        case 0x008269A0u: *x = em_area21_owners_008269A0(&h, &result, &f); has_result = 1; break;
        case 0x00827250u: *x = em_area21_owners_00827250(&h, &result, &f); has_result = 1; break;
        default:
            *x = em_area21_owners_008269F0(&h, a0, (uint32_t)c->a[1], (uint32_t)c->a[2], &result, &f);
            has_result = 1;
            break;
        }
        *where = f.address;
        *code = f.code;
    } else if (owners[k].module == 'd') {
        EmArea21DoorsHooks h;
        b21d_hooks(r, &h);
        EmArea21DoorsFault f = {0, 0};
        switch (fn) {
        case 0x00826BB0u: *x = em_area21_doors_00826BB0(&h, a0, &f); break;
        case 0x00826E60u: *x = em_area21_doors_00826E60(&h, a0, &f); break;
        case 0x0082A1D0u: *x = em_area21_doors_0082A1D0(&h, a0, &f); break;
        case 0x00825C70u: *x = em_area21_doors_00825C70(&h, a0, &f); break;
        case 0x00825C60u: *x = em_area21_doors_00825C60(&h, a0, &f); break;
        case 0x008272C0u: *x = em_area21_doors_008272C0(&h, a0, &f); break;
        case 0x008273C0u: *x = em_area21_doors_008273C0(&h, a0, &f); break;
        case 0x00827640u: *x = em_area21_doors_00827640(&h, a0, &f); break;
        case 0x00827880u: *x = em_area21_doors_00827880(&h, a0, &f); break;
        case 0x00827A30u: *x = em_area21_doors_00827A30(&h, a0, &f); break;
        case 0x00827E70u: *x = em_area21_doors_00827E70(&h, a0, &f); break;
        default: *x = em_area21_doors_00828350(&h, a0, &f); break;
        }
        *where = f.address;
        *code = f.code;
    } else {
        EmArea21GroupsHooks h;
        b21p_hooks(r, &h);
        EmArea21GroupsFault f = {0, 0};
        switch (fn) {
        case 0x00823580u: *x = em_area21_groups_00823580(&h, a0, sp, &f); break;
        case 0x008237B0u: *x = em_area21_groups_008237B0(&h, a0, &f); break;
        case 0x00823860u: *x = em_area21_groups_00823860(&h, a0, &f); break;
        case 0x008238E0u: *x = em_area21_groups_008238E0(&h, a0, &f); break;
        case 0x008243B0u: *x = em_area21_groups_008243B0(&h, a0, sp, &f); break;
        case 0x00824FE0u: *x = em_area21_groups_00824FE0(&h, a0, sp, &f); break;
        case 0x00824CF0u: *x = em_area21_groups_00824CF0(&h, a0, sp, &f); break;
        case 0x00825B00u: *x = em_area21_groups_00825B00(&h, a0, sp, &f); break;
        case 0x00823B40u: *x = em_area21_groups_00823B40(&h, a0, sp, &f); break;
        default: *x = em_area21_groups_00823AA0(&h, a0, &f); break;
        }
        *where = f.address;
        *code = f.code;
    }
    if (*x >= 0 && has_result) c->v0 = (uint64_t)(int64_t)result;
    return 1;
}

static int dispatch_second_half(EmArea01Runtime *r, EmArea01Call *c, int *out)
{
    const uint32_t fn = c->function;
    const uint32_t a0 = (uint32_t)c->a[0];
    int x;
    uint32_t where;
    int32_t code;
    int a21 = 0;   /* lane PORT-A21 */
    if (r->area == 0x0E && fn == 0x00825AA0u) {
        if (area_frame(r, c, 1, 0x20u, out) < 0) return 1;
        EmArea03RouteHooks h;
        r03_hooks(r, &h);
        EmArea03RouteFault f = {0, 0};
        x = em_area03_route_00825290(&h, a0, &f);
        where = f.address;
        code = f.code;
    } else if (r->area == 0x0E && fn == 0x00823FC0u) {
        if (area_frame(r, c, 1, 0x60u, out) < 0) return 1;
        EmArea19RouteHooks h;
        m19_hooks(r, &h);
        EmArea19RouteFault f = {0, 0};
        x = em_area19_route_00823FC0(&h, a0, &f);
        where = f.address;
        code = f.code;
    } else if (r->area == 0x0E && fn == 0x00825530u) {
        const uint32_t sp = c->sp;
        int32_t i = 0;
        if (area_frame(r, c, 2, 0x80u, out) < 0) return 1;
        EmLevel10PortHooks h;
        m10_hooks(r, &h);
        EmLevel10PortFault f = {0, 0};
        x = em_level10_port_00825530(&h, a0, (uint32_t)c->a[1], sp, &i, &f);
        if (x >= 0) c->v0 = (uint64_t)(int64_t)i;
        where = f.address;
        code = f.code;
    } else if (r->area == 0x0E && fn == 0x00825A10u) {
        const uint32_t sp = c->sp;
        if (area_frame(r, c, 1, 0x40u, out) < 0) return 1;
        EmLevel13PortHooks h;
        m13_hooks(r, &h);
        EmLevel13PortFault f = {0, 0};
        x = em_level13_port_00825A10(&h, a0, sp, &f);
        where = f.address;
        code = f.code;
    } else if (r->area == 0x0E && a14_owner(fn)) {
        static const struct { uint32_t fn, frame; unsigned na; } owners[] = {
            {0x00823580u, 0xA0u, 1}, {0x00823760u, 0x30u, 1}, {0x008238C0u, 0x10u, 0}, {0x008238E0u, 0x80u, 1},
            {0x00823B30u, 0x30u, 1}, {0x00823E30u, 0x30u, 1}, {0x00825C40u, 0x50u, 3}, {0x00825FD0u, 0x30u, 1}};
        unsigned k = 0;
        while (owners[k].fn != fn) ++k;
        const uint32_t sp = c->sp;
        if (area_frame(r, c, owners[k].na, owners[k].frame, out) < 0) return 1;
        EmArea14OverlayHooks h;
        a14o_hooks(r, &h);
        EmArea14OverlayFault f = {0, 0};
        int32_t result = 0;
        switch (fn) {
        case 0x00823580u: x = em_area14_overlay_00823580(&h, a0, sp, &f); break;
        case 0x00823760u: x = em_area14_overlay_00823760(&h, a0, &f); break;
        case 0x008238C0u: x = em_area14_overlay_008238C0(&h, &result, &f); break;
        case 0x008238E0u: x = em_area14_overlay_008238E0(&h, a0, sp, &f); break;
        case 0x00823B30u: x = em_area14_overlay_00823B30(&h, a0, &f); break;
        case 0x00823E30u: x = em_area14_overlay_00823E30(&h, a0, &f); break;
        case 0x00825C40u:
            x = em_area14_overlay_00825C40(&h, a0, (uint32_t)c->a[1], (uint32_t)c->a[2], &result, &f);
            break;
        default: x = em_area14_overlay_00825FD0(&h, a0, &f); break;
        }
        if (x >= 0 && (fn == 0x008238C0u || fn == 0x00825C40u)) c->v0 = (uint64_t)(int64_t)result;
        where = f.address;
        code = f.code;
    } else if (r->area == 0x14 && fn == 0x00825F20u) {
        if (area_frame(r, c, 1, 0x20u, out) < 0) return 1;
        EmArea03RouteHooks h;
        r03_hooks(r, &h);
        EmArea03RouteFault f = {0, 0};
        x = em_area03_route_00825290(&h, a0, &f);
        where = f.address;
        code = f.code;
    } else if (r->area == 0x14 && fn == 0x00824440u) {
        if (area_frame(r, c, 1, 0x60u, out) < 0) return 1;
        EmArea19RouteHooks h;
        m19_hooks(r, &h);
        EmArea19RouteFault f = {0, 0};
        x = em_area19_route_00824440(&h, a0, &f);
        where = f.address;
        code = f.code;
    } else if (r->area == 0x14 && fn == 0x008259B0u) {
        const uint32_t sp = c->sp;
        int32_t i = 0;
        if (area_frame(r, c, 2, 0x80u, out) < 0) return 1;
        EmLevel10PortHooks h;
        m10_hooks(r, &h);
        EmLevel10PortFault f = {0, 0};
        x = em_level10_port_008259B0(&h, a0, (uint32_t)c->a[1], sp, &i, &f);
        if (x >= 0) c->v0 = (uint64_t)(int64_t)i;
        where = f.address;
        code = f.code;
    } else if (r->area == 0x14 && fn == 0x00825E90u) {
        const uint32_t sp = c->sp;
        if (area_frame(r, c, 1, 0x40u, out) < 0) return 1;
        EmLevel13PortHooks h;
        m13_hooks(r, &h);
        EmLevel13PortFault f = {0, 0};
        x = em_level13_port_00825E90(&h, a0, sp, &f);
        where = f.address;
        code = f.code;
    } else if (r->area == 0x14 && a20_owner(fn)) {
        /* frames from each function's first instruction; 0x8239B0's
         * nested 0x823BE0 adds 0x70 to its 0x20 */
        static const struct { uint32_t fn, frame; } owners[] = {
            {0x00823580u, 0x20u}, {0x00823600u, 0x10u}, {0x00823660u, 0x30u}, {0x00823780u, 0x20u},
            {0x00823840u, 0x30u}, {0x008239B0u, 0x90u}, {0x00823D30u, 0x30u}};
        unsigned k = 0;
        while (owners[k].fn != fn) ++k;
        const uint32_t sp = c->sp;
        if (area_frame(r, c, 1, owners[k].frame, out) < 0) return 1;
        EmArea20OverlayHooks h;
        a20o_hooks(r, &h);
        EmArea20OverlayFault f = {0, 0};
        switch (fn) {
        case 0x00823580u: x = em_area20_overlay_00823580(&h, a0, &f); break;
        case 0x00823600u: x = em_area20_overlay_00823600(&h, a0, &f); break;
        case 0x00823660u: x = em_area20_overlay_00823660(&h, a0, &f); break;
        case 0x00823780u: x = em_area20_overlay_00823780(&h, a0, &f); break;
        case 0x00823840u: x = em_area20_overlay_00823840(&h, a0, &f); break;
        case 0x008239B0u: x = em_area20_overlay_008239B0(&h, a0, sp, &f); break;
        default: x = em_area20_overlay_00823D30(&h, a0, &f); break;
        }
        where = f.address;
        code = f.code;
    } else if (r->area == 0x11 && a17_owner(fn) >= 0) {
        const int k = a17_owner(fn);
        const uint32_t sp = c->sp;
        if (area_frame(r, c, a17_owners[k].na, a17_owners[k].frame, out) < 0) return 1;
        EmArea17OverlayHooks h;
        a17o_hooks(r, &h);
        EmArea17OverlayFault f = {0, 0};
        x = a17_run(&h, fn, c, sp, &f);
    } else if (r->area == 0x10 && a16_owner(fn) >= 0) {
        const int k = a16_owner(fn);
        const uint32_t sp = c->sp;
        if (area_frame(r, c, a16_owners[k].na, a16_owners[k].frame, out) < 0) return 1;
        EmArea16OverlayHooks h;
        a16o_hooks(r, &h);
        EmArea16OverlayFault f = {0, 0};
        int32_t result = 0;
        switch (fn) {
        case 0x00823580u: x = em_area16_overlay_00823580(&h, a0, sp, &f); break;
        case 0x00824440u: x = em_area16_overlay_00824440(&h, a0, sp, &f); break;
        case 0x00824690u: x = em_area16_overlay_00824690(&h, a0, sp, &f); break;
        case 0x00824E10u: x = em_area16_overlay_00824E10(&h, a0, sp, &f); break;
        case 0x00825260u: x = em_area16_overlay_00825260(&h, a0, sp, &f); break;
        case 0x00825310u: x = em_area16_overlay_00825310(&h, a0, sp, &f); break;
        case 0x00825630u:
            x = em_area16_overlay_00825630(&h, a0, (uint32_t)c->a[1], (uint32_t)c->a[2], &result, &f);
            break;
        case 0x00825800u: x = em_area16_overlay_00825800(&h, a0, sp, &f); break;
        case 0x008261A0u: x = em_area16_overlay_008261A0(&h, a0, sp, &f); break;
        case 0x008263D0u: x = em_area16_overlay_008263D0(&h, a0, sp, &f); break;
        case 0x00826C20u: x = em_area16_overlay_00826C20(&h, a0, 0, 0, &result, &f); break;
        case 0x00826C50u: x = em_area16_overlay_00826C50(&h, a0, sp, &f); break;
        case 0x00827030u: x = em_area16_overlay_00827030(&h, a0, sp, &f); break;
        case 0x008273F0u: x = em_area16_overlay_008273F0(&h, a0, sp, &f); break;
        case 0x00827B70u: x = em_area16_overlay_00827B70(&h, a0, sp, &f); break;
        case 0x00827CE0u: x = em_area16_overlay_00827CE0(&h, a0, sp, &f); break;
        case 0x00827FA0u: x = em_area16_overlay_00827FA0(&h, a0, sp, &f); break;
        case 0x008237A0u: x = em_area16_overlay_008237A0(&h, a0, sp, &f); break;
        case 0x008239B0u: x = em_area16_overlay_008239B0(&h, a0, sp, &f); break;
        case 0x00823D20u: x = em_area16_overlay_00823D20(&h, a0, sp, &f); break;
        case 0x00823F70u: x = em_area16_overlay_00823F70(&h, a0, sp, &f); break;
        case 0x00824110u: x = em_area16_overlay_00824110(&h, a0, sp, &f); break;
        default: x = em_area16_overlay_00828200(&h, a0, sp, &f); break;
        }
        if (x >= 0 && (fn == 0x00825630u || fn == 0x00826C20u)) c->v0 = (uint64_t)(int64_t)result;
        where = f.address;
        code = f.code;
    } else if (r->area == 0x15 && (a21 = dispatch_area21(r, c, out, &x, &where, &code)) != 0) {
        if (a21 < 0) return 1;   /* area_frame refused: *out holds its fault */
    } else if (r->area == 0x15 && (fn == 0x00823A10u || fn == 0x00823AB0u)) {
        if (area_frame(r, c, 1, 0x10u, out) < 0) return 1;
        EmLevel17BootHooks h;
        m17x_hooks(r, &h);
        EmLevel17BootFault f = {0, 0};
        x = fn == 0x00823A10u ? em_level17_boot_a21_00823A10(&h, a0, &f) : em_level17_boot_a21_00823AB0(&h, a0, &f);
        where = f.address;
        code = (int32_t)f.code;
    } else {
        return 0;
    }
    if (x < 0) {
        fprintf(stderr, "em_area01: area %02X %08X fault code %d at %08X\n", (unsigned)r->area, (unsigned)fn,
                (int)code, (unsigned)where);
        *out = fail(r, where ? where : fn);
        return 1;
    }
    *out = 0;
    return 1;
}
