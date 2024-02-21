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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "hal-spi-vs1003.h"
#include "vs1053-regs.h"
#include "vs1053.h"
#include "vs1053-plugin.h"

//--------------------------------------------
void delay_ms(uint32_t time_ms);

//--------------------------------------------
void vs1053_init_iface(void)
{
	hal_spi_vs1003_init();
	vs1053_reset();
}

//--------------------------------------------
void vs1053_reset(void)
{
	hal_spi_vs1003_reset();
	vs1053_write_register(VS1053_CLOCKF, VS1053_CLOCKF_MULT_XTALIx30 | VS1053_CLOCKF_ADDx10);
	delay_ms(1);
	vs1053_load_user_code();
}

//--------------------------------------------
uint16_t vs1053_read_register(uint8_t reg)
{
	uint16_t data;

	hal_spi_vs1003_xcs(0);
	hal_spi_vs1003_txrx(VS1053_CMD_READ);
	hal_spi_vs1003_txrx(reg);
	data = hal_spi_vs1003_txrx(0) << 8;
	data |= hal_spi_vs1003_txrx(0);
	hal_spi_vs1003_xcs(1);
	return data;
}

//--------------------------------------------
void vs1053_write_register(uint8_t reg, uint16_t data)
{
	hal_spi_vs1003_xcs(0);
	hal_spi_vs1003_txrx(VS1053_CMD_WRITE);
	hal_spi_vs1003_txrx(reg);
	hal_spi_vs1003_txrx(data >> 8);
	hal_spi_vs1003_txrx(data);
	hal_spi_vs1003_xcs(1);
}

//--------------------------------------------
void vs1053_write_data(uint8_t *buf, size_t size)
{
	size_t cnt;

	hal_spi_vs1003_xdcs(0);
	for (cnt = 0; cnt < size; cnt++)
	{
		while (!hal_spi_vs1003_dreq());
		hal_spi_vs1003_txrx(*buf++);
	}
	hal_spi_vs1003_xdcs(1);
}

//--------------------------------------------
void vs1053_sinewave_test(uint32_t time_ms)
{
    uint8_t sine_on[]  = {0x53, 0xEF, 0x6E, 0x44, 0x00, 0x00, 0x00, 0x00};
    uint8_t sine_off[] = {0x45, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00};
	size_t cnt;

	hal_spi_vs1003_reset();
	vs1053_write_register(VS1053_MODE, VS1053_MODE_SDINEW | VS1053_MODE_TESTS);
	while (!hal_spi_vs1003_dreq());
	for (cnt = 0; cnt < sizeof(sine_on); cnt++)
	{
		hal_spi_vs1003_xdcs(0);
		hal_spi_vs1003_txrx(sine_on[cnt]);
		hal_spi_vs1003_xdcs(1);
	}
	delay_ms(time_ms);
	for (cnt = 0; cnt < sizeof(sine_off); cnt++)
	{
		hal_spi_vs1003_xdcs(0);
		hal_spi_vs1003_txrx(sine_off[cnt]);
		hal_spi_vs1003_xdcs(1);
	}
	hal_spi_vs1003_reset();
	vs1053_write_register(VS1053_MODE, VS1053_MODE_SDINEW);
	vs1053_write_register(VS1053_CLOCKF, VS1053_CLOCKF_MULT_XTALIx30 | VS1053_CLOCKF_ADDx10);
}

//--------------------------------------------
void vs1053_load_user_code(void)
{
	size_t cnt = 0;

	while (cnt < sizeof(vs1053_plugin) / sizeof(uint16_t))
	{
		unsigned short addr, n, val;
		addr = vs1053_plugin[cnt++];
		n = vs1053_plugin[cnt++];
		if (n & 0x8000U)
		{
			// RLE run, replicate n samples
			n &= 0x7FFF;
			val = vs1053_plugin[cnt++];
			while (n--)
			{
				vs1053_write_register(addr, val);
			}
		}
		else
		{
			// Copy run, copy n samples
			while (n--)
			{
				val = vs1053_plugin[cnt++];
				vs1053_write_register(addr, val);
			}
		}
	}
}
