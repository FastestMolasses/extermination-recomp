/* Field shorthands of the em_area00s2_creature*.c translations (included by
 * them only, after em_area00s2_creature_internal.h). */
#ifndef EM_AREA00S2_CREATURE_LOCAL_H
#define EM_AREA00S2_CREATURE_LOCAL_H

#include "em_area00s2_creature_internal.h"

#define B(a) a0s2_u8(o, (a))
#define SB(a) a0s2_s8(o, (a))
#define SH(a) a0s2_s16(o, (a))
#define R(a) a0s2_u32(o, (a))
#define W(a, v) a0s2_w32(o, (a), (v))
#define WB(a, v) a0s2_w8(o, (a), (v))
#define WH(a, v) a0s2_w16(o, (a), (v))
#define F(v) a0s2_bits(v)
/* A hook call; a failed or missing hook ends the translation. */
#define C(fn, ...) do { if (a0s2_c_##fn(o, __VA_ARGS__)) return 0; } while (0)
/* A call of another translation; its fault ends this one too. */
#define SIB(call) do { (void)(call); if (a0s2_failed(o)) return 0; } while (0)
#define ADD(a, b) A0S2_ADD((a), (b))
#define SUB(a, b) A0S2_SUB((a), (b))
#define MUL(a, b) A0S2_MUL((a), (b))
#define DIV(a, b) A0S2_DIV((a), (b))
#define LT(a, b) A0S2_LT((a), (b))
#define LE(a, b) A0S2_LE((a), (b))
#define EQ(a, b) A0S2_EQ((a), (b))

/* The vector 0x810360 (x at +0, z at +8: the player record's position in the
 * other creature modules) and the vector 0x810320 the reaction fills. */
#define PLAYER_POS 0x810360u
#define AIM 0x810320u
/* The byte 0x810803: 1 stops the behaviour for the frame; the scripted
 * state writes 0x80 and waits for 0x81, 0014E2C0 waits for 2. */
#define FLAG803 0x810803u
/* The scratchpad vectors the creature works in. */
#define V36A0 0x700036A0u
#define V38A0 0x700038A0u
#define V38B0 0x700038B0u

#endif
