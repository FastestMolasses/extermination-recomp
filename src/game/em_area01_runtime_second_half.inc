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
 * Any other overlay address of these areas reaches the host's workers
 * (fail-stop) as before. */
static int a14_owner(uint32_t fn)
{
    return fn == 0x00823580u || fn == 0x00823760u || fn == 0x008238C0u || fn == 0x008238E0u || fn == 0x00823B30u ||
           fn == 0x00823E30u || fn == 0x00825C40u || fn == 0x00825FD0u;
}

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
