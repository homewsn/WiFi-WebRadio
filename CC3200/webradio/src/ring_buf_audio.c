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

#include <stdint.h>     /* uint8_t ... uint64_t */
#include <stdlib.h>     /* size_t */
#include "osi.h"
#include "ring_buf.h"

//--------------------------------------------
static ring_buf_t ring_buf;
static uint8_t audio_buf[4096];

//--------------------------------------------
int ring_buf_audio_init(void)
{
	return ring_buf_init(&ring_buf, audio_buf, sizeof(audio_buf));
}

//--------------------------------------------
int ring_buf_audio_clear(void)
{
	return ring_buf_clear(&ring_buf);
}

//--------------------------------------------
int ring_buf_audio_put(uint8_t *buf, size_t size)
{
	return ring_buf_put(&ring_buf, buf, size);
}

//--------------------------------------------
int ring_buf_audio_get(uint8_t *buf, size_t size)
{
	return ring_buf_get(&ring_buf, buf, size);
}

//--------------------------------------------
int ring_buf_audio_get_percentage_fill(void)
{
	return ring_buf_get_percentage_fill(&ring_buf);
}

//--------------------------------------------
int ring_buf_audio_destroy(void)
{
	return ring_buf_destroy(&ring_buf);
}
