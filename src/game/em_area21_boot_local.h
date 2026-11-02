/* Field shorthands of the em_area21_boot*.c translations (included by them
 * only, after em_area21_boot_internal.h). */
#ifndef EM_AREA21_BOOT_LOCAL_H
#define EM_AREA21_BOOT_LOCAL_H

#include "em_area21_boot_internal.h"

#define B(a) a21b_u8(o, (a))
#define SB(a) a21b_s8(o, (a))
#define SH(a) a21b_s16(o, (a))
#define R(a) a21b_u32(o, (a))
#define W(a, v) a21b_w32(o, (a), (v))
#define WB(a, v) a21b_w8(o, (a), (v))
#define WH(a, v) a21b_w16(o, (a), (v))
#define F(v) a21b_bits(v)
/* A hook call; a failed or missing hook ends the translation. */
#define C(fn, ...) do { if (a21b_c_##fn(o, __VA_ARGS__)) return 0; } while (0)
/* A call of another translation; its fault ends this one too. */
#define SIB(call) do { (void)(call); if (a21b_failed(o)) return 0; } while (0)
/* The node's +0x4C method; its failure ends the translation. */
#define METHOD(self) do { if (a21b_method(o, (self))) return 0; } while (0)
#define ADD(a, b) A21B_ADD((a), (b))
#define SUB(a, b) A21B_SUB((a), (b))
#define MUL(a, b) A21B_MUL((a), (b))
#define DIV(a, b) A21B_DIV((a), (b))
#define LT(a, b) A21B_LT((a), (b))
#define LE(a, b) A21B_LE((a), (b))
#define EQ(a, b) A21B_EQ((a), (b))
#define CVT(w) A21B_CVT_S_W(w)

/* The scratchpad vectors the creatures work in. */
#define V36A0 0x700036A0u
#define V38A0 0x700038A0u
#define V38B0 0x700038B0u
#define PLAYER_POS 0x810360u
/* The hard-mode flag (0x81070A). */
#define HARD 0x81070Au

#endif
