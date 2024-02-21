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

#ifndef HAL_SPI_VS1003_H_
#define HAL_SPI_VS1003_H_

//--------------------------------------------
void hal_spi_vs1003_reset(void);
void hal_spi_vs1003_init(void);
uint8_t hal_spi_vs1003_txrx(uint8_t data);
void hal_spi_vs1003_xcs(uint8_t state);
void hal_spi_vs1003_xdcs(uint8_t state);
uint8_t hal_spi_vs1003_dreq(void);

#endif // HAL_SPI_VS1003_H_
