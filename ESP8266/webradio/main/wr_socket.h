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

#ifndef WR_SOCKET
#define WR_SOCKET

//--------------------------------------------
#define WR_HTTP_SOCKET      IPPROTO_IP   // TCP socket without security (http)
#define WR_HTTPS_SOCKET     (100)        // TCP socket with TLS security (https)

//--------------------------------------------
int wr_socket(int domain, int type, int protocol);
int wr_connect(int s, const struct sockaddr *name, socklen_t namelen);
ssize_t wr_send(int s, const void *dataptr, size_t size, int flags);
ssize_t wr_recv(int s, void *mem, size_t len, int flags);
int wr_close(int s);

#endif /* WR_SOCKET */