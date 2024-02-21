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

#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "wr_socket.h"
#include "sdkconfig.h"

#define TLS_USING_OPENSSL     0
#define TLS_USING_MBEDTLS     1

#if !TLS_USING_OPENSSL && !TLS_USING_MBEDTLS
#pragma GCC error "You must select ssl/tls library"
#endif
#if TLS_USING_OPENSSL && TLS_USING_MBEDTLS
#pragma GCC error "You must select only one ssl/tls library"
#endif

#if TLS_USING_OPENSSL
#include "openssl/ssl.h"
#elif TLS_USING_MBEDTLS
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#if 0
#include "mbedtls/certs.h"
#endif
#endif

//--------------------------------------------
static const char* TAG = "wrsocket";

//--------------------------------------------
void delay_ms(uint32_t time_ms);

//--------------------------------------------
#if TLS_USING_OPENSSL
static SSL_CTX *ctx;
static SSL *ssl;
#elif TLS_USING_MBEDTLS
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_ssl_context ssl;
static mbedtls_x509_crt cacert;
static mbedtls_ssl_config conf;
static mbedtls_net_context server_fd;
static bool mbedtls_init = false;
#endif

//--------------------------------------------
int wr_socket(int domain, int type, int protocol)
{
    int res = 0;

    if (domain != AF_INET || protocol != IPPROTO_IP)
    {
        return -1;
    }
    switch (type)
    {
        case WR_HTTP_SOCKET:
            return socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        case WR_HTTPS_SOCKET:
#if TLS_USING_OPENSSL
        {
            int sock;
            ESP_LOGI(TAG, "Create SSL context ...");
            ctx = SSL_CTX_new(TLSv1_1_client_method());
            if (!ctx)
            {
                ESP_LOGE(TAG, "failed");
                return -2;
            }
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
            ESP_LOGI(TAG, "Create socket ...");
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (sock < 0)
            {
                SSL_CTX_free(ctx);
                ESP_LOGE(TAG, "failed");
                return sock;
            }
            return sock;
        }
#elif TLS_USING_MBEDTLS
            ESP_LOGI(TAG, "Create SSL context ...");
            mbedtls_net_init(&server_fd);
            mbedtls_ssl_init(&ssl);
            mbedtls_ssl_config_init(&conf);
            mbedtls_entropy_init(&entropy);
            if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0) != 0)
            {
                res = -2;
                goto exit;
            }
            if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
            {
                res = -3;
                goto exit;
            }
#if 0
            res = mbedtls_x509_crt_parse(&cacert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
            if (res < 0)
            {
                res = -4;
                goto exit;
            }
#endif
            mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
            mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
            mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
            if (mbedtls_ssl_setup(&ssl, &conf) != 0)
            {
                res = -5;
                goto exit;
            }
            ESP_LOGI(TAG, "Create socket ...");
            server_fd.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (server_fd.fd < 0)
            {
                res = server_fd.fd;
                goto exit;
            }
            mbedtls_init = true;
            return server_fd.fd;
#endif
        default:
            return -1;
    }
#if TLS_USING_OPENSSL
#elif TLS_USING_MBEDTLS
exit:
    ESP_LOGE(TAG, "failed");
    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_init = false;
    return res;
#endif
}

//--------------------------------------------
int wr_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int res;
    res = connect(s, name, namelen);
    if (res < 0)
    {
        return res;
    }
#if TLS_USING_OPENSSL
    if (ctx)
    {
        ESP_LOGI(TAG, "Create SSL ...");
        ssl = SSL_new(ctx);
        if (!ssl)
        {
            SSL_CTX_free(ctx);
            ctx = NULL;
            close(s);
            ESP_LOGI(TAG, "failed");
            return -1;
        }
        if (SSL_set_fd(ssl, s) != 1)
        {
            return -2;
        }
        if (SSL_connect(ssl) != 1)
        {
            return -3;
        }
        return 0;
    }
#elif TLS_USING_MBEDTLS
    if (mbedtls_init)
    {
        ESP_LOGI(TAG, "Create SSL ...");
        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
        return mbedtls_ssl_handshake(&ssl);
    }
#endif
    return res;
}

//--------------------------------------------
ssize_t wr_send(int s, const void *dataptr, size_t size, int flags)
{
#if TLS_USING_OPENSSL
    if (ssl)
    {
        return SSL_write(ssl, dataptr, size);
    }
    else
#elif TLS_USING_MBEDTLS
    if (mbedtls_init)
    {
        return mbedtls_ssl_write( &ssl, dataptr, size);
    }
#endif
    {
        return send(s, dataptr, size, flags);
    }
}


//--------------------------------------------
ssize_t wr_recv(int s, void *mem, size_t len, int flags)
{
#if TLS_USING_OPENSSL
    if (ssl)
    {
        return SSL_read(ssl, mem, len);
    }
    else
#elif TLS_USING_MBEDTLS
    if (mbedtls_init)
    {
        int res;
        while (1)
        {
            res = mbedtls_ssl_read(&ssl, mem, len);
            if (res == MBEDTLS_ERR_SSL_WANT_READ || res == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                delay_ms(10);
                continue;
            }
            break;
        }
        return res;
    }
#endif
    {
        return recv(s, mem, len, flags);
    }
}

//--------------------------------------------
int wr_close(int s)
{
#if TLS_USING_OPENSSL
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = NULL;
    }
    if (ctx)
    {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
#elif TLS_USING_MBEDTLS
    mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_init = false;
#endif
    return close(s);
}
