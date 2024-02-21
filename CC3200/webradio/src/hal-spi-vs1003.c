/*
* Copyright (c) 2023 Vladimir Alemasov
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
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "pin.h"
#include "spi.h"
#include "utils.h"
#include "rom_map.h"
#include "gpio.h"
#include "prcm.h"

//--------------------------------------------
#define	PIN_SCK           PIN_05                // SCK  --> SCLK
#define	PIN_MISO          PIN_06                // MISO <-- MISO
#define	PIN_MOSI          PIN_07                // MOSI --> MOSI

//--------------------------------------------
#define	PIN_RESET         PIN_04                // GPIO13 --> XRST
#define	GPIO_PORT_RESET   GPIOA1_BASE           // 13 / 8 = 1 -> A1
#define	GPIO_BIT_RESET    5                     // 13 % 8 = 5
#define	GPIO_PIN_RESET    1 << GPIO_BIT_RESET   // 1 << 5 = 0x20
#define	PIN_XCS           PIN_08                // GPIO17 --> XCS
#define	GPIO_PORT_XCS     GPIOA2_BASE           // 17 / 8 = 2 -> A2
#define	GPIO_BIT_XCS      1                     // 17 % 8 = 1
#define	GPIO_PIN_XCS      1 << GPIO_BIT_XCS     // 1 << 1 = 0x02
#define	PIN_XDCS          PIN_15                // GPIO22 --> XDCS
#define	GPIO_PORT_XDCS    GPIOA2_BASE           // 22 / 8 = 2 -> A2
#define	GPIO_BIT_XDCS     6                     // 22 % 8 = 6
#define	GPIO_PIN_XDCS     1 << GPIO_BIT_XDCS    // 1 << 6 = 0x40
#define	PIN_DREQ          PIN_18                // GPIO28 <-- DREQ
#define	GPIO_PORT_DREQ    GPIOA3_BASE           // 28 / 8 = 3 -> A3
#define	GPIO_BIT_DREQ     4                     // 28 % 8 = 4
#define	GPIO_PIN_DREQ     1 << GPIO_BIT_DREQ    // 1 << 4 = 0x10

//--------------------------------------------
#define SPI_BIT_RATE  1000000

//--------------------------------------------
void hal_spi_vs1003_reset(void)
{
	MAP_GPIOPinWrite(GPIO_PORT_RESET, GPIO_PIN_RESET, 1 << GPIO_BIT_RESET);
	MAP_UtilsDelay(10000);
	MAP_GPIOPinWrite(GPIO_PORT_RESET, GPIO_PIN_RESET, 0 << GPIO_BIT_RESET);
	MAP_UtilsDelay(10000);
	MAP_GPIOPinWrite(GPIO_PORT_XCS, GPIO_PIN_XCS, 1 << GPIO_BIT_XCS);
	MAP_GPIOPinWrite(GPIO_PORT_XDCS, GPIO_PIN_XDCS, 1 << GPIO_BIT_XDCS);
	MAP_GPIOPinWrite(GPIO_PORT_RESET, GPIO_PIN_RESET, 1 << GPIO_BIT_RESET);
	MAP_UtilsDelay(20000);
	while (!MAP_GPIOPinRead(GPIO_PORT_DREQ, GPIO_PIN_DREQ) >> GPIO_BIT_DREQ);
}

//--------------------------------------------
void hal_spi_vs1003_init(void)
{
	// enable peripheral clocks
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);

	// configure GPIO pins
	MAP_PinTypeGPIO(PIN_RESET, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIO_PORT_RESET, GPIO_PIN_RESET, GPIO_DIR_MODE_OUT);
	MAP_PinTypeGPIO(PIN_XCS, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIO_PORT_XCS, GPIO_PIN_XCS, GPIO_DIR_MODE_OUT);
	MAP_PinTypeGPIO(PIN_XDCS, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIO_PORT_XDCS, GPIO_PIN_XDCS, GPIO_DIR_MODE_OUT);
	MAP_PinTypeGPIO(PIN_DREQ, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIO_PORT_DREQ, GPIO_PIN_DREQ, GPIO_DIR_MODE_IN);

	// configure SPI pins
	MAP_PinTypeSPI(PIN_SCK, PIN_MODE_7);
	MAP_PinTypeSPI(PIN_MISO, PIN_MODE_7);
    MAP_PinTypeSPI(PIN_MOSI, PIN_MODE_7);

	// reset SPI
	MAP_SPIReset(GSPI_BASE);

	// configure SPI
	MAP_SPIConfigSetExpClk(GSPI_BASE,
	                       MAP_PRCMPeripheralClockGet(PRCM_GSPI),
	                       SPI_BIT_RATE,
	                       SPI_MODE_MASTER,
	                       SPI_SUB_MODE_0,
	                       SPI_3PIN_MODE | SPI_TURBO_OFF | SPI_CS_ACTIVEHIGH | SPI_WL_8);

	// enable SPI
	MAP_SPIEnable(GSPI_BASE);
}

//--------------------------------------------
uint8_t hal_spi_vs1003_txrx(uint8_t data)
{
	unsigned long tmp;

	MAP_SPIDataPut(GSPI_BASE, (unsigned long)data);
	MAP_SPIDataGet(GSPI_BASE, &tmp);
	return (uint8_t)tmp;
}

//--------------------------------------------
void hal_spi_vs1003_xcs(uint8_t state)
{
	MAP_GPIOPinWrite(GPIO_PORT_XCS, GPIO_PIN_XCS, state << GPIO_BIT_XCS);
}

//--------------------------------------------
void hal_spi_vs1003_xdcs(uint8_t state)
{
	MAP_GPIOPinWrite(GPIO_PORT_XDCS, GPIO_PIN_XDCS, state << GPIO_BIT_XDCS);
}

//--------------------------------------------
uint8_t hal_spi_vs1003_dreq(void)
{
	return MAP_GPIOPinRead(GPIO_PORT_DREQ, GPIO_PIN_DREQ) >> GPIO_BIT_DREQ;
}
