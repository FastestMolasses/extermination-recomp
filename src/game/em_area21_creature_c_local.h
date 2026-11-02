/* Field shorthands of the em_area21_creature_c*.c translations (included by
 * them only, after em_area21_creature_c_internal.h). */
#ifndef EM_AREA21_CREATURE_C_LOCAL_H
#define EM_AREA21_CREATURE_C_LOCAL_H

#include "em_area21_creature_c_internal.h"

#define B(a) a21c_u8(o, (a))
#define SB(a) a21c_s8(o, (a))
#define SH(a) a21c_s16(o, (a))
#define R(a) a21c_u32(o, (a))
#define W(a, v) a21c_w32(o, (a), (v))
#define WB(a, v) a21c_w8(o, (a), (v))
#define WH(a, v) a21c_w16(o, (a), (v))
#define F(v) a21c_bits(v)
/* A hook call; a failed or missing hook ends the translation. */
#define C(fn, ...) do { if (a21c_c_##fn(o, __VA_ARGS__)) return 0; } while (0)
/* A hook call with no arguments. */
#define C0(fn, out) do { if (a21c_c_##fn(o, (out))) return 0; } while (0)
/* A call of another translation; its fault ends this one too. */
#define SIB(call) do { (void)(call); if (a21c_failed(o)) return 0; } while (0)
#define ADD(a, b) A21C_ADD((a), (b))
#define SUB(a, b) A21C_SUB((a), (b))
#define MUL(a, b) A21C_MUL((a), (b))
#define DIV(a, b) A21C_DIV((a), (b))
#define LT(a, b) A21C_LT((a), (b))
#define LE(a, b) A21C_LE((a), (b))
#define EQ(a, b) A21C_EQ((a), (b))
#define CVT(w) A21C_CVT_S_W(w)

/* The scratchpad vectors and words the creature works in. */
#define V36A0 0x700036A0u
#define V38A0 0x700038A0u
#define V38B0 0x700038B0u
#define SPAD_ANGLE 0x70003A20u
#define PLAYER_POS 0x810360u
/* The bone table pointer (gp-relative) and the 255 of the random bytes. */
#define BONE_TABLE 0x275B40u
#define F255 0x437F0000u

/* A random byte scaled into [0, 1]: ((r >> shift) & 0xFF) / 255. */
static inline uint32_t a21c_unit(int32_t r, unsigned shift)
{
    return A21C_DIV(A21C_CVT_S_W((uint32_t)(a21c_sra((uint32_t)r, shift) & 0xFF)), F255);
}

#endif
