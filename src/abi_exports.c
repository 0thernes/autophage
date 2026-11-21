#include "autophage_internal.h"

/* ============================================================================
 * PUBLIC C11 ABI METADATA & HARDWARE PROBE EXPORTS
 * (All other AUTOPHAGE_API functions are defined in soup_engine.c & metrology_ovbs.c)
 * ============================================================================ */

uint32_t autophage_abi_version(void) {
    return AUTOPHAGE_ABI_VERSION;
}

void autophage_probe_hardware(AutophageHwCaps* out_caps) {
    if (!out_caps) return;
    hw_detect_caps(out_caps);
}
