/* Captured bytes are test input only. Exercise the production view adapter
 * and draw owner against original code without a GPU. */
#include "area01_model_bridge.c"
#include "game/em_area01_model_draw.h"
#define report draw_report
#define words draw_words
#include "game/em_owner_draw_live.c"
#undef words
#undef report
#include "game/em_load_veil_particles.h"
#include "game/em_render_context.h"
#include "game/em_packet_chain_original.h"
#include "game/em_sdk_math_original.h"
#include "game/em_player_stage_workers.h"
EmGameState g;
static EmArea01ModelDraw draw_view;
static EmRenderContext render;
static EmRenderContextView render_views[2];
static EmLoadVeilParticles veil;
uint32_t em_frame_counter(void){return 1;}
uint32_t em_rcl_fault(void){return 0;}
const uint8_t *em_rcl_bytes(uint32_t a,uint32_t n)
{
    if(a>=0x70000000u && a-0x70000000u<=0x4000u && n<=0x4000u-(a-0x70000000u))
        return spr+(a-0x70000000u);
    return resource(NULL,a,n);
}
uint8_t *em_rcl_bytes_mut(uint32_t a,uint32_t n)
{return (uint8_t *)(void *)em_rcl_bytes(a,n);}
int em_rcl_001D1F80(int32_t a0,int32_t a1,int32_t a2)
{return em_load_veil_particles_001D1F80(&veil,a0,a1,a2);}
int em_rcl_001D2910(int32_t a0,uint32_t *out)
{return em_render_context_001D2910(&render,a0,out);}
/* The draw methods (001CB130 kind 9 ...): 001CAAC0's chain over the same
 * memory, and the veil module's builders the render context would call. */
static EmPacketChain chain;
static EmPacketChainRegion chain_regions[2];
EmPacketChain *em_rcl_packet_chain(void){return &chain;}
int em_rcl_001D40E0(int32_t chan,int six,uint32_t model,uint32_t model_w04,const uint32_t rgb[3],
                    const uint8_t copy[16],const uint8_t xy[64],uint8_t colours[64])
{return em_load_veil_particles_001D40E0(&veil,chan,six,model,model_w04,rgb,copy,xy,colours);}
int em_rcl_001D3F60(int32_t chan,uint32_t model,uint32_t model_w04,const uint8_t copy[16])
{return em_load_veil_particles_001D3F60(&veil,chan,model,model_w04,copy);}
int em_rcl_001D1F20(int32_t chan){return em_load_veil_particles_001D1F20(&veil,chan);}
int em_rcl_001D1FF0(int32_t chan,int32_t a1){return em_load_veil_particles_001D1FF0(&veil,chan,a1);}
int em_rcl_001CB760(uint32_t table,int32_t id,uint32_t address)
{return em_packet_chain_001CB760(&chain,table,id,address);}
/* The method's D_0026E470 / D_0026E4C0 quadword and rows (ELF data; the
 * oracle checks the captured bytes equal the ELF's before serving them).
 * Production reads them from effect_tables.emet. */
const uint8_t *em_effects_live_window(uint32_t a,uint32_t n)
{
    static const uint32_t win[][2]={{0x0026E470u,0x50u},{0x0026E4C0u,0x50u}};
    for(unsigned i=0;i<sizeof win/sizeof win[0];++i)
        if(a>=win[i][0] && n<=win[i][1] && a-win[i][0]<=win[i][1]-n)return ram+a;
    return NULL;
}
static int veil_fabs(void *ctx,uint32_t x,uint32_t *r)
{(void)ctx;float f,v;memcpy(&f,&x,4);v=em_sdk_math_original_0011DF78(f);memcpy(r,&v,4);return 0;}
static int veil_int(void *ctx,uint32_t x,int32_t *r){(void)ctx;*r=em_player_float_to_int(x);return 0;}
#ifndef EM_A01_TEST_SHADOW
int em_shadow_live_actor_001DA6A0(uint32_t record,const uint8_t *bytes,uint32_t size,
                                const uint8_t *const nodes[],uint32_t count)
{(void)record;(void)bytes;(void)size;(void)nodes;(void)count;abort();}
#endif
int ad_seed(uint8_t *memory,uint8_t *scratch,uint32_t node)
{
    if(am_seed(memory,scratch,node,1)<0)return -1;
    em_owner_draw_live_reset();g.point_lights_loaded=1;
    memcpy(L.rig,ram+0x817BC0,sizeof L.rig);
    memcpy(L.spr.s3400,spr+0x3400,64);memcpy(L.spr.s3440,spr+0x3440,64);
    memcpy(L.spr.s3480,spr+0x3480,64);memcpy(L.spr.s3AC0,spr+0x3AC0,64);
    /* Explicit handoff: the captured canonical values were copied above;
     * pose and the renderer now write the same existing physical bytes. */
    pose.spad3400=(uint32_t *)(void *)em_owner_draw_live_memory(0x70003400u,64);
    pose.spad3440=(uint32_t *)(void *)em_owner_draw_live_memory(0x70003440u,64);
    if(!pose.spad3400 || !pose.spad3440)return -1;
    memset(&render,0,sizeof render);
    render_views[0]=(EmRenderContextView){0,0x2000000u,ram};
    render_views[1]=(EmRenderContextView){0x70000000u,0x4000u,spr};
    render.world.ctx=EM_RCL_CONTEXT;render.world.views=render_views;render.world.view_count=2;
    memset(&veil,0,sizeof veil);
    veil.world.cursor=(uint32_t *)(void *)(ram+EM_RCL_CONTEXT+0x10);veil.world.cursor_count=4;
    veil.world.ctx_9C=(uint32_t *)(void *)(ram+EM_RCL_CONTEXT+0x9C);
    veil.world.d00275674=(uint32_t *)(void *)(ram+0x275674);
    veil.world.packet=ram;veil.world.packet_address=0;veil.world.packet_size=0x2000000u;
    veil.world.d00275678=(uint32_t *)(void *)(ram+0x275678);
    veil.world.d0027568C=(uint32_t *)(void *)(ram+0x27568C);
    veil.world.ctx_0C=(uint32_t *)(void *)(ram+EM_RCL_CONTEXT+0x0C);
    veil.world.ctx_50=(uint32_t *)(void *)(ram+EM_RCL_CONTEXT+0x50);
    veil.world.d0026E880=ram+0x26E880;veil.world.d00241010=ram+0x241010;
    veil.workers.w_0011DF78=veil_fabs;veil.workers.w_001281C0=veil_int;
    chain_regions[0]=(EmPacketChainRegion){0,0x2000000u,ram};
    chain_regions[1]=(EmPacketChainRegion){0x70000000u,0x4000u,spr};
    uint32_t ctx_word,gs_word;memcpy(&ctx_word,ram+0x275670,4);memcpy(&gs_word,ram+0x275674,4);
    em_packet_chain_init(&chain,chain_regions,2,ctx_word,gs_word);
    em_area01_model_draw_bind(&draw_view,&model);
    draw_view.current=node;   /* D_00275B44, as em_area01_live sets it from the host */
    return 0;
}
int ad_call(uint32_t function,uint32_t node)
{return em_area01_model_draw_call(&draw_view,function,node);}
void ad_snapshot(void)
{
    am_snapshot();
    memcpy(ram+0x817BC0,L.rig,sizeof L.rig);
    memcpy(spr+0x3400,L.spr.s3400,64);memcpy(spr+0x3440,L.spr.s3440,64);
    memcpy(spr+0x3480,L.spr.s3480,64);memcpy(spr+0x3AC0,L.spr.s3AC0,64);
}
uint32_t ad_units(void){return em_owner_draw_live_count();}
uint32_t ad_fault(void){return model.fault_address;}
int ad_pose(uint32_t node)
{uint32_t out;return em_area01_model_call(&model,0x001C68C0u,node,0,0,0,0,0,&out);}
int ad_failure_boundary(uint32_t node,uint32_t fn,uint32_t hole,int active)
{
    if(active && em_area01_actor_view_begin(&views)<0)return -1;
    resource_hole=hole;
    int rc=em_area01_model_draw_call(&draw_view,fn,node);
    if(rc>=0 || views.fault || views.active!=active || model.fault_address!=(hole?hole:fn))return -1;
    return active ? em_area01_actor_view_commit(&views) : 0;
}
