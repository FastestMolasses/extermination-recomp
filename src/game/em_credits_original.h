/* em_credits_original.h - the ending's credits: 0022D360 (the reset) and
 * 0022D380 (the scroll), both byte-matched C in the decomp
 * (src/func_0022D360.c, src/func_0022D380.c). 001AD740 state 0 resets them
 * and state 1 sub 1 / sub 2 call the scroll once per frame
 * (em_scene_task.c em_sf_001AD740). Oracle: tools/test_credits_reference.py.
 *
 * The state D_00822CD0 (0x20 bytes, .bss): +0 the state byte (0 start,
 * 1 scrolling, 2 the hold), +8 the scroll y (float), +0x14 the frame count,
 * +0x18 the hold timer, +0x1C the fraction (float). 0022D360 is
 * 00121A28(D_00822CD0, 0, 32): the 32 bytes cleared.
 *
 * The table D_00268B40 (.data): rows of {type, text address}, ended by the
 * type 4. Each row is 0x15 lines below the one before. Types 1, 2 and 3
 * draw the text centred at 0x800 - width / 2 (001CC170, arithmetic shift),
 * twice in the shadow colour one pixel left and right, then once in the
 * text colour, all through 001CC1E0(1, x, y + 0x800, 10, 20, text, colour);
 * type 1 also underlines it with 00207F80(1, ...). Type 0 draws nothing.
 * Rows at y <= -0x90 are skipped, the first at y >= 0x91 ends the frame.
 *
 * The two colour records are 8 bytes on the stack: +0 the word 0x606060,
 * +4 0x80 (text) or 0x28 (shadow), +5 0, +6 never written by the original
 * (stack garbage; 001CC1E0's packet builder 001CC3B0 does not read it, the
 * port passes 0), +7 the half-line offset 0 or 12 (sixteenths of a pixel),
 * from the scroll's fraction. */
#ifndef EM_CREDITS_ORIGINAL_H
#define EM_CREDITS_ORIGINAL_H

#include <stdint.h>

#define EM_CREDITS_STATE 0x00822CD0u
#define EM_CREDITS_TABLE 0x00268B40u

typedef struct {
    void *ctx;
    /* `size` readable bytes at an original .data address (the table's
     * rows), or NULL (fault). */
    const uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
    /* 001CC170(text): *width. 0, or -1 (fault). */
    int (*cc170)(void *ctx, uint32_t text, int32_t *width);
    /* 001CC1E0(slot, x, y, w, h, text, colour record). 0, or -1. */
    int (*cc1e0)(void *ctx, int32_t slot, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t text,
                 const uint8_t colour[8]);
    /* 00207F80(slot, x0, y0, x1, y1, rgba). 0, or -1. */
    int (*f80)(void *ctx, int32_t slot, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t rgba);
} EmCreditsHooks;

/* 0022D360: the state cleared. */
void em_credits_0022D360(uint8_t state[0x20]);
/* 0022D380: one frame; *result is its return (1 once the hold ends).
 * 0, or -1 when a hook faulted (the frame stops there). */
int em_credits_0022D380(uint8_t state[0x20], const EmCreditsHooks *h, int32_t *result);

#endif
