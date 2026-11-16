/* em_norender.h - the headless "no render" developer switch.
 *
 * EM_NO_RENDER=1 skips the work whose only output is the picture: the VU1
 * object kernels and the packet walks the world flush runs to hand triangles
 * to the GS model, and the calls into the GS model that follow them. The
 * game's state is untouched: every flag a draw consumes, every counter or
 * list a draw resets and every record a later frame reads is still updated
 * exactly as the draw updates it. A skipped draw cannot latch a fail-stop
 * (draw faults are not detected under the switch).
 *
 * Honoured only in a headless run (em_headless) that reads no picture and no
 * draw log: not with EM_HEADLESS_RASTER=1, any EM_ variable naming CAPTURE,
 * PRESENT, _PX, DUMP, DIGEST, _FB, GS_ or PIXEL, or EM_AREA_CHANGE_LOG (the
 * tick log's digests and logs are the draw modules' own). Developer switch,
 * not a launcher option (docs/LAUNCHER_OPTIONS.md "Not launcher options";
 * proof: tools/test_norender_equal.py). */
#ifndef EM_NORENDER_H
#define EM_NORENDER_H

#include <stdlib.h>
#include <string.h>

#include "em_platform.h"

static inline int em_norender_refused_by_environment(void)
{
    static const char *const picture[] = {"CAPTURE", "PRESENT", "_PX", "DUMP", "DIGEST", "_FB", "GS_", "PIXEL"};
    const char *raster = getenv("EM_HEADLESS_RASTER");
    if (raster && raster[0] == '1') return 1;
    const char *log = getenv("EM_AREA_CHANGE_LOG");
    if (log && log[0]) return 1;
    for (char **e = environ; e && *e; ++e) {
        const char *v = *e;
        if (strncmp(v, "EM_", 3) != 0) continue;
        const char *eq = strchr(v, '=');
        const size_t n = eq ? (size_t)(eq - v) : strlen(v);
        char name[128];
        if (n >= sizeof name) return 1;
        memcpy(name, v, n);
        name[n] = 0;
        for (unsigned i = 0; i < sizeof picture / sizeof picture[0]; ++i)
            if (strstr(name, picture[i])) return 1;
    }
    return 0;
}

/* 1 when the switch is on and honoured. Read once per translation unit. */
static inline int em_norender(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("EM_NO_RENDER");
        on = v && v[0] == '1' && em_headless() && !em_norender_refused_by_environment();
    }
    return on;
}

#endif
