/* em_background_live.c - see em_background_live.h. */
#include "game/em_background_live.h"
#include "game/em_norender.h"

#include <stdio.h>
#include <string.h>

#include "game/em_chain_page.h"
#include "game/em_chain_page_live.h"
#include "game/em_diag.h"
#include "game/em_frame.h"
#include "game/em_render_context_live.h"

#define PRIMS_MAX 2048u          /* 31 strips of 62 triangles: 1922 */
#define GRID_PRIMS_MAX 4096u     /* +0x1E8 (AREA21): 2786 or 2787 measured over g75 .. g77 */

static struct {
    EmChainPage page;
    EmGfxGsPrim prims[PRIMS_MAX];
    EmGfxGsEnv env[PRIMS_MAX];
    EmChainPage grid;
    EmGfxGsPrim grid_prims[GRID_PRIMS_MAX];
    EmGfxGsEnv grid_env[GRID_PRIMS_MAX];
    uint32_t fault;
    EmBackgroundLiveLog log;
} S;

static int fail(uint32_t address, const char *what)
{
    if (!S.fault) {
        S.fault = address;
        fprintf(stderr, "background: %s (%08X: %s, detail %08X)\n", what, (unsigned)address,
                em_chain_page_fault_name(S.page.fault), (unsigned)S.page.fault_detail);
    }
    return -1;
}

uint32_t em_background_live_fault(void) { return S.fault; }

static const uint8_t *reader(void *ctx, uint32_t address, uint32_t size)
{
    (void)ctx;
    return em_chain_page_live_read(address, size);
}

static uint32_t fnv(uint32_t h, uint32_t w)
{
    for (unsigned k = 0; k < 4u; ++k) h = (h ^ ((w >> (8u * k)) & 0xFFu)) * 16777619u;
    return h;
}

int em_background_live_draw(EmGfx *gfx)
{
    if (S.fault) return -1;
    if (em_norender()) return 0;   /* a pure read of the list: the grid's draw is the picture only */
    const uint8_t *head = em_rcl_bytes(EM_RCL_CONTEXT + 0x1D8u, 4);
    if (!head) return fail(0x001E0DF0u, "the render context has no +0x1D8 word");
    const uint32_t start = (uint32_t)head[0] | (uint32_t)head[1] << 8 | (uint32_t)head[2] << 16 |
                           (uint32_t)head[3] << 24;
    if (!start) return fail(0x001E0DF0u, "the gate CALLs the channel-3 list but +0x1D8 is 0");
    EmChainPage *p = &S.page;
    memset(p, 0, sizeof *p);
    p->read = reader;
    p->prims = S.prims;
    p->prim_env = S.env;
    p->prim_capacity = PRIMS_MAX;
    if (em_chain_page_run_call(p, start) < 0) return fail(p->fault_address, "the channel-3 list walk faulted");
    if (p->counts.mscal_grid != 1u) return fail(start, "the channel-3 list ran no grid program");
    for (uint32_t i = 0; i < p->prim_count; ++i)
        if (!(S.env[i].set & EM_GFX_GS_ENV_ZBUF) || !((S.env[i].zbuf >> 32) & 1u))
            return fail(start, "a grid triangle drawn with Z writes on (ZBUF_1 ZMSK 0)");
    if (em_gfx_background_prims_env(gfx, S.prims, S.env, p->prim_count) < 0)
        return fail(start, "the renderer refused the grid triangles");
    /* 001E0DF0's second CALL: +0x1E8, 001E1AD0's list (render flag 0x22:
     * AREA21's sea; AREA21_PORT.md "Grid"), right after +0x1D8's. It is GIF
     * DIRECT data only: 001E1760's strips into the 256 x 256 texture at FBP
     * 0x12C (Z masked), the 32 x 32 screen grid sampling it (TBP 0x2580,
     * blended, no Z: AFAIL RGB_ONLY) and 001E17E0's strips that write Z
     * alone (TEST AFAIL ZB_ONLY, ZBUF ZMSK 0): the sea's depth, which step
     * K's read-back meets (AREA21_PORT.md "Step K's Z read-back"). The walk
     * records every primitive with the environment the list wrote before
     * it, and the GS frame model draws them. Without the GS frame (the GPU
     * renderer) the list is not drawn: the GPU background path draws only
     * the asset grid of +0x1D8. */
    const uint8_t *grid_head = em_rcl_bytes(EM_RCL_CONTEXT + 0x1E8u, 4);
    if (!grid_head) return fail(0x001E0DF0u, "the render context has no +0x1E8 word");
    const uint32_t grid_start = (uint32_t)grid_head[0] | (uint32_t)grid_head[1] << 8 |
                                (uint32_t)grid_head[2] << 16 | (uint32_t)grid_head[3] << 24;
    S.log.grid_start = S.log.grid_prims = 0;
    if (grid_start && em_gfx_gs_world_enabled(gfx)) {
        EmChainPage *q = &S.grid;
        memset(q, 0, sizeof *q);
        q->read = reader;
        q->prims = S.grid_prims;
        q->prim_env = S.grid_env;
        q->prim_capacity = GRID_PRIMS_MAX;
        if (em_chain_page_run_call(q, grid_start) < 0) {
            S.page.fault = q->fault;   /* the fault's name and detail in the report */
            S.page.fault_detail = q->fault_detail;
            return fail(q->fault_address, "the +0x1E8 list walk (001E1AD0's) faulted");
        }
        if (em_gfx_background_prims_env(gfx, S.grid_prims, S.grid_env, q->prim_count) < 0)
            return fail(grid_start, "the renderer refused the +0x1E8 list's primitives");
        S.log.grid_start = grid_start;
        S.log.grid_prims = q->prim_count;
    }
    EmBackgroundLiveLog *l = &S.log;
    l->frame = em_frame_counter();
    l->serial = em_frame_serial();
    l->draws++;
    l->start = start;
    l->prims = p->prim_count;
    /* The digest is the tick log's only (em_diag.h): 0 without it. */
    const int digests = em_diag_digests();
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; digests && i < p->prim_count; ++i)
        for (uint32_t k = 0; k < 3u; ++k) {
            const EmGfxGsVertex *v = &S.prims[i].v[k];
            h = fnv(fnv(fnv(fnv(h, v->x), v->y), v->s), v->t);
        }
    l->digest = digests ? h : 0u;
    static const uint32_t at[11] = {0x000, 0x081, 0x102, 0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207};
    for (unsigned k = 0; k < 11u; ++k) memcpy(l->upload[k], p->dmem[at[k]].w, 16);
    return 0;
}

void em_background_live_log(EmBackgroundLiveLog *out)
{
    if (out) *out = S.log;
}
