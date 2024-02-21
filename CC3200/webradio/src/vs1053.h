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

#ifndef VS1053_H_
#define VS1053_H_

//--------------------------------------------
void vs1053_init_iface(void);
void vs1053_reset(void);
uint16_t vs1053_read_register(uint8_t reg);
void vs1053_write_register(uint8_t reg, uint16_t data);
void vs1053_write_data(uint8_t *buf, size_t size);
void vs1053_sinewave_test(uint32_t time_ms);
void vs1053_load_user_code(void);

#endif // VS1053_H_
