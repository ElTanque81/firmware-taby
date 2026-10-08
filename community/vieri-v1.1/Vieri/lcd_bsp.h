#pragma once
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialises QSPI bus + SH8601-compatible panel (Waveshare AMOLED 1.64 V1.1).
// on_done is called (from ISR) after every pixel DMA transfer finishes.
esp_lcd_panel_handle_t lcd_bsp_init(esp_lcd_panel_io_color_trans_done_cb_t on_done, int max_transfer_bytes);

#ifdef __cplusplus
}
#endif
