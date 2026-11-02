/* Field shorthands of the em_area17_creature*.c translations (included by
 * them only, after em_area17_creature_internal.h). */
#ifndef EM_AREA17_CREATURE_LOCAL_H
#define EM_AREA17_CREATURE_LOCAL_H

#include "em_area17_creature_internal.h"

#define B(a) a17c_u8(o, (a))
#define SB(a) a17c_s8(o, (a))
#define SH(a) a17c_s16(o, (a))
#define R(a) a17c_u32(o, (a))
#define W(a, v) a17c_w32(o, (a), (v))
#define WB(a, v) a17c_w8(o, (a), (v))
#define WH(a, v) a17c_w16(o, (a), (v))
#define F(v) a17c_bits(v)
/* A hook call; a failed or missing hook ends the translation. */
#define C(fn, ...) do { if (a17c_c_##fn(o, __VA_ARGS__)) return 0; } while (0)
/* A call of another translation; its fault ends this one too. */
#define SIB(call) do { (void)(call); if (a17c_failed(o)) return 0; } while (0)
#define ADD(a, b) A17C_ADD((a), (b))
#define SUB(a, b) A17C_SUB((a), (b))
#define MUL(a, b) A17C_MUL((a), (b))
#define DIV(a, b) A17C_DIV((a), (b))
#define LT(a, b) A17C_LT((a), (b))
#define LE(a, b) A17C_LE((a), (b))
#define EQ(a, b) A17C_EQ((a), (b))

/* The scratchpad vectors the creature works in. */
#define V36A0 0x700036A0u
#define V38A0 0x700038A0u
#define V38B0 0x700038B0u
#define PLAYER_POS 0x810360u

#endif
