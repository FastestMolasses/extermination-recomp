/* em_fade_packets.c - see em_fade_packets.h. Translated from the decomp C
 * of 001AEA50, 001AE900, 001AEBE0 and 001AEE70 (all byte-matched). */
#include "game/em_fade_packets.h"

#include <stddef.h>
#include <string.h>

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void put64(uint8_t *p, uint64_t v)
{
    put32(p, (uint32_t)v);
    put32(p + 4, (uint32_t)(v >> 32));
}

/* 001AE900's two 0x90-byte blocks: the VIF and GIF header (FLUSHA, DIRECT
 * 8; one loop of a sprite with blending, seven registers), ALPHA_2 = 0xA1
 * with FIX 0x80, then per bar an RGBAQ (255, 255, 255, 0x80) and its two
 * corners (the D_0024A8C0 halfwords times 16, Z 0x0FFFFFE0). */
static void blocks_001AE900(uint8_t *b, const int16_t corners[8])
{
    for (int j = 0; j < 2; ++j, b += 0x90) {
        static const uint32_t head[8] = {0x13000000u, 0, 0, 0x50000008u, 0x8001u, 0x71A34000u, 0xF441441Eu,
                                         0xFFFFFFFFu};
        for (int i = 0; i < 8; ++i) put32(b + 4 * i, head[i]);
        put64(b + 0x20, 0xA1u | (uint64_t)0x80u << 32);
        put64(b + 0x28, 0x43u);
        uint8_t *e = b + 0x30;
        const int16_t *src = corners;
        for (int i = 0; i < 2; ++i, e += 0x30, src += 4) {
            put32(e + 0x00, 0xFFu);
            put32(e + 0x04, 0xFFu);
            put32(e + 0x08, 0xFFu);
            put32(e + 0x0C, 0x80u);
            put32(e + 0x10, (uint32_t)((int32_t)src[0] * 0x10));
            put32(e + 0x14, (uint32_t)((int32_t)src[1] * 0x10));
            put32(e + 0x18, 0x0FFFFFE0u);
            put32(e + 0x1C, 0);
            put32(e + 0x20, (uint32_t)((int32_t)src[2] * 0x10));
            put32(e + 0x24, (uint32_t)((int32_t)src[3] * 0x10));
            put32(e + 0x28, 0x0FFFFFE0u);
            put32(e + 0x2C, 0);
        }
    }
}

void em_fpk_001AEA50(EmFadePackets *p, const int16_t corners[8])
{
    /* the two 0x60-byte fade blocks: FLUSH, DIRECT 5; one loop of a sprite
     * with blending, four registers; ALPHA_2 0xA1 / FIX 0x80; RGBAQ (255,
     * 255, 255, 0x80); the corners (0x7000, 0x7900) and (0x9000, 0x8700),
     * Z 0x0FFFFFF0 */
    static const uint32_t words[24] = {0x11000000u, 0, 0, 0x50000005u, 0x8001u, 0x41A34000u, 0xFFFF441Eu,
                                       0xFFFFFFFFu, 0xA1u, 0x80u, 0x43u, 0, 0xFFu, 0xFFu, 0xFFu, 0x80u,
                                       0x7000u, 0x7900u, 0x0FFFFFF0u, 0, 0x9000u, 0x8700u, 0x0FFFFFF0u, 0};
    for (int i = 0; i < 2; ++i)
        for (int w = 0; w < 24; ++w) put32(p->fade + 0x60 * i + 4 * w, words[w]);
    blocks_001AE900(p->letterbox, corners);
}

/* The REF both append at the cursor: half +0 the quadword count, byte +3
 * 0x30 (REF), word +4 the block; the other bytes stay as they were. */
static void ref(uint8_t *tag, uint16_t qwc, uint32_t block)
{
    tag[3] = 0x30u;
    put32(tag + 4, block);
    tag[0] = (uint8_t)qwc;
    tag[1] = (uint8_t)(qwc >> 8);
}

void em_fpk_001AEBE0(EmFadePackets *p, int parity, int32_t level, int draw, uint8_t *tag)
{
    uint8_t *b = p->letterbox + 0x90 * (parity & 1);
    for (int i = 0; i < 2; ++i) {
        put32(b + 0x30 + 0x30 * i, (uint32_t)level);
        put32(b + 0x34 + 0x30 * i, (uint32_t)level);
        put32(b + 0x38 + 0x30 * i, (uint32_t)level);
    }
    if (draw && tag) ref(tag, 9, EM_FPK_D_0028A7B0 + 0x90u * (uint32_t)(parity & 1));
}

void em_fpk_001AEE70(EmFadePackets *p, int parity, int16_t level, int8_t colour, int draw, uint8_t *tag)
{
    uint8_t *b = p->fade + 0x60 * (parity & 1);
    put32(b + 0x30, (uint32_t)(int32_t)level);
    put32(b + 0x34, (uint32_t)(int32_t)level);
    put32(b + 0x38, (uint32_t)(int32_t)level);
    put64(b + 0x20, (colour == 0 ? 0xA1u : 0x68u) | (uint64_t)0x80u << 32);
    if (draw && tag) ref(tag, 6, EM_FPK_D_0028A8E0 + 0x60u * (uint32_t)(parity & 1));
}

const uint8_t *em_fpk_bytes(const EmFadePackets *p, uint32_t address, uint32_t size)
{
    if (!p) return NULL;
    if (address >= EM_FPK_D_0028A7B0 && size <= EM_FPK_LETTERBOX_BYTES &&
        address - EM_FPK_D_0028A7B0 <= EM_FPK_LETTERBOX_BYTES - size)
        return p->letterbox + (address - EM_FPK_D_0028A7B0);
    if (address >= EM_FPK_D_0028A8E0 && size <= EM_FPK_FADE_BYTES &&
        address - EM_FPK_D_0028A8E0 <= EM_FPK_FADE_BYTES - size)
        return p->fade + (address - EM_FPK_D_0028A8E0);
    return NULL;
}
