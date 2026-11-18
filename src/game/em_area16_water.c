/* AREA16's water grid: 001DAFA0, 001DB240, 001DB250, 001DB040, 001DB480
 * with 001DAE10 and 001DAC90 (em_area16_water.h). From the decomp C
 * (src/func_001DAFA0.c .. func_001DB480.c; 001DB040 and 001DAC90 are
 * NEARMISS, so the access order follows the original instructions, read
 * locally and never reproduced), checked against the original instructions
 * by tools/test_area16_water_reference.py.
 *
 * The grid at 0x818000: 32 rows of 32 cells, 0x10 bytes each (u, v, the
 * height velocity, the height), rows 0x200 apart. Its block at 0x81C000:
 * +0x00 damping, +0x04 spring rate, +0x08 stiffness, +0x0C impulse, +0x11
 * the stage byte (0x81C011), +0x20 the origin, +0x28 its height, +0x30 the
 * shade quadword (four words that ease toward 0xC0; the last is the fog
 * word of 001DAC90). The names say what the code does; they are not
 * verified game semantics. */
#include "em_area16_water_internal.h"

#define B(a) a16w_u8(o, (a))
#define R(a) a16w_u32(o, (a))
#define W(a, v) a16w_w32(o, (a), (v))
#define WB(a, v) a16w_w8(o, (a), (v))
#define WH(a, v) a16w_w16(o, (a), (v))
#define F(v) a16w_bits(v)
/* A hook call; a failed or missing hook ends the translation. */
#define C(fn, ...) do { if (a16w_c_##fn(o, __VA_ARGS__)) return 0; } while (0)
#define C0(fn) do { if (a16w_c_##fn(o)) return 0; } while (0)
/* A call of another translation; its fault ends this one too. */
#define SIB(call) do { (void)(call); if (a16w_failed(o)) return 0; } while (0)
#define ADD(a, b) A16W_ADD((a), (b))
#define SUB(a, b) A16W_SUB((a), (b))
#define MUL(a, b) A16W_MUL((a), (b))
#define DIV(a, b) A16W_DIV((a), (b))
#define LT(a, b) A16W_LT((a), (b))
#define LE(a, b) A16W_LE((a), (b))

#define GRID 0x00818000u
#define BLOCK 0x0081C000u
#define STAGE 0x0081C011u
#define CTX_PTR 0x00275670u /* the render context pointer; its +0x1C is the packet cursor */
#define SCREEN_MATRIX 0x70003AC0u
#define ROW 0x200u

/* ------------------------------------------------------------------------
 * 001DAFA0: every cell's velocity and height (+0x0C, then +0x08) zeroed,
 * row by row; then the block: damping 0.991, rate 0.022, impulse 0.4,
 * stiffness 0.05, the origin (306.1, 184.0, 460.2256) and +0x2C = 1.0.
 * ---------------------------------------------------------------------- */
uint32_t a16w_001DAFA0(A16W *o)
{
    for (uint32_t i = 0; i < 32; ++i)
        for (uint32_t j = 0; j < 32; ++j) {
            const uint32_t c = GRID + i * ROW + j * 0x10u;
            W(c + 0xCu, 0);
            W(c + 0x8u, 0);
        }
    W(BLOCK + 0x00u, 0x3F7DB22Du);
    W(BLOCK + 0x04u, 0x3CB43958u);
    W(BLOCK + 0x0Cu, 0x3ECCCCCDu);
    W(BLOCK + 0x08u, 0x3D4CCCCDu);
    W(BLOCK + 0x20u, 0x43990CCDu);
    W(BLOCK + 0x24u, 0x43380000u);
    W(BLOCK + 0x28u, 0x43E61CE0u);
    W(BLOCK + 0x2Cu, 0x3F800000u);
    return 0;
}

/* 001DB240(stage): the stage byte 0x81C011 = stage. */
uint32_t a16w_001DB240(A16W *o, int32_t stage)
{
    WB(STAGE, (uint32_t)stage);
    return 0;
}

/* ------------------------------------------------------------------------
 * 001DB040(grid): one wave step over the 32 x 32 cells (the block at
 * grid + 0x4000). With the neighbours clamped to the edges (up = row
 * i - 1, down = row i + 1, left / right = column j -+ 1):
 *   lap = down.h + (up.h + (left.h + right.h)), then less (4 + stiffness)
 *         * h in the one rounding of the EE's multiply-subtract;
 *   vel = vel * damping (stored), then vel + lap * rate (stored again);
 *   u = 2 (left.h - right.h); v = 2 (up.h - down.h).
 * Then one random cell, row and column each (00122BB8 >> 16) % 32 (the
 * C remainder: negative for a negative draw), gains the impulse in its
 * velocity; then every height integrates h + vel, at most 70.0.
 * ---------------------------------------------------------------------- */
uint32_t a16w_001DB040(A16W *o, uint32_t g)
{
    const uint32_t stiff_raw = R(g + 0x4008u);
    const uint32_t damp = R(g + 0x4000u);
    const uint32_t rate = R(g + 0x4004u);
    const uint32_t stiff = ADD(F(4.0f), stiff_raw);
    int32_t r = 0, row, col;
    uint32_t i, j;
    for (i = 0; i < 32; ++i) {
        const uint32_t up = i == 0 ? 0 : i - 1, down = i + 1 >= 32 ? 31 : i + 1;
        const uint32_t base = g + i * ROW;
        for (j = 0; j < 32; ++j) {
            const uint32_t left = j == 0 ? 0 : j - 1, right = j + 1 >= 32 ? 31 : j + 1;
            const uint32_t c = base + j * 0x10u;
            const uint32_t a = base + left * 0x10u, b = base + right * 0x10u;
            const uint32_t t0 = g + up * ROW + j * 0x10u, t1 = g + down * ROW + j * 0x10u;
            uint32_t hu = R(t0 + 0xCu), hl = R(a + 0xCu), hr = R(b + 0xCu), hd = R(t1 + 0xCu);
            const uint32_t h = R(c + 0xCu), vel = R(c + 0x8u);
            uint32_t sum = ADD(hl, hr);
            sum = ADD(hu, sum);
            const uint32_t lap = em_ee_msub_bits(em_ee_adda_bits(hd, sum), stiff, h);
            const uint32_t damped = MUL(vel, damp);
            const uint32_t push = MUL(lap, rate);
            W(c + 0x8u, damped);
            W(c + 0x8u, ADD(damped, push));
            hl = R(a + 0xCu);
            hr = R(b + 0xCu);
            W(c + 0x0u, MUL(F(2.0f), SUB(hl, hr)));
            hu = R(t0 + 0xCu);
            hd = R(t1 + 0xCu);
            W(c + 0x4u, MUL(F(2.0f), SUB(hu, hd)));
        }
    }
    C(00122BB8, &r);
    row = a16w_sra((uint32_t)r, 16) % 32;
    C(00122BB8, &r);
    col = a16w_sra((uint32_t)r, 16) % 32;
    {
        const uint32_t impulse = R(g + 0x400Cu);
        const uint32_t c = g + (uint32_t)row * ROW + (uint32_t)col * 0x10u;
        const uint32_t vel = R(c + 0x8u);
        W(c + 0x8u, ADD(vel, impulse));
    }
    for (i = 0; i < 32; ++i)
        for (j = 0; j < 32; ++j) {
            const uint32_t c = g + i * ROW + j * 0x10u;
            const uint32_t h0 = R(c + 0xCu);
            uint32_t h = ADD(h0, R(c + 0x8u));
            if (!LE(h, F(70.0f))) h = F(70.0f);
            W(c + 0xCu, h);
        }
    return 0;
}

/* ------------------------------------------------------------------------
 * 001DB250: the stage byte selects (the original's jump table; a value
 * above 6 is first reset to 0 and runs as stage 0):
 *   0  the shade words (0x71, 0x53, 0x71, 0) and damping 0.991, rate
 *      0.0176, stiffness 0.05, impulse 0.2;
 *   1  damping 0.991, rate 0.022, stiffness 0.05, impulse 0.4;
 *   2  the stage back to 0;
 *   3, 4  impulse 2.0, the stage + 1;
 *   5  damping * 0.9995, rate * 0.2, impulse * 0.99, stiffness 0; the
 *      stage + 1 once the impulse is below 0.5;
 *   6  nothing.
 * From stage 3 on, each shade word moves by (0xC0 - word) >> 6 and the
 * origin height by 0.1 (452.2256 - height). Below stage 6 the wave step
 * 001DB040(0x818000) runs.
 * ---------------------------------------------------------------------- */
uint32_t a16w_001DB250(A16W *o)
{
    uint32_t v;
    switch (B(STAGE)) {
    default:
        WB(BLOCK + 0x11u, 0);
        /* fall through */
    case 0:
        W(BLOCK + 0x30u, 0x71);
        W(BLOCK + 0x34u, 0x53);
        W(BLOCK + 0x38u, 0x71);
        W(BLOCK + 0x3Cu, 0);
        W(BLOCK + 0x00u, 0x3F7DB22Du);
        W(BLOCK + 0x04u, 0x3C902DE0u);
        W(BLOCK + 0x08u, 0x3D4CCCCDu);
        W(BLOCK + 0x0Cu, 0x3E4CCCCDu);
        break;
    case 1:
        W(BLOCK + 0x00u, 0x3F7DB22Du);
        W(BLOCK + 0x04u, 0x3CB43958u);
        W(BLOCK + 0x08u, 0x3D4CCCCDu);
        W(BLOCK + 0x0Cu, 0x3ECCCCCDu);
        break;
    case 2:
        WB(BLOCK + 0x11u, 0);
        break;
    case 3:
    case 4:
        W(BLOCK + 0x0Cu, 0x40000000u);
        v = B(BLOCK + 0x11u);
        WB(BLOCK + 0x11u, v + 1u);
        break;
    case 5:
        v = R(BLOCK + 0x00u);
        W(BLOCK + 0x00u, MUL(v, 0x3F7FDF3Bu));
        v = R(BLOCK + 0x04u);
        W(BLOCK + 0x04u, MUL(v, 0x3E4CCCCDu));
        v = R(BLOCK + 0x0Cu);
        W(BLOCK + 0x0Cu, MUL(v, 0x3F7D70A4u));
        W(BLOCK + 0x08u, 0);
        v = R(BLOCK + 0x0Cu);
        if (LT(v, F(0.5f))) {
            v = B(BLOCK + 0x11u);
            WB(BLOCK + 0x11u, v + 1u);
        }
        break;
    case 6:
        break;
    }
    if (B(BLOCK + 0x11u) >= 3u) {
        for (uint32_t k = 0; k < 4; ++k) {
            const uint32_t at = BLOCK + 0x30u + 4u * k;
            const uint32_t w = R(at);
            W(at, w + (uint32_t)a16w_sra(0xC0u - w, 6));
        }
        const uint32_t height = R(BLOCK + 0x28u);
        W(BLOCK + 0x28u, ADD(height, MUL(0x3DCCCCCDu, SUB(0x43E21CE0u, height))));
    }
    if (B(BLOCK + 0x11u) != 6u) SIB(a16w_001DB040(o, GRID));
    return 0;
}

/* ------------------------------------------------------------------------
 * 001DAC90(dst, screen, uv, shade): one vertex record. The shade quadword
 * is copied to dst; when its fog word (+0x0C) is 0 the record's fog word
 * is float_to_int(uv[2] + (0.0 + k uv[0])) + 0x10 with k = 5.2 for
 * uv[0] > 0, else 0.5; otherwise the shade's word is kept. Then
 *   s = 0.5 + ((x + 16 uv[0]) / 8192 - 4), t = 0.5 + ((y + 16 uv[1]) /
 *   3584 - 9.142858), q = 1.0,
 * the screen x, y, z through float_to_int (001281C0) and +0x2C = 0xFF.
 * ---------------------------------------------------------------------- */
uint32_t a16w_001DAC90(A16W *o, uint32_t dst, uint32_t screen, uint32_t uv, uint32_t shade)
{
    int32_t r = 0;
    uint32_t x, y, k;
    {
        const uint64_t lo = a16w_u64(o, shade & ~15u), hi = a16w_u64(o, (shade & ~15u) + 8u);
        a16w_w64(o, dst & ~15u, lo);
        a16w_w64(o, (dst & ~15u) + 8u, hi);
    }
    const uint32_t fog = R(shade + 0xCu);
    if (fog == 0) {
        const uint32_t u = R(uv);
        k = LE(u, 0) ? F(0.5f) : 0x40A66666u /* 5.2 */;
        const uint32_t depth = R(uv + 8u);
        C(001281C0, ADD(depth, ADD(A16W_CVT_S_W(fog), MUL(k, u))), &r);
        W(dst + 0xCu, (uint32_t)r + 0x10u);
    } else {
        W(dst + 0xCu, fog);
    }
    k = R(uv);
    x = R(screen);
    W(dst + 0x10u, ADD(F(0.5f), SUB(DIV(ADD(x, MUL(F(16.0f), k)), F(8192.0f)), F(4.0f))));
    k = R(uv + 4u);
    y = R(screen + 4u);
    W(dst + 0x14u, ADD(F(0.5f), SUB(DIV(ADD(y, MUL(F(16.0f), k)), F(3584.0f)), 0x41124925u)));
    W(dst + 0x18u, 0x3F800000u);
    x = R(screen);
    C(001281C0, x, &r);
    W(dst + 0x20u, (uint32_t)r);
    y = R(screen + 4u);
    C(001281C0, y, &r);
    W(dst + 0x24u, (uint32_t)r);
    x = R(screen + 8u);
    C(001281C0, x, &r);
    W(dst + 0x28u, (uint32_t)r);
    W(dst + 0x2Cu, 0xFF);
    return 0;
}

/* The VU0 clip test of v.xyz against |v.w| (vclipw.xyz; flag bits +x, -x,
 * +y, -y, +z, -z): denormals as zero, magnitude compares. A lane with
 * exponent 255 is outside the measured model: fault. */
static uint32_t a16w_clipw(A16W *o, const uint32_t v[4])
{
    for (int i = 0; i < 4; ++i)
        if (((v[i] >> 23) & 0xFFu) == 0xFFu) {
            a16w_latch(o, 0x001DAE10u, EM_AREA16_WATER_FAULT_UNMEASURED);
            return 0;
        }
    const uint32_t w = em_eei_daz(v[3]) & 0x7FFFFFFFu;
    uint32_t f = 0;
    for (int k = 0; k < 3; ++k) {
        const uint32_t x = em_eei_daz(v[k]);
        if ((x & 0x7FFFFFFFu) > w) f |= (x >> 31) ? 2u << (2 * k) : 1u << (2 * k);
    }
    return f;
}

/* ------------------------------------------------------------------------
 * 001DAE10(dst, pos, uv, shade): the screen point p = 0x70003AC0 * pos
 * (001026A0 into the frame vector at sp - 0x10); pos (all four words) is
 * also transformed on VU0 by the rows vf24..vf27 (rows[0] * x + rows[1] *
 * y + rows[2] * z + rows[3] * 1.0) and clipped against its w. The vertex
 * is rejected when a clip flag is set, when s = 16 / p.w is negative, or
 * when the scaled x (else the scaled y) is below 0 or not at most 65520;
 * p.xyz *= s (stored). Then 001DAC90(dst, p, uv, shade), and a rejected
 * vertex gets +0x2C = 0x8000.
 * ---------------------------------------------------------------------- */
uint32_t a16w_001DAE10(A16W *o, uint32_t dst, uint32_t pos, uint32_t uv, uint32_t shade, uint32_t sp,
                       const uint32_t rows[4][4])
{
    static const uint32_t vf0[4] = {0, 0, 0, EM_EE_ONE};
    const uint32_t p = sp - 0x10u;
    uint32_t v[4], acc[4] = {0, 0, 0, 0}, clip[4] = {0, 0, 0, 0}, s, x;
    int reject = 0;
    C(001026A0, p, SCREEN_MATRIX, pos);
    for (int k = 0; k < 4; ++k) v[k] = R((pos & ~15u) + 4u * (uint32_t)k);
    if (a16w_failed(o)) return 0;
    if (em_vu_vec_bits(EM_VU_MULABC, 15, 0, rows[0], v, 0, NULL, acc) != EM_EE_FLOAT_OK ||
        em_vu_vec_bits(EM_VU_MADDABC, 15, 1, rows[1], v, 0, acc, acc) != EM_EE_FLOAT_OK ||
        em_vu_vec_bits(EM_VU_MADDABC, 15, 2, rows[2], v, 0, acc, acc) != EM_EE_FLOAT_OK ||
        em_vu_vec_bits(EM_VU_MADDBC, 15, 3, rows[3], vf0, 0, acc, clip) != EM_EE_FLOAT_OK) {
        a16w_latch(o, 0x001DAE10u, EM_AREA16_WATER_FAULT_UNMEASURED);
        return 0;
    }
    if (a16w_clipw(o, clip) & 0x3Fu) reject = 1;
    if (a16w_failed(o)) return 0;
    s = DIV(F(16.0f), R(p + 0xCu));
    if (LT(s, 0)) reject = 1;
    x = R(p);
    W(p, MUL(x, s));
    x = R(p + 4u);
    W(p + 4u, MUL(x, s));
    x = R(p + 8u);
    W(p + 8u, MUL(x, s));
    x = R(p);
    if (LT(x, 0) || !LE(x, F(65520.0f))) {
        reject = 1;
    } else {
        x = R(p + 4u);
        if (LT(x, 0) || !LE(x, F(65520.0f))) reject = 1;
    }
    SIB(a16w_001DAC90(o, dst, p, uv, shade));
    if (reject) W(dst + 0x2Cu, 0x8000);
    return 0;
}

/* ------------------------------------------------------------------------
 * 001DB480(sp): the grid's GS display list. The frame is sp - 0xE0: the
 * 001D6C90 stack arguments at +0x00..+0x30 (doublewords 1, 2, 0, 1, 0, 1,
 * 0), the vertex position at +0xB0, its (u, v, depth, 0) at +0xC0 and the
 * kick vector at +0xD0. The packet cursor ((0x275670)+0x1C) is kept as the
 * chain head; unit 3 is set up with the buffer 0x258000 (001D6B10,
 * 001D6BA0(3, .., 3, 0), 001D1FF0(3, 3), 001D6C90(3, 0, 1, 0, 0, 1, 0,
 * 0)); the matrix 001CD370(3) is loaded as vf24..vf27. For each of the 31
 * row pairs a packet at the cursor (byte +3 = 0x10, word +4 = 0, halfword
 * +0 = 0xC2, each through the cursor read again; the cursor moves by
 * 0xC30; the quadword +0x10 cleared, +0x1C = 0x500000C1, the GIF tag
 * 0x300E400000008040 / 0x421 at +0x20 / +0x28), then per column j two
 * strip vertices through 001DAE10 (0x30 bytes each from +0x30): rows i
 * and i + 1, position (origin.x + u, origin.y - 10 + v [+ 1.1612903 for
 * the second], origin.z + 0.1 h, 1.0), (cell u, cell v, 128 (1 - damping
 * word), 0) and the shade at 0x81C030; u steps by 1.0 from -16, v by
 * 1.1612903 per row. Then 001D1F20(3), 001D1FF0(3, 1), the closing tag
 * (byte +3 = 0x60, +4 = 0, +0 = 0, the cursor + 0x10), 00102948(kick,
 * origin), kick +8 = 455.0 and 001CAAC0(kick, the chain head).
 * ---------------------------------------------------------------------- */
uint32_t a16w_001DB480(A16W *o, uint32_t sp)
{
    const uint32_t frame = sp - 0xE0u, va = frame + 0xB0u, vb = frame + 0xC0u, kick = frame + 0xD0u;
    uint32_t ctx, head, m = 0, rows[4][4], v = 0, u, pkt, dl, f;
    ctx = R(CTX_PTR);
    head = R(ctx + 0x1Cu);
    C(001D6B10, 3, 0x258000, 8, 8);
    C(001D6BA0, 3, 0x258000, 8, 8, 3, 0);
    C(001D1FF0, 3, 3);
    a16w_w64(o, frame + 0x00u, 1);
    a16w_w64(o, frame + 0x08u, 2);
    a16w_w64(o, frame + 0x10u, 0);
    a16w_w64(o, frame + 0x18u, 1);
    a16w_w64(o, frame + 0x20u, 0);
    a16w_w64(o, frame + 0x28u, 1);
    a16w_w64(o, frame + 0x30u, 0);
    C(001D6C90, 3, 0, 1, 0, 0, 1, 0, 0);
    C(001CD370, 3, &m);
    for (uint32_t r = 0; r < 4; ++r)
        for (uint32_t k = 0; k < 4; ++k) rows[r][k] = R(((m + r * 0x10u) & ~15u) + 4u * k);
    if (a16w_failed(o)) return 0;
    for (uint32_t i = 0; i < 31; ++i) {
        ctx = R(CTX_PTR);
        WB(R(ctx + 0x1Cu) + 3u, 0x10);
        W(R(ctx + 0x1Cu) + 4u, 0);
        WH(R(ctx + 0x1Cu), 0xC2);
        pkt = R(ctx + 0x1Cu);
        W(ctx + 0x1Cu, pkt + 0xC30u);
        a16w_w64(o, (pkt + 0x10u) & ~15u, 0);
        a16w_w64(o, ((pkt + 0x10u) & ~15u) + 8u, 0);
        W(pkt + 0x1Cu, 0x500000C1u);
        a16w_w64(o, pkt + 0x20u, UINT64_C(0x300E400000008040));
        a16w_w64(o, pkt + 0x28u, 0x421);
        if (a16w_failed(o)) return 0;
        dl = pkt + 0x30u;
        u = F(-16.0f);
        for (uint32_t j = 0; j < 32; ++j) {
            for (uint32_t second = 0; second < 2; ++second) {
                const uint32_t c = GRID + (i + second) * ROW + j * 0x10u;
                W(va, ADD(R(BLOCK + 0x20u), u));
                f = ADD(SUB(R(BLOCK + 0x24u), F(10.0f)), v);
                if (second) f = ADD(0x3F94A529u /* 1.1612903 */, f);
                W(va + 4u, f);
                f = R(c + 0xCu);
                W(va + 8u, ADD(R(BLOCK + 0x28u), MUL(0x3DCCCCCDu /* 0.1 */, f)));
                W(va + 0xCu, 0x3F800000u);
                W(vb, R(c));
                W(vb + 4u, R(c + 4u));
                f = R(BLOCK + 0x00u);
                W(vb + 8u, MUL(F(128.0f), SUB(F(1.0f), f)));
                W(vb + 0xCu, 0);
                SIB(a16w_001DAE10(o, dl + 0x30u * second, va, vb, BLOCK + 0x30u, frame, rows));
            }
            dl += 0x60u;
            u = ADD(u, F(1.0f));
        }
        v = ADD(v, 0x3F94A529u);
    }
    C(001D1F20, 3);
    C(001D1FF0, 3, 1);
    ctx = R(CTX_PTR);
    WB(R(ctx + 0x1Cu) + 3u, 0x60);
    W(R(ctx + 0x1Cu) + 4u, 0);
    WH(R(ctx + 0x1Cu), 0);
    pkt = R(ctx + 0x1Cu);
    W(ctx + 0x1Cu, pkt + 0x10u);
    C(00102948, kick, BLOCK + 0x20u);
    W(kick + 8u, 0x43E38000u);
    C(001CAAC0, kick, head);
    return 0;
}

/* ---- the exported entries --------------------------------------------- */
#define ENTRY(body) \
    A16W o; \
    if (a16w_begin(&o, h, fault)) return -1; \
    (void)(body); \
    return a16w_end(&o)

int em_area16_water_001DAFA0(const EmArea16WaterHooks *h, EmArea16WaterFault *fault)
{ ENTRY(a16w_001DAFA0(&o)); }

int em_area16_water_001DB240(const EmArea16WaterHooks *h, int32_t a0, EmArea16WaterFault *fault)
{ ENTRY(a16w_001DB240(&o, a0)); }

int em_area16_water_001DB040(const EmArea16WaterHooks *h, uint32_t a0, EmArea16WaterFault *fault)
{ ENTRY(a16w_001DB040(&o, a0)); }

int em_area16_water_001DB250(const EmArea16WaterHooks *h, EmArea16WaterFault *fault)
{ ENTRY(a16w_001DB250(&o)); }

int em_area16_water_001DB480(const EmArea16WaterHooks *h, uint32_t sp, EmArea16WaterFault *fault)
{ ENTRY(a16w_001DB480(&o, sp)); }
