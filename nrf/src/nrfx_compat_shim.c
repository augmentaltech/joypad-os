// nrfx_compat_shim.c - nRF52840 DK bring-up shim
//
// The vendored joypad TinyUSB fork (src/lib/tinyusb) is built against an older
// nRF Connect SDK (v3.1.0) whose nrfx HAL provided nrf_clock_hf_is_running().
// NCS v3.3.0's nrfx removed that symbol, so TinyUSB's nRF device-controller
// driver (dcd_nrf5x.c) links with an undefined reference. Reimplement it here
// by reading HFCLKSTAT directly — the DCD only needs to know whether the
// high-frequency crystal is running before it brings up USB.
//
// Only compiled into the nrf52840dk build (see nrf/CMakeLists.txt); the shipping
// boards build against the older SDK where the real symbol exists, so adding it
// there would be a duplicate definition.

#include <nrf.h>
#include <stdbool.h>

bool nrf_clock_hf_is_running(const void *p_reg, int clk_src)
{
    (void)clk_src;
    const NRF_CLOCK_Type *clk = (const NRF_CLOCK_Type *)p_reg;
    return (clk->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) != 0;
}
