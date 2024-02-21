/*
* Copyright (c) 2023, 2024 Vladimir Alemasov
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

#include <stdint.h>
#include <unistd.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/display/Display.h>
#include "ti_drivers_config.h"

//--------------------------------------------
#define SPI_BIT_RATE  1000000

//--------------------------------------------
static SPI_Handle spiHandle;

//--------------------------------------------
void hal_spi_vs1003_reset(void)
{
	GPIO_write(CONFIG_GPIO_VS1003_RESET, 1);
	usleep(1000);
	GPIO_write(CONFIG_GPIO_VS1003_RESET, 0);
	usleep(1000);
	GPIO_write(CONFIG_GPIO_VS1003_XCS, 1);
	GPIO_write(CONFIG_GPIO_VS1003_XDCS, 1);
	GPIO_write(CONFIG_GPIO_VS1003_RESET, 1);
	usleep(1000);
	while (!GPIO_read(CONFIG_GPIO_VS1003_DREQ));
}

//--------------------------------------------
void hal_spi_vs1003_init(void)
{
	// it is OK to call GPIO_init() and SPI_init() multiple times
	GPIO_init();
	SPI_init();

	SPI_Params spiParams;
	SPI_Params_init(&spiParams);
	spiParams.bitRate = SPI_BIT_RATE;
	spiHandle = SPI_open(CONFIG_SPI_0, &spiParams);
}

//--------------------------------------------
uint8_t hal_spi_vs1003_txrx(uint8_t data)
{
	SPI_Transaction transaction;
	uint8_t tx_buf[1];
	uint8_t rx_buf[1];
	volatile bool res;

	tx_buf[0] = data;
	transaction.count = 1;
	transaction.txBuf = (void *)tx_buf;
	transaction.rxBuf = (void *)rx_buf;

	res = SPI_transfer(spiHandle, &transaction);

	return rx_buf[0];
}

//--------------------------------------------
void hal_spi_vs1003_xcs(uint8_t state)
{
	GPIO_write(CONFIG_GPIO_VS1003_XCS, state);
}

//--------------------------------------------
void hal_spi_vs1003_xdcs(uint8_t state)
{
	GPIO_write(CONFIG_GPIO_VS1003_XDCS, state);
}

//--------------------------------------------
uint8_t hal_spi_vs1003_dreq(void)
{
	return GPIO_read(CONFIG_GPIO_VS1003_DREQ);
}
