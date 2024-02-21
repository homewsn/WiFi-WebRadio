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

#ifndef RING_BUF_AUDIO_H
#define RING_BUF_AUDIO_H

//--------------------------------------------
int ring_buf_audio_init(void);
int ring_buf_audio_clear(void);
int ring_buf_audio_put(uint8_t *buf, size_t size);
int ring_buf_audio_get(uint8_t *buf, size_t size);
int ring_buf_audio_get_percentage_fill(void);
int ring_buf_audio_destroy(void);

#endif /* RING_BUF_AUDIO_H */