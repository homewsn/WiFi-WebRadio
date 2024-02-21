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
#include "pthread.h"
#include "ring_buf.h"

//--------------------------------------------
int ring_buf_init(ring_buf_t *ring_buf, uint8_t *buf, size_t size)
{
	if (ring_buf->valid)
	{
		return -1;
	}
	if (pthread_mutex_init(&ring_buf->mutex, NULL) < 0)
	{
		return -1;
	}
	ring_buf->valid = 1;
	ring_buf->buffer = buf;
	ring_buf->length = size;
	ring_buf->count = 0;
	ring_buf->head = size - 1;
	ring_buf->tail = 0;
	ring_buf->max_count = 0;
	return 0;
}

//--------------------------------------------
int ring_buf_clear(ring_buf_t *ring_buf)
{
	if (!ring_buf->valid)
	{
		return -1;
	}
	pthread_mutex_lock(&ring_buf->mutex);
	ring_buf->count = 0;
	ring_buf->head = ring_buf->length - 1;
	ring_buf->tail = 0;
	ring_buf->max_count = 0;
	pthread_mutex_unlock(&ring_buf->mutex);
	return 0;
}

//--------------------------------------------
int ring_buf_put(ring_buf_t *ring_buf, uint8_t *buf, size_t size)
{
	size_t next;
	size_t added = 0;

	if (!ring_buf->valid)
	{
		return -1;
	}
	pthread_mutex_lock(&ring_buf->mutex);

	size = size > ring_buf->length - ring_buf->count ? ring_buf->length - ring_buf->count : size;
	while (size)
	{
		next = (ring_buf->head + 1) % ring_buf->length;
		ring_buf->buffer[next] = *buf++;
		ring_buf->head = next;
		++ring_buf->count;
		--size;
		++added;
	}
	if (ring_buf->max_count < ring_buf->count)
	{
		ring_buf->max_count = ring_buf->count;
	}

	pthread_mutex_unlock(&ring_buf->mutex);

	return added;
}

//--------------------------------------------
int ring_buf_get(ring_buf_t *ring_buf, uint8_t *buf, size_t size)
{
	size_t removed = 0;

	if (!ring_buf->valid)
	{
		return -1;
	}
	pthread_mutex_lock(&ring_buf->mutex);

	size = size > ring_buf->count ? ring_buf->count : size;
	while (size)
	{
		*buf++ = ring_buf->buffer[ring_buf->tail++];
		ring_buf->tail %= ring_buf->length;
		--ring_buf->count;
		--size;
		++removed;
	}

	pthread_mutex_unlock(&ring_buf->mutex);

	return removed;
}

//--------------------------------------------
int ring_buf_get_percentage_fill(ring_buf_t *ring_buf)
{
	size_t percentage_fill = 0;

	if (!ring_buf->valid)
	{
		return -1;
	}
	pthread_mutex_lock(&ring_buf->mutex);

	percentage_fill = ring_buf->count * 100 / ring_buf->length;

	pthread_mutex_unlock(&ring_buf->mutex);

	return percentage_fill;
}

//--------------------------------------------
int ring_buf_destroy(ring_buf_t *ring_buf)
{
	if (!ring_buf->valid)
	{
		return -1;
	}
	pthread_mutex_destroy(&ring_buf->mutex);
	ring_buf->valid = 0;
	return 0;
}
