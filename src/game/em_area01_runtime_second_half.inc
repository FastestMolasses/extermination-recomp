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
 * Any other overlay address of these areas reaches the host's workers
 * (fail-stop) as before. */
static int dispatch_second_half(EmArea01Runtime *r, EmArea01Call *c, int *out)
{
    const uint32_t fn = c->function;
    const uint32_t a0 = (uint32_t)c->a[0];
    int x;
    uint32_t where;
    int32_t code;
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
