/* em_fade_packets.h - the GS packets of the two screen effects: the
 * letterbox bars 001AEBE0 and the transition fade 001AEE70 (GS_EXACT.md
 * section 9 "Slot 1").
 *
 * Their state machines are em_fade.c's (EmScreenFade, EmTransitionFade);
 * this is the other half of each function: the blocks it sends and the
 * slot-1 REF it appends.
 *   001AEA50 (with 001AE900, boot 001AB430): the two fade blocks
 *            D_0028A8E0 + parity * 0x60 and the two letterbox blocks
 *            D_0028A7B0 + parity * 0x90, the bars' corners from the
 *            boot ELF's D_0024A8C0 (eight halfwords, times 16);
 *   001AEBE0 after its state step: the level (the int D_0028A8D4) into the
 *            three colour words of both entries of block D_00810E80, then,
 *            when it draws, a REF of that block (9 quadwords) at the
 *            render context's +0x14 cursor;
 *   001AEE70 after its state step: the level (the short +0xC4) into the
 *            three colour words of block D_00810E80, its ALPHA (0xA1 when
 *            the colour byte +0xC2 is 0, else 0x68, FIX 0x80), then, when
 *            the mode byte is non-zero, a REF of that block (6 quadwords).
 * The REFs write bytes +0 / +1 (qwc), +3 (0x30) and +4..+7 (the block's
 * address) of the cursor's quadword, as the originals do; the rest stays.
 * tools/test_fade_packets_reference.py runs the original instructions. */
#ifndef EM_FADE_PACKETS_H
#define EM_FADE_PACKETS_H

#include <stdint.h>

#define EM_FPK_D_0028A7B0 0x0028A7B0u /* the letterbox blocks, 2 x 0x90 */
#define EM_FPK_D_0028A8E0 0x0028A8E0u /* the fade blocks, 2 x 0x60 */
#define EM_FPK_LETTERBOX_BYTES 0x120u
#define EM_FPK_FADE_BYTES 0xC0u

typedef struct EmFadePackets {
    uint8_t letterbox[EM_FPK_LETTERBOX_BYTES];
    uint8_t fade[EM_FPK_FADE_BYTES];
} EmFadePackets;

/* 001AEA50's and 001AE900's block bytes (their state words are em_fade's).
 * `corners` is D_0024A8C0: per bar x0, y0, x1, y1. */
void em_fpk_001AEA50(EmFadePackets *p, const int16_t corners[8]);
/* 001AEBE0's packet half: `level` (the post-step D_0028A8D4) into block
 * `parity`; with `draw` (the original's state was non-zero and the gate
 * open: em_screen_fade_tick's result) the REF into `tag` (16 bytes; may be
 * NULL when not drawing). */
void em_fpk_001AEBE0(EmFadePackets *p, int parity, int32_t level, int draw, uint8_t *tag);
/* 001AEE70's packet half: `level` the post-step +0xC4, `colour` +0xC2,
 * `draw` the mode byte was non-zero (em_transition_fade_tick's result). */
void em_fpk_001AEE70(EmFadePackets *p, int parity, int16_t level, int8_t colour, int draw, uint8_t *tag);
/* The host bytes of [address, address + size) inside the blocks, or NULL. */
const uint8_t *em_fpk_bytes(const EmFadePackets *p, uint32_t address, uint32_t size);

#endif
