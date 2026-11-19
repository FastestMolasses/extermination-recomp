/* em_sight_live.c - see em_sight_live.h. */
#include "game/em_sight_live.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/em_actor_collision.h"
#include "game/em_aim_fire_sdk_memory.h"
#include "game/em_aim_fire_tables.h"
#include "game/em_camera_live.h"
#include "game/em_collision_world.h"
#include "game/em_player.h"
#include "game/em_player_stage_workers.h"
#include "game/em_render_context_live.h"
#include "game/em_render_verify_rest.h"
#include "game/em_scene_bindings.h"
#include "game/em_sight.h"
#include "game/em_sight_lock.h"

#define STACK_BASE  UINT32_C(0x7F000000)   /* the window below the aim host's sp */
#define STACK_SIZE  0x1000u
#define CLASS2_BASE UINT32_C(0x0028AF30)   /* the class-2 list's static base */
#define D_275B90    UINT32_C(0x00275B90)
#define D_275B98    UINT32_C(0x00275B98)
#define RADAR_BASE  UINT32_C(0x0028AAB0)   /* the radar list's static base (001AAD00 resets D_00275B54 to it) */
#define D_275B54    UINT32_C(0x00275B54)
#define D_275B58    UINT32_C(0x00275B58)

/* The ELF .data blocks the drawers read (tools/export_sight_tables.py). */
static const struct {
    uint32_t address, size;
} k_blocks[] = {{0x00253240u, 0x190u}, {0x0026E6F0u, 0x120u}, {0x0026A990u, 0x150u}};
#define BLOCKS (sizeof k_blocks / sizeof k_blocks[0])

static struct {
    EmSightLiveServices services;
    int tables_tried, tables_ok;
    uint8_t d253240[0x190], d26E6F0[0x120], d26A990[0x150];
    uint8_t stack[STACK_SIZE];
    uint8_t class2_cursor[4], class2_count[2], class2_words[4 * EM_ACTOR_LIST_MAX];
    uint8_t radar_cursor[4], radar_count[2], radar_words[4 * EM_ACTOR_RADAR_MAX];
    /* 0x700036A0..0x7000371F: the radar's two matrices (0022E7F0 / 0022E630
     * write every word they read there first: 001029C0, the stores, 001031E0) */
    uint8_t s36a0[0x80];
    uint8_t number[16]; /* 001C5FB0's D_008111D0, this module's copy (as em_status_pages_live's) */
    /* read-only copies of class-2 entry fields, taken at the read (owner_view) */
    uint8_t entry[8][64];
    unsigned entry_next;
    void *(*map)(void *, uint32_t, size_t, int);    /* the aim camera's, for the call */
    int (*call)(void *, EmAimFireTargetCall *);
    int reported;
} L;

static uint8_t *table(unsigned i) { return i == 0 ? L.d253240 : i == 1 ? L.d26E6F0 : L.d26A990; }

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void wr32(uint8_t *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i));
}

/* assets/sight_tables.emst: 'EMST', u32 version 1, u32 block count, u32 0;
 * count x (u32 address, u32 size, u32 file offset, u32 0); the bytes. */
static int tables(void)
{
    if (L.tables_tried) return L.tables_ok;
    L.tables_tried = 1;
    FILE *f = fopen(EM_SIGHT_TABLES_PATH, "rb");
    if (!f) {
        fprintf(stderr, "em_sight_live: %s missing (run tools/export_sight_tables.py)\n", EM_SIGHT_TABLES_PATH);
        return 0;
    }
    uint8_t head[16 + 16 * BLOCKS];
    int ok = fread(head, 1, sizeof head, f) == sizeof head && !memcmp(head, "EMST", 4) && rd32(head + 4) == 1 &&
             rd32(head + 8) == BLOCKS;
    for (unsigned i = 0; ok && i < BLOCKS; ++i) {
        const uint8_t *e = head + 16 + 16 * i;
        ok = rd32(e) == k_blocks[i].address && rd32(e + 4) == k_blocks[i].size &&
             fseek(f, (long)rd32(e + 8), SEEK_SET) == 0 &&
             fread(table(i), 1, k_blocks[i].size, f) == k_blocks[i].size;
    }
    fclose(f);
    if (!ok) fprintf(stderr, "em_sight_live: %s malformed\n", EM_SIGHT_TABLES_PATH);
    L.tables_ok = ok;
    return ok;
}

static int within(uint32_t a, size_t n, uint32_t base, uint32_t size)
{
    return n && a >= base && n <= size && a - base <= size - n;
}

/* D_00275B90 / D_00275B98 and the entry words of the live class-2 list. */
static void *class2(uint32_t a, size_t n)
{
    const EmActorClassLists *ls = em_collision_world_lists();
    if (!ls) return NULL;
    const EmActorClassList *l = &ls->list[EM_ACTOR_LIST_CLASS2];
    if (l->live < 0 || l->live > EM_ACTOR_LIST_MAX) return NULL;
    const uint32_t live = (uint32_t)l->live;
    if (within(a, n, D_275B90, 4)) {
        wr32(L.class2_cursor, CLASS2_BASE - 4u * live);
        return L.class2_cursor + (a - D_275B90);
    }
    if (within(a, n, D_275B98, 2)) {
        L.class2_count[0] = (uint8_t)live;
        L.class2_count[1] = (uint8_t)(live >> 8);
        return L.class2_count + (a - D_275B98);
    }
    const uint32_t low = CLASS2_BASE - 4u * live;
    if (!within(a, n, low, 4u * live)) return NULL;
    const EmCollisionWorldOwners *o = em_collision_world_owners();
    if (!o || !o->address_of) return NULL;
    /* slot i lives at CLASS2_BASE - 4 (i + 1) */
    for (uint32_t at = (a - low) & ~3u; at < a - low + n; at += 4) {
        const uint32_t i = live - 1u - at / 4u;
        const uint32_t word = l->slot[i] ? o->address_of(o->context, l->slot[i]) : 0;
        if (!word) return NULL;
        wr32(L.class2_words + 4u * (EM_ACTOR_LIST_MAX - 1u - i), word);
    }
    return L.class2_words + 4u * EM_ACTOR_LIST_MAX - (CLASS2_BASE - a);
}

/* D_00275B54 / D_00275B58 and the radar list's words (the collision
 * world's radar pushes, 001B1CE0): slot i at RADAR_BASE - 4 (i + 1), the
 * cursor at the last push, as the original leaves them before 001AAD00. */
static void *radar(uint32_t a, size_t n)
{
    const EmActorClassLists *ls = em_collision_world_lists();
    if (!ls || ls->count_b58 < 0 || ls->count_b58 > EM_ACTOR_RADAR_MAX) return NULL;
    const uint32_t live = (uint32_t)ls->count_b58;
    if (within(a, n, D_275B54, 4)) {
        wr32(L.radar_cursor, RADAR_BASE - 4u * live);
        return L.radar_cursor + (a - D_275B54);
    }
    if (within(a, n, D_275B58, 2)) {
        L.radar_count[0] = (uint8_t)live;
        L.radar_count[1] = (uint8_t)(live >> 8);
        return L.radar_count + (a - D_275B58);
    }
    const uint32_t low = RADAR_BASE - 4u * live;
    if (!within(a, n, low, 4u * live)) return NULL;
    const EmCollisionWorldOwners *o = em_collision_world_owners();
    if (!o || !o->address_of) return NULL;
    for (uint32_t at = (a - low) & ~3u; at < a - low + n; at += 4) {
        const uint32_t i = live - 1u - at / 4u;
        const uint32_t word = ls->radar[i] ? o->address_of(o->context, ls->radar[i]) : 0;
        if (!word) return NULL;
        wr32(L.radar_words + 4u * (EM_ACTOR_RADAR_MAX - 1u - i), word);
    }
    return L.radar_words + 4u * EM_ACTOR_RADAR_MAX - (RADAR_BASE - a);
}

/* 0x70003600..0x7000363F: one owner when the camera's three shared
 * quadwords are contiguous (then 0x70003620 is the one between them). */
static uint8_t *scratch3600(uint32_t a, size_t n)
{
    uint8_t *p0 = em_camera_live_scratch_bytes(0x70003600u, 16), *p1 = em_camera_live_scratch_bytes(0x70003610u, 16),
            *p3 = em_camera_live_scratch_bytes(0x70003630u, 16);
    if (p0 && p1 == p0 + 16 && p3 == p0 + 48) return p0 + (a - 0x70003600u);
    return n <= 16 ? em_camera_live_scratch_bytes(a, (uint32_t)n) : NULL;
}

/* A class-2 entry outside AREA11 (an area's own pool record, e.g. AREA22's
 * creatures): 00199770 reads its +0 and +0x34, 00183B80 its +2, +3, +0xD and
 * +0x9F, read only. The bytes are the record as the area's actor view
 * composes it from its owners (em_scene_bindings_actor_snapshot), copied at
 * the read into one of a few rotating buffers. */
static void *owner_view(uint32_t a, size_t n)
{
    if (n > sizeof L.entry[0] || a < EM_ACTOR_POOL_BASE ||
        a - EM_ACTOR_POOL_BASE >= EM_ACTOR_POOL_CAPACITY * EM_ACTOR_RECORD_SIZE)
        return NULL;
    uint8_t *b = L.entry[L.entry_next];
    if (em_scene_bindings_actor_snapshot(a, (uint32_t)n, b) != 0) return NULL;
    L.entry_next = (L.entry_next + 1u) % (sizeof L.entry / sizeof L.entry[0]);
    return b;
}

static void *sight_map(void *ctx, uint32_t a, size_t n, int write)
{
    if (!n || n > STACK_SIZE) return NULL;
    if (within(a, n, STACK_BASE, STACK_SIZE)) return L.stack + (a - STACK_BASE);
    for (unsigned i = 0; i < BLOCKS; ++i)
        if (within(a, n, k_blocks[i].address, k_blocks[i].size))
            return write || !tables() ? NULL : table(i) + (a - k_blocks[i].address);
    void *p;
    if (!write && (p = class2(a, n))) return p;
    if (!write && (p = radar(a, n))) return p;
    if (within(a, n, 0x700036A0u, sizeof L.s36a0)) return L.s36a0 + (a - 0x700036A0u);
    if (within(a, n, 0x70003600u, 0x40)) return scratch3600(a, n);
    if (within(a, n, 0x008111D0u, sizeof L.number)) return L.number + (a - 0x008111D0u);
    /* 001DD2F0 / 001DD600's templates D_002533D0..D_0025348F (assets/aim_fire_tables.emaf) */
    if (within(a, n, 0x002533D0u, 0xC0u)) {
        const uint8_t *t;
        if (write) return NULL;
        if (!(t = em_aim_fire_tables_bytes(a, (uint32_t)n)) && em_aim_fire_tables_load() == 0)
            t = em_aim_fire_tables_bytes(a, (uint32_t)n);
        return (void *)(uintptr_t)t;
    }
    if (L.map && (p = L.map(ctx, a, n, write))) return p;
    /* the player's node records (its +0x110 words; the radar reads node 7's
     * +0x90 row), read only */
    if (!write && (p = (void *)(uintptr_t)player_pose_record_bytes(a, (uint32_t)n))) return p;
    if ((p = write ? em_rcl_bytes_mut(a, (uint32_t)n) : (void *)(uintptr_t)em_rcl_bytes(a, (uint32_t)n))) return p;
    /* a class-2 entry's fields: its publishing owner's record */
    const EmCollisionWorldOwners *o = em_collision_world_owners();
    if (o && o->record_bytes && (p = o->record_bytes(o->context, a, (uint32_t)n))) return p;
    return write ? NULL : owner_view(a, n);
}

static uint8_t *sight_map32(void *ctx, uint32_t a, uint32_t n, int write) { return sight_map(ctx, a, n, write); }

static int read_words(uint32_t a, uint32_t *w, unsigned count)
{
    const uint8_t *p = sight_map(NULL, a, 4u * count, 0);
    if (!p) return -1;
    for (unsigned i = 0; i < count; ++i) w[i] = rd32(p + 4 * i);
    return 0;
}

static int refuse(uint32_t fn, const char *why)
{
    if (!L.reported) {
        L.reported = 1;
        fprintf(stderr, "em_sight_live: callee %08X refused: %s\n", fn, why);
    }
    return -1;
}

static int leaf(void *ctx, EmAimFireTargetCall *c);

/* The reticle's 001CBA50 and 001281C0, through the drawers' leaf. */
static int lock_leaf(void *ctx, EmAimFireReticleCall *k)
{
    EmAimFireTargetCall c;
    memset(&c, 0, sizeof c);
    c.function = k->function;
    c.sp = k->sp;
    c.na = k->na;
    c.nf = k->nf;
    memcpy(c.a, k->a, sizeof k->a);
    c.f[0] = k->f12;
    const int status = leaf(ctx, &c);
    k->v0 = c.v0;
    return status;
}

/* The drawers' leaf callees. */
static int leaf(void *ctx, EmAimFireTargetCall *c)
{
    const int32_t a0 = (int32_t)(uint32_t)c->a[0], a1 = (int32_t)(uint32_t)c->a[1],
                  a2 = (int32_t)(uint32_t)c->a[2], a3 = (int32_t)(uint32_t)c->a[3],
                  t0 = (int32_t)(uint32_t)c->a[4];
    switch (c->function) {
    case 0x001D1F80u:
        return em_rcl_001D1F80(a0, a1, a2) < 0 ? refuse(c->function, "001D1F80 fault") : 0;
    case 0x001D64A0u: {
        uint32_t g0[3], g1[3], col[4];
        if (read_words((uint32_t)a1, g0, 3) < 0 || read_words((uint32_t)a2, g1, 3) < 0 ||
            read_words((uint32_t)a3, col, 4) < 0)
            return refuse(c->function, "an argument outside the views");
        return em_rcl_001D64A0(a0, g0, g1, col, t0) < 0 ? refuse(c->function, "001D64A0 fault") : 0;
    }
    case 0x00207F80u:
        if (!L.services.rect) return refuse(c->function, "no 00207F80 service bound");
        return L.services.rect(a0, a1, a2, a3, t0, (uint32_t)c->a[5]) == 1 ? 0 : refuse(c->function, "refused");
    case 0x001CBA50u: {
        uint8_t text[64], style[8];
        uint32_t n = 0;
        const uint8_t *s;
        do {
            if (!(s = sight_map(NULL, (uint32_t)c->a[5] + n, 1, 0))) return refuse(c->function, "the text unmapped");
            text[n] = *s;
        } while (text[n++] && n < sizeof text);
        if (text[n - 1]) return refuse(c->function, "the text unterminated");
        if (!(s = sight_map(NULL, (uint32_t)c->a[6], 8, 0))) return refuse(c->function, "the style unmapped");
        memcpy(style, s, 8);
        if (!L.services.text) return refuse(c->function, "no 001CBA50 service bound");
        return L.services.text(a0, a1, a2, a3, t0, text, n, style) == 1 ? 0 : refuse(c->function, "refused");
    }
    case 0x001281C0u:
        c->v0 = (uint64_t)(int64_t)em_player_float_to_int(c->f[0]);
        return 0;
    case 0x00102948u: case 0x001028B8u: case 0x001028D0u: case 0x001026A0u:
    case 0x001029C0u: case 0x00102760u: case 0x00102BB0u: case 0x001026D0u: case 0x001031E0u:
        return em_aim_fire_sdk_memory_call(NULL, sight_map, c) == 0 ? 0 : refuse(c->function, "an SDK operand");
    /* the radar's 2D packets (0022E7F0 / 0022E630) */
    case 0x00207D00u:
        if (!L.services.blend) return refuse(c->function, "no 00207D00 service bound");
        return L.services.blend(a0, a1) == 1 ? 0 : refuse(c->function, "refused");
    case 0x00207E40u:
        if (!L.services.sprite) return refuse(c->function, "no 00207E40 service bound");
        return L.services.sprite(a0, a1, a2, a3, t0, (uint32_t)c->a[5], c->a[6]) == 1 ? 0
                                                                                        : refuse(c->function, "refused");
    case 0x00208040u: {
        uint32_t v[3][3];
        for (unsigned k = 0; k < 3; ++k)
            if (read_words((uint32_t)c->a[1 + k], v[k], 3) < 0) return refuse(c->function, "a corner unmapped");
        const uint32_t pa[2] = {v[0][0], v[0][2]}, pb[2] = {v[1][0], v[1][2]}, pc[2] = {v[2][0], v[2][2]};
        if (!L.services.fan) return refuse(c->function, "no 00208040 service bound");
        return L.services.fan(a0, pa, pb, pc, (uint32_t)c->a[4]) == 1 ? 0 : refuse(c->function, "refused");
    }
    case 0x00102850u: {
        uint32_t v[4], out[4];
        EmRvrFault rf;
        memset(&rf, 0, sizeof rf);
        if (read_words((uint32_t)a1 & ~15u, v, 4) < 0) return refuse(c->function, "the source unmapped");
        if (em_rvr_00102850(out, v, c->f[0], &rf) < 0) return refuse(c->function, "00102850 fault");
        uint8_t *d = sight_map(NULL, (uint32_t)a0 & ~15u, 16, 1);
        if (!d) return refuse(c->function, "the destination unmapped");
        for (unsigned i = 0; i < 4; ++i) wr32(d + 4 * i, out[i]);
        return 0;
    }
    case 0x00183B80u: {
        EmAimFireTarget t;
        memset(&t, 0, sizeof t);
        t.map = sight_map32;
        t.sp = c->sp;
        uint32_t result = 0;
        if (em_aim_fire_target_00183B80(&t, (uint32_t)a0, &result) < 0) return refuse(c->function, "the entry unmapped");
        c->v0 = result;
        return 0;
    }
    case 0x00183C40u:
        if (!L.services.frame) return refuse(c->function, "no aim/fire composition bound");
        return L.services.frame(c) < 0 ? refuse(c->function, "the composition faulted") : 0;
    case 0x001DD170u: {
        /* 00199770's lock mark (em_sight_lock: vf23 = the fog quadword) */
        EmSightLock k;
        memset(&k, 0, sizeof k);
        k.context = ctx;
        k.map = sight_map32;
        k.call = lock_leaf;
        if (em_sight_lock_001DD170(&k, c->sp, (uint32_t)c->a[0], (uint32_t)c->a[1], (uint32_t)c->a[2],
                                   (uint32_t)c->a[3], (uint32_t)c->a[4]) < 0) {
            if (!L.reported) {
                L.reported = 1;
                fprintf(stderr, "em_sight_live: 001DD170 fault in %08X at %08X\n", k.fault_function, k.fault_address);
            }
            return -1;
        }
        return 0;
    }
    default:
        /* sin 0011E2A8, cos 0011DE90, sqrt 0011E748, fabs 0011DF78, atan2 0011E620, the wrap 001B1470 */
        if (c->function != 0x0011E2A8u && c->function != 0x0011DE90u && c->function != 0x0011E748u &&
            c->function != 0x0011DF78u && c->function != 0x0011E620u && c->function != 0x001B1470u)
            return refuse(c->function, "not a sight leaf");
        return L.call && L.call(ctx, c) == 0 ? 0 : refuse(c->function, "the aim camera's math");
    }
}

/* EM_SIGHT_DUMP=<file> (developer / test, tools/test_sight_live.py): one
 * JSON line per drawer call: the function, its f12 bits, the slot-1 cursor
 * (*D_00275670 + 0x14) before the call and the bytes the call put between
 * that cursor and the cursor after it. */
static int slot1_cursor(uint32_t *out)
{
    const uint8_t *ctx = em_rcl_bytes(0x00275670u, 4);
    const uint8_t *cur = ctx ? em_rcl_bytes(rd32(ctx) + 0x14u, 4) : NULL;
    if (!cur) return -1;
    *out = rd32(cur);
    return 0;
}

static void dump_call(const char *path, const EmAimFireTargetCall *f, uint32_t before)
{
    uint32_t after = 0;
    if (slot1_cursor(&after) < 0 || after < before || after - before > 0x10000u) return;
    const uint8_t *b = after > before ? em_rcl_bytes(before, after - before) : NULL;
    FILE *o = fopen(path, "a");
    if (!o) return;
    fprintf(o, "{\"fn\": \"%08X\", \"f12\": \"%08X\", \"start\": \"%08X\", \"bytes\": \"",
            (unsigned)f->function, (unsigned)f->f[0], (unsigned)before);
    for (uint32_t i = 0; b && i < after - before; ++i) fprintf(o, "%02x", b[i]);
    fprintf(o, "\"}\n");
    fclose(o);
}

void em_sight_live_bind(const EmSightLiveServices *services)
{
    if (services) L.services = *services;
    else memset(&L.services, 0, sizeof L.services);
}

int em_sight_live_call(EmAimFireTargetCall *f, void *(*map)(void *, uint32_t, size_t, int),
                       int (*call)(void *, EmAimFireTargetCall *))
{
    if (!f) return -1;
    switch (f->function) {
    case 0x001DB830u: case 0x001DB9D0u: case 0x001DBCB0u: case 0x001DBD50u: case 0x001DBE20u:
    case 0x001DBF00u: case 0x001DC020u: case 0x001DC610u: case 0x001DC890u: case 0x001DC960u:
    case 0x00199770u: case 0x0022E7F0u:
        break;
    default:
        return 1;
    }
    if (!within(f->sp - 0x300u, 0x300, STACK_BASE, STACK_SIZE)) return refuse(f->function, "sp outside the window");
    L.map = map;
    L.call = call;
    EmSight h;
    memset(&h, 0, sizeof h);
    h.map = sight_map32;
    h.call = leaf;
    uint32_t before = 0;
    const char *dump = getenv("EM_SIGHT_DUMP");
    if (dump && dump[0]) slot1_cursor(&before);
    const int rc = em_sight_call(&h, f);
    if (rc == 0 && dump && dump[0]) dump_call(dump, f, before);
    if (rc < 0 && !L.reported) {
        L.reported = 1;
        fprintf(stderr, "em_sight_live: %08X fault %d at %08X\n", h.fault_function, (int)h.fault, h.fault_address);
    }
    return rc < 0 ? -1 : 0;
}
