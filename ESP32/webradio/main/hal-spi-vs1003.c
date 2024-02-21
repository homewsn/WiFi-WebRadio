/*
* Copyright (c) 2024 Vladimir Alemasov
* All rights reserved
*
* This program and the accompanying materials are distributed under 
* the terms of GNU General Public License version 2 
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

//--------------------------------------------
void delay_ms(uint32_t time_ms);
static const char* TAG = "hal-spi-vs1003";

//--------------------------------------------
#define SPI_HOST_ID      SPI3_HOST
#define SPI_CLK_2MHz     (2*1000*1000)

//--------------------------------------------
#define	GPIO_CLK         18
#define	GPIO_MISO        19
#define	GPIO_MOSI        23
#define	GPIO_RESET       22
#define	GPIO_XCS         5
#define	GPIO_XDCS        16
#define	GPIO_DREQ        4

#define	GPIO_NOT_USED    (-1)
#define OUTPUT_PINS_SEL  (1ULL << GPIO_RESET) | (1ULL << GPIO_XCS) | (1ULL << GPIO_XDCS)
#define INPUT_PINS_SEL   (1ULL << GPIO_DREQ)

//--------------------------------------------
static spi_device_handle_t spi;

//--------------------------------------------
void hal_spi_vs1003_reset(void)
{
	delay_ms(1);
    gpio_set_level(GPIO_RESET, 1);
	delay_ms(1);
    gpio_set_level(GPIO_RESET, 0);
	delay_ms(1);
    gpio_set_level(GPIO_XCS, 1);
    gpio_set_level(GPIO_XDCS, 1);
    gpio_set_level(GPIO_RESET, 1);
	delay_ms(1);
	while (!gpio_get_level(GPIO_DREQ));
    ESP_LOGD(TAG, "reset ok");
}

//--------------------------------------------
void hal_spi_vs1003_init(void)
{
    gpio_config_t io_conf = { 0 };
    spi_bus_config_t buscfg= { 0 };
    spi_device_interface_config_t dev_cfg = { 0 };

    buscfg.sclk_io_num = GPIO_CLK;
    buscfg.miso_io_num = GPIO_MISO;
    buscfg.mosi_io_num = GPIO_MOSI;
    buscfg.quadwp_io_num = GPIO_NOT_USED;
    buscfg.quadhd_io_num = GPIO_NOT_USED;
    buscfg.data4_io_num = GPIO_NOT_USED;
    buscfg.data5_io_num = GPIO_NOT_USED;
    buscfg.data6_io_num = GPIO_NOT_USED;
    buscfg.data7_io_num = GPIO_NOT_USED;
    buscfg.max_transfer_sz = 0;
    //Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_ID, &buscfg, SPI_DMA_DISABLED));

    // Output pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = OUTPUT_PINS_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Input pins
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pin_bit_mask = INPUT_PINS_SEL;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Interface parameters
    dev_cfg.clock_speed_hz = SPI_CLK_2MHz;
    dev_cfg.spics_io_num = GPIO_NOT_USED;
    dev_cfg.queue_size = 1;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi));
}

//--------------------------------------------
uint8_t hal_spi_vs1003_txrx(uint8_t data)
{
    spi_transaction_t t = { 0 };

    t.length = 8;
    t.tx_data[0] = data;
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;

    // Start send data
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));

    return t.rx_data[0];
}

//--------------------------------------------
void hal_spi_vs1003_xcs(uint8_t state)
{
    gpio_set_level(GPIO_XCS, state);
}

//--------------------------------------------
void hal_spi_vs1003_xdcs(uint8_t state)
{
    gpio_set_level(GPIO_XDCS, state);
}

//--------------------------------------------
uint8_t hal_spi_vs1003_dreq(void)
{
	return gpio_get_level(GPIO_DREQ);
}
