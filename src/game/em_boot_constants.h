/* The boot data every recording of an area holds unchanged from the ELF
 * (tools/export_boot_constants.py): read-only, the fallback behind the
 * exported windows for any owner. A read outside it faults. */
#ifndef EM_BOOT_CONSTANTS_H
#define EM_BOOT_CONSTANTS_H

#include <stdint.h>

#define EM_BOOT_CONSTANTS_PATH "assets/boot_constants.embc"

/* Select the area of D_00810700. Every area reads the one export: its boot
 * tables are the same data anywhere (the gait table D_00248670 a ledge reads
 * in AREA00, not in a window of its own). Returns 0, or -1 when the file is
 * missing. */
int em_boot_constants_select(unsigned area);
/* The bytes of [address, address + size), or NULL. */
const uint8_t *em_boot_constants(uint32_t address, uint32_t size);

#endif
