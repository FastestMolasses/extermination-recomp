#ifndef EM_SNOW_H
#define EM_SNOW_H

#include "game/em_weather.h"

enum { EM_SNOW_TILE_COUNT = 108 };

typedef struct EmSnowConfig {
    float descriptor[9][4];
    float lookup[80];
    float rows[6][12];
} EmSnowConfig;

typedef struct EmSnowTile {
    float descriptor[9][4];
    float params[4];
    float matrix[16];
} EmSnowTile;

/* Loads locally exported original tables; no guessed fallback geometry. */
int em_snow_config_load(EmSnowConfig *config, const char *path);

/* 0011E2A8(x), the SDK sinf 001E67C0 calls for each row's drift wave: 0
 * with *result, or -1 (a fault). Live: em_sdk_math_original's translation
 * (em_sdk_math_original_w_0011E2A8 over the one SDK context). */
typedef int (*EmSnowSine)(void *ctx, float x, float *result);

/* 001E67C0's tile emission (flags bit 1 clear: AREA11 and every area but
 * AREA21). Advances the renderer-owned phase fields once per row. Each
 * tile carries the actual VU50..5D submission parameters. Returns 0, or -1
 * when the SDK VU0 scale 00102900 faults (em_ee_float.h refused a form) or
 * the sine worker faults; the caller fail-stops. */
int em_snow_tiles(EmWeather *weather, const EmSnowConfig *config,
                   float strength, const float eye[3],
                   EmSnowSine sine, void *sine_ctx,
                   EmSnowTile tiles[EM_SNOW_TILE_COUNT]);
/* The same with 001E67C0's flags bit 1 (`reversed`): 001E55F0 sets it when
 * the area key (D_00810700 << 8) + D_00810701 is 0x1500 (AREA21 sub 0). The
 * row tilt is then -row[0] minus the wobble (instead of row[0] plus it), and
 * each row's phase moves down by 1.5 x row[2] x strength, wrapping below
 * 1.0 by + 1.0 (instead of up, wrapping above 2.0 by - 1.0); the drift
 * still grows by 0.004 x strength (decomp func_001E67C0.c). */
int em_snow_tiles_mode(EmWeather *weather, const EmSnowConfig *config,
                       float strength, const float eye[3],
                       EmSnowSine sine, void *sine_ctx, int reversed,
                       EmSnowTile tiles[EM_SNOW_TILE_COUNT]);

#endif
