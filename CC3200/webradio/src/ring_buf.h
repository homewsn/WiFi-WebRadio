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

#ifndef RING_BUF_H
#define RING_BUF_H

//--------------------------------------------
typedef struct
{
	uint8_t *buffer;
	size_t length;
	size_t count;
	size_t head;
	size_t tail;
	size_t max_count;
	OsiLockObj_t mutex;
	int valid;
} ring_buf_t;

//--------------------------------------------
int ring_buf_init(ring_buf_t *ring_buf, uint8_t *buf, size_t size);
int ring_buf_clear(ring_buf_t *ring_buf);
int ring_buf_put(ring_buf_t *ring_buf, uint8_t *buf, size_t size);
int ring_buf_get(ring_buf_t *ring_buf, uint8_t *buf, size_t size);
int ring_buf_get_percentage_fill(ring_buf_t *ring_buf);
int ring_buf_destroy(ring_buf_t *ring_buf);

#endif /* RING_BUF_H */