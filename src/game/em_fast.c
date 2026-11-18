/* em_fast.c - see em_fast.h. */
#include "game/em_fast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "em_platform.h"
#include "game/em_bgm.h"
#include "game/em_frame.h"

int em_fast_flag;
int em_fast_norender_ok;

static uint32_t s_seg, s_off;
static int s_requested;
static void (*s_end)(void *);
static void *s_user;
static struct timespec s_t0;

/* A picture or GS diagnostic still needs the drawn frames (em_norender.h's
 * list), but not a capture: the playback holds those until the switch. */
static int refuses_skip(void)
{
    static const char *const picture[] = {"PRESENT", "_PX", "DUMP", "DIGEST", "_FB", "GS_", "PIXEL"};
    const char *log = getenv("EM_AREA_CHANGE_LOG");
    if (log && log[0]) return 1;
    for (char **e = environ; e && *e; ++e) {
        if (strncmp(*e, "EM_", 3) != 0) continue;
        const char *eq = strchr(*e, '=');
        const size_t n = eq ? (size_t)(eq - *e) : strlen(*e);
        char name[128];
        if (n >= sizeof name) return 1;
        memcpy(name, *e, n);
        name[n] = 0;
        for (unsigned i = 0; i < sizeof picture / sizeof picture[0]; ++i)
            if (strstr(name, picture[i])) return 1;
    }
    return 0;
}

int em_fast_init(void)
{
    const char *v = getenv("EM_FAST_UNTIL");
    if (!v || !v[0]) return 0;
    char *end = NULL;
    const unsigned long seg = strtoul(v, &end, 10);
    if (end == v || *end != ':') {
        fprintf(stderr, "EM_FAST_UNTIL=%s: want <segment>:<offset>\n", v);
        return -1;
    }
    const char *rest = end + 1;
    const unsigned long off = strtoul(rest, &end, 10);
    if (end == rest || *end) {
        fprintf(stderr, "EM_FAST_UNTIL=%s: want <segment>:<offset>\n", v);
        return -1;
    }
    s_seg = (uint32_t)seg;
    s_off = (uint32_t)off;
    s_requested = 1;
    const char *raster = getenv("EM_HEADLESS_RASTER");
    em_fast_norender_ok = !refuses_skip() && !(raster && raster[0] == '1');
    em_fast_flag = 1;
    clock_gettime(CLOCK_MONOTONIC, &s_t0);
    printf("fast: playing to segment %u offset %u with no picture, no sound and no pacing\n", s_seg, s_off);
    return 1;
}

int em_fast_start_unpaced(void)
{
    if (em_fast_flag) em_frame_set_unpaced(1);
    return em_fast_flag;
}

int em_fast_target(uint32_t *segment, uint32_t *offset)
{
    if (!s_requested) return 0;
    if (segment) *segment = s_seg;
    if (offset) *offset = s_off;
    return 1;
}

void em_fast_bind(void (*end)(void *), void *user)
{
    s_end = end;
    s_user = user;
}

void em_fast_end(void)
{
    if (!em_fast_flag) return;
    em_fast_flag = 0;
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("fast: reached segment %u offset %u in %.1f s\n", s_seg, s_off,
           (double)(t1.tv_sec - s_t0.tv_sec) + (double)(t1.tv_nsec - s_t0.tv_nsec) / 1e9);
    fflush(stdout);
    em_frame_set_unpaced(0);
    em_bgm_fast_end();      /* drops the skipped part's sound, then unmutes */
    if (s_end) s_end(s_user);
}
