/* ctypes bridge for tools/test_status_glyph_nodes_reference.py: the status
 * models (em_status_models) spawn the equipment letters for a D_00810CA4..A7
 * selection (0020E250), walk the pool twice (001B0000: each letter's
 * 0020E460 state 0, then state 1 with 001C6380 and the 001CB580 draw) and
 * hand back one letter's record and its node world matrices (+0x90), for
 * the comparison with the original instructions. */
#include <stdint.h>
#include <string.h>

#include "em_gfx.h"
#include "game/em_actor_light_001D89D0.h"
#include "game/em_status_models.h"

/* em_gfx stubs: the walk only queues draws; nothing is rendered here. */
struct EmGfxMesh { int id; };
static struct EmGfxMesh mesh;
EmGfxMesh *em_gfx_mesh_create(EmGfx *g, const float *v, uint32_t vc, const uint32_t *i, uint32_t ic,
                              const EmGfxTexDesc *t, uint32_t tc, const uint8_t *tx, uint32_t f)
{
    (void)g; (void)v; (void)vc; (void)i; (void)ic; (void)t; (void)tc; (void)tx; (void)f;
    return &mesh;
}
void em_gfx_mesh_destroy(EmGfx *g, EmGfxMesh *m) { (void)g; (void)m; }
void em_gfx_draw_skinned(EmGfx *g, EmGfxMesh *m, const float *vp, const float *p, uint32_t n)
{
    (void)g; (void)m; (void)vp; (void)p; (void)n;
}
void em_gfx_char_rig(EmGfx *g, const EmGfxCharRig *rig) { (void)g; (void)rig; }
void em_gfx_fog_off(EmGfx *g) { (void)g; }

/* 001D8C20 + the bound 001D89D0, as tests/status_models_test.c binds it. */
static int32_t mode_word;
static int light_rgb(void *ctx, const EmOwnerServicesOwner *o, uint32_t rgb[4])
{
    (void)o;
    memcpy(rgb, ctx, 16);
    return 0;
}
static int light(void *ctx, int32_t mode, const EmOwnerServicesOwner *owner, const uint32_t rgb[4], float a[16],
                 float b[16])
{
    (void)ctx;
    mode_word = mode;
    if (!owner)
        return 0;
    EmActorLight l;
    memset(&l, 0, sizeof l);
    l.world.ctx_246C = &mode_word;
    uint32_t words[4];
    memcpy(words, rgb, sizeof words);
    EmActorLightBinding binding = {&l, words, light_rgb};
    return em_actor_light_w_001D89D0(&binding, owner, a, b);
}

/* Spawn the letters for `ca`, walk twice, and copy the record whose +0x0D is
 * `glyph` (0x2F0 bytes) and its nodes' +0x90 (16 floats each, up to `max`).
 * Returns the node count, or -1 (a fault or no such record). */
int sgn_run(const char *assets, const uint8_t ca[4], const uint8_t view[64], uint8_t glyph, uint8_t *record,
            float *worlds, int max)
{
    EmStatusModels *m = em_status_models_load(assets);
    if (!m)
        return -1;
    em_status_models_set_light(m, light, NULL);
    if (em_status_models_configure(m) != 1) { /* 0020DFA0, then the oracle's D_00810610 */
        em_status_models_free(m, NULL);
        return -1;
    }
    memcpy(em_status_models_view_bytes(m), view, 64);
    EmStatusModelsInputs in = {.health = 100.0f};
    memcpy(in.ca, ca, 4);
    int r = -1;
    if (em_status_models_clear(m) == 1 && em_status_models_event(m, EM_STATUS_HUB_BUILD_MODELS, 0, &in) == 1 &&
        em_status_models_event(m, EM_STATUS_HUB_ACTORS_TICK, 0, &in) == 1 &&
        em_status_models_event(m, EM_STATUS_HUB_ACTORS_TICK, 0, &in) == 1) {
        const EmStatusScenePool *pool = em_status_models_pool(m);
        for (unsigned i = 0; i < EM_STATUS_SCENE_POOL_RECORDS; ++i) {
            const EmStatusSceneActor *a = &pool->record[i];
            if (!a->b00 || a->b0D != glyph)
                continue;
            memcpy(record, a, sizeof *a);
            r = a->b0C;
            for (int b = 0; b < a->b0C && b < max; ++b)
                if (!em_status_models_node_world(m, i, (unsigned)b, worlds + 16 * b))
                    r = -1;
            break;
        }
    }
    em_status_models_free(m, NULL);
    return r;
}
