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
#include "driver/spi.h"
#include "esp8266/spi_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"

//--------------------------------------------
extern spi_dev_t SPI1;
void delay_ms(uint32_t time_ms);
static const char* TAG = "hal-spi-vs1003";

//--------------------------------------------
#define SPI_BIT_RATE_DIV  SPI_2MHz_DIV

//--------------------------------------------
#define	GPIO_RESET       15
#define	GPIO_XCS         5
#define	GPIO_XDCS        16
#define	GPIO_DREQ        4

#define OUTPUT_PINS_SEL  (1ULL << GPIO_RESET) | (1ULL << GPIO_XCS) | (1ULL << GPIO_XDCS)
#define INPUT_PINS_SEL   (1ULL << GPIO_DREQ)

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
    esp_err_t err;
    gpio_config_t io_conf = { 0 };
    spi_config_t spi_config = { 0 };

    // Output pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = OUTPUT_PINS_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    err = gpio_config(&io_conf);
    ESP_LOGD(TAG, "gpio_config for output pins: err = %d", err);

    // Input pins
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pin_bit_mask = INPUT_PINS_SEL;
    err = gpio_config(&io_conf);
    ESP_LOGD(TAG, "gpio_config for input pins: err = %d", err);

    // Set SPI to master mode
    spi_config.mode = SPI_MASTER_MODE;
    // Interface parameters
    spi_config.interface.miso_en = 1;
    spi_config.interface.mosi_en = 1;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_BIT_RATE_DIV;
    err = spi_init(HSPI_HOST, &spi_config);

    ESP_LOGD(TAG, "spi_init: err = %d", err);

    // Enable full duplex mode for HSPI
    SPI1.user.duplex = 1;
}

//--------------------------------------------
uint8_t hal_spi_vs1003_txrx(uint8_t data)
{
    uint32_t tmp = (uint32_t)data;

    // Waiting for an incomplete transfer
    while (SPI1.cmd.usr);

    SPI1.user.usr_command = 0;
    SPI1.user.usr_addr = 0;
    SPI1.user.usr_mosi = 1;
    SPI1.user.usr_miso = 0;
    SPI1.user1.usr_mosi_bitlen = 8 - 1;
    SPI1.data_buf[0] = tmp;

    // Start send data
    SPI1.cmd.usr = 1;
    // Waiting for transfer
    while (SPI1.cmd.usr);

    tmp = SPI1.data_buf[0];
    return (uint8_t)tmp;
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
