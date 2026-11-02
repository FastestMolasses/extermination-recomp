/* The haze of the creature's head sway (00134300): the grid draw
 * 001E04E0, its vertex emit 001E0240 and the jittered vertex record
 * 001E00F0. From the decomp C, checked against the original instructions
 * by tools/test_area17_creature_reference.py. */
#include "em_area17_creature_local.h"

/* 001E00F0(dst, screen, fog): one vertex record: colour (0x80, 0x80,
 * 0x80), the fog word, the texture coordinates of the screen point moved
 * by a random -10..+10 pixels (two draws of 00122BB8 scaled by 2^-31):
 * s = ((x + 16 (20 ju - 10)) / 16 - 2048) / 512 + 0.5 and t likewise with
 * y and 224, q = 1, the screen x, y, z as integers (001281C0) and the
 * word +0x2C = 0xFF. */
uint32_t a17c_001E00F0(A17C *o, uint32_t dst, uint32_t screen, int32_t fog)
{
    int32_t r = 0;
    uint32_t ju, jv, s, t, v;
    C(00122BB8, &r);
    ju = MUL(0x30000000u, A17C_CVT_S_W(r));
    C(00122BB8, &r);
    W(dst, 0x80);
    W(dst + 4u, 0x80);
    W(dst + 8u, 0x80);
    jv = MUL(0x30000000u, A17C_CVT_S_W(r));
    W(dst + 0xCu, (uint32_t)fog);
    s = R(screen);
    s = ADD(s, MUL(F(16.0f), ADD(F(-10.0f), MUL(F(20.0f), ju))));
    s = ADD(DIV(SUB(DIV(s, F(16.0f)), F(2048.0f)), F(512.0f)), F(0.5f));
    W(dst + 0x10u, s);
    t = R(screen + 4u);
    t = ADD(t, MUL(F(16.0f), ADD(F(-10.0f), MUL(F(20.0f), jv))));
    t = ADD(DIV(SUB(DIV(t, F(16.0f)), F(2048.0f)), F(224.0f)), F(0.5f));
    W(dst + 0x14u, t);
    W(dst + 0x18u, F(1.0f));
    v = R(screen);
    C(001281C0, v, &r);
    W(dst + 0x20u, (uint32_t)r);
    v = R(screen + 4u);
    C(001281C0, v, &r);
    W(dst + 0x24u, (uint32_t)r);
    v = R(screen + 8u);
    C(001281C0, v, &r);
    W(dst + 0x28u, (uint32_t)r);
    W(dst + 0x2Cu, 0xFF);
    return 0;
}

/* 001E0240(dst, vector, fog, seed): the vector through the screen matrix
 * 0x70003AC0 (001026A0 into the frame vector at sp - 0x10), divided by
 * w / 16; the result `seed >> 1` gains bit 8 when the scale is negative
 * or the screen x (or else y) lies outside 0..65520; the record
 * 001E00F0(dst, frame vector, fog); with any bit set +0x2C = 0x8000. */
int32_t a17c_001E0240(A17C *o, uint32_t dst, uint32_t vector, int32_t fog, int32_t seed, uint32_t sp)
{
    const uint32_t buf = sp - 0x10u;
    int32_t flags = a17c_sra((uint32_t)seed, 1);
    uint32_t scale, v;
    C(001026A0, buf, 0x70003AC0u, vector);
    scale = DIV(F(16.0f), R(buf + 0xCu));
    if (LT(scale, 0)) flags |= 8;
    v = R(buf);
    W(buf, MUL(v, scale));
    v = R(buf + 4u);
    W(buf + 4u, MUL(v, scale));
    v = R(buf + 8u);
    W(buf + 8u, MUL(v, scale));
    v = R(buf);
    if (LT(v, 0) || !LE(v, F(65520.0f))) {
        flags |= 8;
    } else {
        v = R(buf + 4u);
        if (LT(v, 0) || !LE(v, F(65520.0f))) flags |= 8;
    }
    SIB(a17c_001E00F0(o, dst, buf, fog));
    if (flags != 0) W(dst + 0x2Cu, 0x8000);
    return flags;
}

/* One packet header of 001E04E0 at the render context's cursor
 * ((0x275670)+0x1C, read again for each field): byte +3 = 0x10, word +4
 * = 0, halfword +0 = 0x62; the cursor moves by 0x630; the quadword +0x10
 * is cleared, +0x1C = 0x50000061, the GIF tag at +0x20 / +0x28. Returns
 * the packet. */
static uint32_t a17c_packet(A17C *o)
{
    const uint32_t ctx = R(0x275670u);
    uint32_t p;
    WB(R(ctx + 0x1Cu) + 3u, 0x10);
    W(R(ctx + 0x1Cu) + 4u, 0);
    WH(R(ctx + 0x1Cu), 0x62);
    p = R(ctx + 0x1Cu);
    W(ctx + 0x1Cu, p + 0x630u);
    a17c_w64(o, (p + 0x10u) & ~15u, 0);
    a17c_w64(o, ((p + 0x10u) & ~15u) + 8u, 0);
    W(p + 0x1Cu, 0x50000061u);
    a17c_w64(o, p + 0x20u, UINT64_C(0x302E400000008020));
    a17c_w64(o, p + 0x28u, 0x421);
    return p;
}

/* 001E04E0(offset, fog, t): the haze grid. Unit 3 is set up with the
 * texture buffer 0x258000 (001D6B10, 001D1F80, 001D6BA0, 001D1FF0); the
 * matrix 001CD370(0) is read row by row; for each of the 15 rows of the
 * 16 x 16 vertex grid 0x81C0F0 a packet of two strip vertices per column
 * (rows i and i + 1): each vertex is the grid point scaled by t
 * (00103230), moved by the offset (001028B8) and emitted (001E0240, its
 * result threaded through the row). Then 001D1F20(3), 001D1FF0(3, 1), the
 * closing packet (byte +3 = 0x60, the cursor moves by 0x10) and the kick
 * 001CAAC0(offset, the cursor at the start). */
uint32_t a17c_001E04E0(A17C *o, uint32_t offset, int32_t fog, uint32_t t, uint32_t sp)
{
    const uint32_t v = sp - 0x10u, inner = sp - 0xA0u;
    uint32_t ctx, wp0, m = 0, p, dl, i, j, k;
    int32_t acc;
    ctx = R(0x275670u);
    wp0 = R(ctx + 0x1Cu);
    C(001D6B10, 3, 0x258000, 8, 8);
    C(001D1F80, 3, 2, 1);
    C(001D6BA0, 3, 0x258000, 8, 8, 0, 0);
    C(001D1FF0, 3, 3);
    C(001CD370, 0, &m);
    for (k = 0; k < 4; ++k) {
        const uint32_t row = (m + k * 0x10u) & ~15u;
        for (i = 0; i < 4; ++i) (void)R(row + i * 4u);
    }
    for (i = 0; i < 15; ++i) {
        p = a17c_packet(o);
        if (a17c_failed(o)) return 0;
        dl = p + 0x30u;
        acc = 0;
        for (j = 0; j < 16; ++j) {
            C(00103230, v, 0x81C0F0u + i * 0x100u + j * 0x10u, t);
            C(001028B8, v, offset, v);
            acc = a17c_001E0240(o, dl, v, fog, acc, inner);
            if (a17c_failed(o)) return 0;
            dl += 0x30u;
            C(00103230, v, 0x81C0F0u + (i + 1u) * 0x100u + j * 0x10u, t);
            C(001028B8, v, offset, v);
            acc = a17c_001E0240(o, dl, v, fog, acc, inner);
            if (a17c_failed(o)) return 0;
            dl += 0x30u;
        }
    }
    C(001D1F20, 3);
    C(001D1FF0, 3, 1);
    ctx = R(0x275670u);
    WB(R(ctx + 0x1Cu) + 3u, 0x60);
    W(R(ctx + 0x1Cu) + 4u, 0);
    WH(R(ctx + 0x1Cu), 0);
    p = R(ctx + 0x1Cu);
    W(ctx + 0x1Cu, p + 0x10u);
    C(001CAAC0, offset, wp0);
    return 0;
}
