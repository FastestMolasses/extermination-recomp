/* em_credits_live.h - the ending's credits in the game (001AD740 states 0
 * and 1): em_credits_original over the exported rows
 * (tools/export_credits.py, assets/credits/credits.emcr), the message
 * service's glyph runs 001CC170 / 001CC1E0 (em_message_live, drawn at this
 * frame's step-F render) and the render context's 00207F80
 * (em_status_gs_page). The state D_00822CD0 is held here. */
#ifndef EM_CREDITS_LIVE_H
#define EM_CREDITS_LIVE_H

#include <stdint.h>

#define EM_CREDITS_EXPORT "assets/credits/credits.emcr"

/* 0022D360. 0, or -1 when the export is missing. */
int em_credits_live_0022D360(void);
/* 0022D380: *result its return. 0, or -1 (fault, printed). */
int em_credits_live_0022D380(int32_t *result);
/* The state bytes D_00822CD0 (for traces). */
const uint8_t *em_credits_live_state(void);

#endif
