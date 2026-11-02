/* 0015B130's slots 0x0D, 0x16 and 0x17 and their piece 00224600: em_level14_port's
 * translations (em_level14_port_grip.c, test_level14_port_reference) over the
 * record and this closure's workers. */

/* The private stack the frames of 0016BF80 and 0016BE40 sit on. */
#define GRIP_STACK_TOP (LEDGE_GRIP_STACK + sizeof LG.stack)

static int gr_00102918(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return ledge_sdk(c, 0x00102918u, a0, a1, a2); }
static int gr_001029C0(void *c, uint32_t a0) { return ledge_sdk(c, 0x001029C0u, a0, 0, 0); }
static int gr_00102C58(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { return ledge_sdk(c, 0x00102C58u, a0, a1, a2); }
static int gr_001031E0(void *c, uint32_t a0, uint32_t a1) { return ledge_sdk(c, 0x001031E0u, a0, a1, 0); }
static int gr_0011A070(void *c, int32_t n0) { (void)c; return w_stop_sound(NULL, n0); }
static int gr_00122BB8(void *c, int32_t *result)
{
    (void)c;
    uint32_t value = 0;
    if (w_random(NULL, &value) < 0) return -1;
    *result = (int32_t)value;
    return 0;
}
static int gr_0016BAE0(void *c, uint32_t a0, int32_t arg) { (void)a0; return em_player_closure_0016BAE0(&L.c0e, c, arg); }
static int gr_00178B90(void *c, uint32_t a0, int32_t arg) { (void)a0; return w_translate(NULL, c, arg); }
static int gr_0017FF80(void *c, uint32_t a0, float blend) { (void)a0; return w_clip_FF80(NULL, c, blend); }
static int gr_00182870(void *c, uint32_t a0, int32_t tier) { (void)a0; return w_land_sound(NULL, c, tier); }
static int gr_00182AB0(void *c, uint32_t a0) { (void)a0; return em_player_closure_00182AB0(&L.c0e, c); }
static int gr_001B12B0(void *c, float f0, float f1, float f2, float *result) { return lg_001B12B0(c, f0, f1, f2, result); }
/* 001FBD50: 0x135 is the walk's looped sound: its handle is the state's. */
static int gr_001FBD50(void *c, uint32_t a0, int32_t id, int32_t flags, float range, int32_t *result)
{
    (void)a0;
    if (id != 0x135) {
        *result = 0;
        return w_sound_full(NULL, c, id, flags, range);
    }
    int handle = -1;
    if (flags != 0 || w_sound_handle(NULL, c, id, &handle) < 0) return -1;
    *result = handle;
    return 0;
}
static int gr_0021C120(void *c, uint32_t a0) { (void)a0; return w_0021C120(NULL, c); }
static int gr_0021C190(void *c, uint32_t a0, int32_t *r) { (void)a0; return w_0021C190(NULL, c, r); }
static int gr_0021C200(void *c, uint32_t a0) { (void)a0; return w_0021C200(NULL, c); }
static int gr_0021C270(void *c, uint32_t a0) { (void)a0; return w_0021C270(NULL, c); }
static int gr_0021C350(void *c, uint32_t a0) { (void)a0; return w_0021C350(NULL, c); }
/* 0019A570(from, to, mask, id): the query's point word 0x700031B4 stays as
 * the state left it; 0x700031D0 names the grid node, whose bytes ledge_bytes
 * reads, or the cell record's stand-in (its +1A low byte is 0: a cell's class
 * never equals a node attribute here). */
static int gr_0019A570(void *c, uint32_t a0, uint32_t a1, int32_t mask, int32_t id, int32_t *result)
{
    float from[4], to[4];
    const uint8_t *f = ledge_bytes(c, a0, 16), *t = ledge_bytes(c, a1, 16);
    if (!f || !t) return -1;
    memcpy(from, f, sizeof from);
    memcpy(to, t, sizeof to);
    int r = 0;
    if (w_segment_r(from, to, (unsigned)mask, id, &r) < 0) return -1;
    const EmCollProbeState *s = em_collision_world_segment()->state;
    memcpy(LG.s31B0, s->point, sizeof s->point);
    memset(LG.rec, 0, sizeof LG.rec);
    LG.word_31D0 = s->record == EM_COLL_PROBE_RECORD_GRID ? em_scene_bindings_grid_node_address((uint32_t)s->node) : LEDGE_HIT;
    *result = r;
    return 0;
}

static void grip_hooks(EmLevel14PortHooks *h, EmPlayerLiveActor *a)
{
    memset(h, 0, sizeof *h);
    h->ctx = a;
    h->bytes = ledge_bytes;
    h->w_001026A0 = lg_001026A0;
    h->w_001028B8 = lg_001028B8;
    h->w_00102918 = gr_00102918;
    h->w_001029C0 = gr_001029C0;
    h->w_00102C58 = gr_00102C58;
    h->w_001031E0 = gr_001031E0;
    h->w_0011A070 = gr_0011A070;
    h->w_0011DF78 = lg_abs_wrap;
    h->w_00122BB8 = gr_00122BB8;
    h->w_0016BAE0 = gr_0016BAE0;
    h->w_001749A0 = lg_001749A0;
    h->w_00174A50 = lg_00174A50;
    h->w_001751A0 = lg_001751A0;
    h->w_00175900 = lg_00175900;
    h->w_00178B90 = gr_00178B90;
    h->w_00179880 = lg_00179880;
    h->w_0017FF80 = gr_0017FF80;
    h->w_00182430 = lg_00182430;
    h->w_00182870 = gr_00182870;
    h->w_00182AB0 = gr_00182AB0;
    h->w_0019A570 = gr_0019A570;
    h->w_001B12B0 = gr_001B12B0;
    h->w_001B1470 = lg_wrap;
    h->w_001B61C0 = lg_001B61C0;
    h->w_001FBD50 = gr_001FBD50;
    h->w_0021C120 = gr_0021C120;
    h->w_0021C190 = gr_0021C190;
    h->w_0021C200 = gr_0021C200;
    h->w_0021C270 = gr_0021C270;
    h->w_0021C350 = gr_0021C350;
    h->w_0021D2E0 = lg_0021D2E0;
}
static int grip_report(const char *name, const EmLevel14PortFault *f)
{
    fprintf(stderr, "player closure: %s fault code %d at %08X\n", name, (int)f->code, (unsigned)f->address);
    return -1;
}
/* +5 0x0D, 00167C80: the drop onto a ledge from surface 0x33. */
static int drop_ledge_state(void *c, EmPlayerLiveActor *a)
{
    (void)c;
    EmLevel14PortHooks h;
    grip_hooks(&h, a);
    EmLevel14PortFault f = {0, 0};
    return em_level14_port_00167C80(&h, 0x008102B0u, &f) < 0 ? grip_report("00167C80", &f) : 0;
}
/* +5 0x16, 0016BC40: the rise to the overhead grip on surface 0x3C. */
static int grip_rise_state(void *c, EmPlayerLiveActor *a)
{
    (void)c;
    refresh_pad();
    EmLevel14PortHooks h;
    grip_hooks(&h, a);
    EmLevel14PortFault f = {0, 0};
    return em_level14_port_0016BC40(&h, 0x008102B0u, &f) < 0 ? grip_report("0016BC40", &f) : 0;
}
/* +5 0x17, 0016BF80: hanging from the surface. */
static int grip_hang_state(void *c, EmPlayerLiveActor *a)
{
    (void)c;
    refresh_pad();
    EmLevel14PortHooks h;
    grip_hooks(&h, a);
    EmLevel14PortFault f = {0, 0};
    return em_level14_port_0016BF80(&h, 0x008102B0u, (uint32_t)GRIP_STACK_TOP, &f) < 0 ? grip_report("0016BF80", &f) : 0;
}
