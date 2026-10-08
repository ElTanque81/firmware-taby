#include "lcd_bsp.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_sh8601.h"

#define LCD_HOST SPI2_HOST
#define PIN_CS 46
#define PIN_CLK 10
#define PIN_D0 11
#define PIN_D1 12
#define PIN_D2 13
#define PIN_D3 14
#define PIN_RST 21

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
  {0x11, (uint8_t[]){0x00}, 0, 80},
  {0xC4, (uint8_t[]){0x80}, 1, 0},
  {0x35, (uint8_t[]){0x00}, 1, 0},
  {0x53, (uint8_t[]){0x20}, 1, 1},
  {0x63, (uint8_t[]){0xFF}, 1, 1},
  {0x51, (uint8_t[]){0x00}, 1, 1},
  {0x29, (uint8_t[]){0x00}, 0, 10},
  {0x51, (uint8_t[]){0xFF}, 1, 0},
};

esp_lcd_panel_handle_t lcd_bsp_init(esp_lcd_panel_io_color_trans_done_cb_t on_done, int max_transfer_bytes)
{
  const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(PIN_CLK, PIN_D0, PIN_D1, PIN_D2, PIN_D3, max_transfer_bytes);
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  esp_lcd_panel_io_handle_t io = NULL;
  const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(PIN_CS, on_done, NULL);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io));

  sh8601_vendor_config_t vendor_config = {
    .init_cmds = lcd_init_cmds,
    .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
    .flags = { .use_qspi_interface = 1 },
  };
  esp_lcd_panel_handle_t panel = NULL;
  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    .vendor_config = &vendor_config,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io, &panel_config, &panel));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
  return panel;
}
