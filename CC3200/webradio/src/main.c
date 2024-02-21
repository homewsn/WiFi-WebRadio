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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
// Simplelink includes
#include "simplelink.h"
// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
// Free_rtos/ti-rtos includes
#include "osi.h"
// Common interface includes
#include "gpio_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "pinmux.h"
// HTTP lib includes
#include "HttpCore.h"
#include "HttpDynamic.h"
// Application includes
#include "vs1053.h"
#include "ring_buf_audio.h"

//--------------------------------------------
#ifndef NOTERM
#define dprintf  Report
#else
#define dprintf
#endif

#ifndef FATAL_ERROR
#define fatal_error()
#endif

//--------------------------------------------
#define SL_STOP_TIMEOUT            (200)
#define WIFI_AP_SSID               "webradio"

//--------------------------------------------
#define MAX_LOCATION_LENGTH  256
typedef struct
{
	char location[MAX_LOCATION_LENGTH];
	bool use_list;
	size_t list_record;
} webradio_t;
static webradio_t webradio;

//--------------------------------------------
#define MAX_SSID_LENGTH       (32 + 1)
#define MAX_PASSWORD_LENGTH   (63 + 1)
typedef struct
{
	char ssid[MAX_SSID_LENGTH];
	char password[MAX_PASSWORD_LENGTH];
	size_t list_record;
} wifi_ap_t;
static wifi_ap_t wifi_ap;
static int16_t wifi_mode;

//--------------------------------------------
#define GET_WIFI_AP_CGI            "/get_wifiap.cgi"
#define POST_WIFI_AP_CGI           "/post_wifiap.cgi"
//#define POST_WIFI0_AP_CGI          "/post_wifiap0.cgi"
#define GET_WEBRADIO_CGI           "/get_webradio.cgi"
#define POST_WEBRADIO_CGI          "/post_webradio.cgi"
#define WIFI_AP_LIST               "options/wifiap.lst"
#define WEBRADIO_LIST              "options/webradio.lst"
#define GET_WIFI_MODE_JS           "/mode.js"
#define GET_WIFI_MODE_JS_CONTENT   "let mode = %d;"

//--------------------------------------------
#define IP_ACQUIRED_WAIT_SEC       6

//--------------------------------------------
#define SPAWN_TASK_PRIORITY        9
#define MAIN_TASK_STACK_SIZE       2048
#define MAIN_TASK_PRIORITY         3
#define PLAY_TASK_STACK_SIZE       256
#define PLAY_TASK_PRIORITY         4
#define HTTP_SERVER_STACK_SIZE     1024
#define HTTP_SERVER_TASK_PRIORITY  5

//--------------------------------------------
#define RECV_BUFFER_SIZE           1024
#define PLAY_BUFFER_SIZE           256
#define ICY_BUFFER_SIZE            1024
#define RESP_CONTEXT_BUFFER_SIZE   1024

//--------------------------------------------
typedef enum
{
	webradio_not_connected = 0,
	webradio_link_connected,
	webradio_load_location,
	webradio_html_header,
	webradio_audio_stream
} webradio_state_t;
static webradio_state_t webradio_state;
static uint8_t icy_buf[ICY_BUFFER_SIZE];
static uint8_t resp_context[RESP_CONTEXT_BUFFER_SIZE];

#ifdef FATAL_ERROR
//--------------------------------------------
static void fatal_error(void)
{
	GPIO_IF_LedOff(MCU_ALL_LED_IND);
    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
	while (1);
}
#endif


#ifdef USE_FREERTOS
//============================================
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//--------------------------------------------
// Application defined hook (or callback) function - assert
void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
	fatal_error();
}

//--------------------------------------------
// Application defined idle task hook
void vApplicationIdleHook(void)
{
	//Handle Idle Hook for Profiling, Power Management etc
}

//--------------------------------------------
// Application defined malloc failed hook
void vApplicationMallocFailedHook(void)
{
	fatal_error();
}

//--------------------------------------------
// Application defined stack overflow hook
void vApplicationStackOverflowHook(OsiTaskHandle *pxTask, signed char *pcTaskName)
{
	fatal_error();
}
#endif //USE_FREERTOS


//============================================
// SimpleLink callback functions
//--------------------------------------------
// WLAN Events
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
	if (pWlanEvent == NULL)
	{
		return;
	}
	switch (pWlanEvent->Event)
	{
	case SL_WLAN_CONNECT_EVENT:
		dprintf("STA Connected to AP: %s , BSSID: %x:%x:%x:%x:%x:%x\n\r",
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_name,
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid[0],
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid[1],
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid[2],
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid[3],
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid[4],
		        pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid[5]);
		break;
	case SL_WLAN_DISCONNECT_EVENT:
		webradio_state = webradio_not_connected;
		break;
	case SL_WLAN_STA_CONNECTED_EVENT:
		break;
	case SL_WLAN_STA_DISCONNECTED_EVENT:
		break;
	}
}
//--------------------------------------------
// Network events
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
	if (pNetAppEvent == NULL)
    {
		return;
	}
	switch(pNetAppEvent->Event)
    {
	case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
		webradio_state = webradio_link_connected;
		dprintf("IPv4 acquired: IP = %d.%d.%d.%d\n\r",
		        (uint8_t)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
		        (uint8_t)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
		        (uint8_t)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
		        (uint8_t)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0));
		break;
	case SL_NETAPP_IP_LEASED_EVENT:
		break;
	case SL_NETAPP_IP_RELEASED_EVENT:
		break;
	default:
		break;
	}
}
//--------------------------------------------
// HTTP server events
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse)
{
}
//--------------------------------------------
// General events
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
}
//--------------------------------------------
// Socket events
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
}


//============================================
// Application static functions
//--------------------------------------------
static void load_list(const char* name, uint8_t **context, size_t *length)
{
	long handle = -1;
	unsigned long token = 0;
	volatile int32_t res;
	SlFsFileInfo_t info;

	res = sl_FsOpen((unsigned char *)name, FS_MODE_OPEN_READ, &token, &handle);
	res = sl_FsGetInfo((unsigned char *)name, token, &info);
	*length = info.FileLen;
	// the last byte is for null (required for string functions in other procedures to count webradio.total_list_records)
	*context = (uint8_t *)malloc(info.FileLen + 1);
	*(*context + info.FileLen) = 0;
	res = sl_FsRead(handle, 0, *context, info.FileLen);
	res = sl_FsClose(handle, NULL, NULL , 0);
}

//--------------------------------------------
static void save_list(const char* name, uint8_t *context, size_t length)
{
	long handle = -1;
	unsigned long token = 0;
	volatile int32_t res;

	res = sl_FsOpen((unsigned char *)name, FS_MODE_OPEN_WRITE, &token, &handle);
	res = sl_FsWrite(handle, 0, context, length);
	res = sl_FsClose(handle, NULL, NULL , 0);
}

//--------------------------------------------
static void load_webradio_location_from_buf(char *location, size_t length)
{
	memset(webradio.location, 0, sizeof(webradio.location));
	strncpy(webradio.location, (char const *)location, length);
	webradio.use_list = false;
}

//--------------------------------------------
static void load_webradio_location_from_list(uint8_t *context, size_t length)
{
	if (length > 0)
	{
		char *beg, *beg1;
		char *end, *end1;
		size_t cnt;

		memset(webradio.location, 0, sizeof(webradio.location));
		beg = (char *)context;
		end = strstr((char const *)beg, "\r\n");
		if (end)
		{
			beg1 = beg;
			end1 = end;
			// the first record
			if (webradio.list_record == 1)
			{
				strncpy(webradio.location, (char const *)beg, end - beg);
				return;
			}
			for (cnt = 2; end < ((char *)context + length); cnt++)
			{
				beg = end + 2;
				end = strstr((char const *)beg, "\r\n");
				if (!end)
				{
					end = (char *)context + length;
				}
				if (webradio.list_record == cnt)
				{
					strncpy(webradio.location, (char const *)beg, end - beg);
					return;
				}
			}
			webradio.list_record = 1;
			strncpy(webradio.location, (char const *)beg1, end1 - beg1);
		}
		else
		{
			// only one record
			webradio.list_record = 1;
			strncpy(webradio.location, (char const *)beg, length);
		}
	}
	else
	{
		fatal_error();
	}
}

//--------------------------------------------
static void set_first_webradio(void)
{
	webradio.use_list = true;
	webradio.list_record = 1;
	webradio_state = webradio_load_location;
}

//--------------------------------------------
static void set_next_webradio(void)
{
	webradio.use_list = true;
	webradio.list_record += 1;
	webradio_state = webradio_load_location;
}

//--------------------------------------------
static void load_webradio_location(void)
{
	uint8_t *context;
	size_t length;

	if (webradio.use_list)
	{
		load_list(WEBRADIO_LIST, &context, &length);
		load_webradio_location_from_list(context, length);
		free(context);
	}
}

//--------------------------------------------
static int load_wifi_ap_from_list(uint8_t *context, size_t length)
{
	if (length > 0)
	{
		char *beg;
		char *end;
		size_t len;
		size_t cnt;

		memset(wifi_ap.ssid, 0, sizeof(wifi_ap.ssid));
		memset(wifi_ap.password, 0, sizeof(wifi_ap.password));
		beg = (char *)context;
		end = strstr((char const *)beg, "\r\n");
		len = end - beg;
		if (end)
		{
			// the first record
			if (wifi_ap.list_record == 1)
			{
				end = strstr((char const *)beg, "/");
				strncpy(wifi_ap.ssid, (char const *)beg, end - beg);
				strncpy(wifi_ap.password, (char const *)(end + 1), len - (end - beg + 1));
				return 0;
			}
			for (cnt = 2; end < ((char *)context + length); cnt++)
			{
				beg = end + 2;
				end = strstr((char const *)beg, "\r\n");
				if (!end)
				{
					end = (char *)context + length;
				}
				len = end - beg;
				if (wifi_ap.list_record == cnt)
				{
					end = strstr((char const *)beg, "/");
					strncpy(wifi_ap.ssid, (char const *)beg, end - beg);
					strncpy(wifi_ap.password, (char const *)(end + 1), len - (end - beg + 1));
					return 0;
				}
			}
			return -1;
		}
		else
		{
			if (wifi_ap.list_record != 1)
			{
				return -1;
			}
			// only one record
			wifi_ap.list_record = 1;
			end = strstr((char const *)beg, "/");
			strncpy(wifi_ap.ssid, (char const *)beg, end - beg);
			strncpy(wifi_ap.password, (char const *)(end + 1), length - (end - beg + 1));
		}
	}
	else
	{
		fatal_error();
	}
	return 0;
}

//--------------------------------------------
static int load_wifi_ap(void)
{
	uint8_t *context;
	size_t length;
	int res;

	load_list(WIFI_AP_LIST, &context, &length);
	res = load_wifi_ap_from_list(context, length);
	free(context);
	return res;
}

//--------------------------------------------
static void set_first_wifi_ap(void)
{
	wifi_ap.list_record = 1;
}

//--------------------------------------------
static void set_next_wifi_ap(void)
{
	wifi_ap.list_record += 1;
}


//============================================
// Application HTTP server handlers
//--------------------------------------------
unsigned char *get_wifiap_cgi(void *args)
{
	uint8_t *buf;
	size_t length;

	load_list(WIFI_AP_LIST, &buf, &length);
	if (length > sizeof(resp_context))
	{
		length = sizeof(resp_context) - 1;
	}
	strncpy((char *)resp_context, (char const *)buf, length);
	resp_context[length] = '\0';
	free(buf);
	return resp_context;
}
//--------------------------------------------
unsigned char *post_wifiap_cgi(void *args)
{
	struct HttpBlob *p = args;
	save_list(WIFI_AP_LIST, p->pData, p->uLength);
	set_first_wifi_ap();
	webradio_state = webradio_not_connected;
#if 0
	return "";
#else
	return 0;
#endif
}
//--------------------------------------------
unsigned char *get_webradio_cgi(void *args)
{
	uint8_t *buf;
	size_t length;

	load_list(WEBRADIO_LIST, &buf, &length);
	if (length > sizeof(resp_context))
	{
		length = sizeof(resp_context) - 1;
	}
	strncpy((char *)resp_context, (char const *)buf, length);
	resp_context[length] = '\0';
	free(buf);
	return resp_context;
}
//--------------------------------------------
unsigned char *post_webradio_cgi(void *args)
{
	struct HttpBlob *p = args;
	save_list(WEBRADIO_LIST, p->pData, p->uLength);
	set_first_webradio();
#if 0
	return "";
#else
	return 0;
#endif
}
//--------------------------------------------
unsigned char *get_mode_js(void *args)
{
	sprintf((char *)resp_context, GET_WIFI_MODE_JS_CONTENT, wifi_mode);
	return resp_context;
}


//============================================
// Application static functions
//--------------------------------------------
static void feed(uint8_t *buf, size_t size)
{
#if 1
	for (size_t len = 0; len < size;)
	{
		size_t feed_len;
		feed_len = ring_buf_audio_put(buf + len, size - len);
		len += feed_len;
		osi_Sleep(1);
	}
#else
	vs1053_write_data(buf, size);
#endif
}

//--------------------------------------------
static int webradio_recv_cb(uint8_t *pdata, size_t len, bool init)
{
	static size_t icy_metaint;
	static size_t icy_metaint_cnt;
	static size_t skip_next_bytes;
	static size_t icy_buf_cnt;

	if (init)
	{
		icy_metaint = 0;
		icy_metaint_cnt = 0;
		skip_next_bytes = 0;
		icy_buf_cnt = 0;
		return 0;
	}
	if (webradio_state == webradio_html_header)
	{
		size_t html_block_length;
		uint8_t *str;

		dprintf((char const *)pdata);
		str = (uint8_t *)strstr((char const *)pdata, "\r\n\r\n");
		if (!str)
		{
			return 1;
		}
		html_block_length = str - pdata + 4;
		webradio_state = webradio_audio_stream;
		str = (uint8_t *)strstr((char const *)pdata, "icy-metaint:");
		if (str)
		{
			icy_metaint = strtol((const char *)(str + 12), NULL, 10);
			icy_metaint_cnt = 0;
		}
		else
		{
			icy_metaint = 0;
		}
		dprintf("icy_metaint: %d\r\n", icy_metaint);
		dprintf("icy_metaint_cnt: %d\r\n", icy_metaint_cnt);
		dprintf("html_block_length: %d\r\n", html_block_length);
		dprintf("len: %d\r\n", len);
		if (html_block_length == len)
		{
			return 2;
		}
		skip_next_bytes = html_block_length;
	}

	size_t meta_size = 0;
	size_t buf_cnt = 0;
	size_t size_to_play = 0;

	while (buf_cnt < len)
	{
		if (icy_metaint && !skip_next_bytes && (buf_cnt + (icy_metaint - icy_metaint_cnt) < len))
		{
			size_to_play = icy_metaint - icy_metaint_cnt;
			meta_size = pdata[buf_cnt + size_to_play];
			meta_size = meta_size * 16 + 1;
		}
		else
		{
			size_to_play = len - buf_cnt;
		}
		if (!skip_next_bytes)
		{
			feed(pdata + buf_cnt, size_to_play);
			buf_cnt += size_to_play;
			if (icy_metaint)
			{
				icy_metaint_cnt += size_to_play;
				if (icy_metaint_cnt == icy_metaint)
				{
					skip_next_bytes = meta_size;
				}
				else
				{
					skip_next_bytes = 0;
				}
			}
			continue;
		}
		if (skip_next_bytes)
		{
			if (len - buf_cnt < skip_next_bytes)
			{
				if (icy_metaint && icy_metaint_cnt)
				{
					size_t copy_bytes;
					copy_bytes = sizeof(icy_buf) - icy_buf_cnt < len - buf_cnt ? sizeof(icy_buf) - icy_buf_cnt : len - buf_cnt;
					if (icy_buf_cnt >= sizeof(icy_buf) || copy_bytes > sizeof(icy_buf) - icy_buf_cnt)
					{
						return -1;
					}
					else
					{
						memcpy(icy_buf + icy_buf_cnt, pdata + buf_cnt, copy_bytes);
					}
					icy_buf_cnt += len - buf_cnt;
				}
				size_to_play = 0;
				skip_next_bytes = skip_next_bytes - (len - buf_cnt);
				buf_cnt = len;
			}
			else
			{
				if (icy_metaint && icy_metaint_cnt)
				{
					size_t copy_bytes;
					copy_bytes = sizeof(icy_buf) - icy_buf_cnt < skip_next_bytes ? sizeof(icy_buf) - icy_buf_cnt : skip_next_bytes;
					if (icy_buf_cnt >= sizeof(icy_buf) || copy_bytes > sizeof(icy_buf) - icy_buf_cnt)
					{
						return -2;
					}
					else
					{
						memcpy(icy_buf + icy_buf_cnt, pdata + buf_cnt, copy_bytes);
					}
					icy_buf_cnt = 0;
					icy_metaint_cnt = 0;
				}
				size_to_play -= skip_next_bytes;
				buf_cnt += skip_next_bytes;
				skip_next_bytes = 0;
			}
			continue;
		}
	}
	return 0;
}

//--------------------------------------------
#define HTTP_VER1_0          "HTTP/1.0"
#define HTTP_VER1_1          "HTTP/1.1"
#define HTTP_LOCATION        "Location:"
#define HTTP_STRING_END      "\r\n"
#define HTTP_HEADER_END      "\r\n\r\n"

//--------------------------------------------
static int check_html_header_status_code(uint8_t *pdata, size_t len, uint32_t *status)
{
	size_t hlen = sizeof(HTTP_VER1_0) - 1;
	char *beg;
	char *end;

	beg = strstr((char const *)pdata, HTTP_HEADER_END);
	if (!beg)
	{
		return -1;
	}
	// Verify HTTP version is 1.1 or 1.0
	if (strncmp((char const*)pdata, HTTP_VER1_0, hlen) &&
		strncmp((char const*)pdata, HTTP_VER1_1, hlen))
	{
		return -2;
	}
	*status = strtoul((char const*)(pdata + hlen), NULL, 10);
	if (!*status)
	{
		return -3;
	}
	if (*status == 201 || *status == 301 || *status == 302 || *status == 303 || *status == 307 || *status == 308)
	{
		// Redirect
		beg = strstr((char const *)pdata, HTTP_LOCATION);
		end = strstr((char const *)beg, HTTP_STRING_END);
		if (!beg || !end)
		{
			return -4;
		}
		beg += sizeof(HTTP_LOCATION) - 1;
		if (*beg == ' ')
		{
			beg += 1;
		}
		load_webradio_location_from_buf(beg, end - beg);
	}
	else
	{
		// No redirect, use server list
		webradio.use_list = true;
	}
	return 0;
}

//--------------------------------------------
static int webradio_recv(short sock_id)
{
	static uint8_t recv_buf[RECV_BUFFER_SIZE];
    long non_blocking = 1;
    volatile int32_t res;
	size_t len;
	uint32_t status;

	// setting socket option to make the socket as non blocking
	res = sl_SetSockOpt(sock_id, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &non_blocking, sizeof(non_blocking));
	webradio_recv_cb(NULL, 0, true);
	while (1)
	{
		res = sl_Recv(sock_id, &recv_buf, sizeof(recv_buf), 0);
		len = (uint32_t)res;
		if (res == SL_EAGAIN)
		{
			osi_Sleep(1);
			continue;
		}
		if (res < 0)
		{
	        dprintf("TCP packet reception error.\r\n");
		    res = sl_Close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			return -1;
		}
		if (res == 0)
		{
	        dprintf("Connection closed.\r\n");
		    res = sl_Close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			return -2;
		}
#ifdef DEBUG_INT
		check_debug_int(&tcp_debug_int, len);
#endif
		if (webradio_state == webradio_load_location)
		{
			// New location
		    res = sl_Close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			return -3;
		}
		if (webradio_state == webradio_html_header)
		{
			res = check_html_header_status_code(recv_buf, len, &status);
			if (res < 0)
			{
				fatal_error();
			}
			if (status != 200)
			{
				// Redirect or error
			    res = sl_Close(sock_id);
				if (res < 0)
				{
					fatal_error();
				}
				if (webradio.use_list == false)
				{
					// redirect
					return 0;
				}
				else
				{
					// error
					return -4;
				}
			}
		}
		if (webradio_state == webradio_not_connected)
		{
			// change WiFi AP
		    res = sl_Close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			return 0;
		}
		res = webradio_recv_cb(recv_buf, len, false);
		if (res < 0)
		{
			// Not supported
	        dprintf("Error in audio stream parsing.\r\n");
		    res = sl_Close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			set_next_webradio();
			return -5;
		}
	}
}

//--------------------------------------------
#define HTTP_PREFIX          "http://"
#define HTTPS_PREFIX         "https://"
#define GET_REQUEST_FORMAT   "GET /%s HTTP/1.1\r\nHost:%s\r\nicy-metadata:1\r\n\r\n"

//--------------------------------------------
static int webradio_connect(const char *uri, short *sock_id)
{
	char *domain;
	size_t domain_len = 0;
	char *url;
	char *urn;
	size_t urn_len = 0;
	char *delim;
	size_t port = 0;
	bool is_secured = false;
	uint32_t ip_addr;
	volatile int32_t res;
	char *get_request;

	url = (char *)uri;
	if (strncasecmp(url, HTTP_PREFIX, sizeof(HTTP_PREFIX) - 1) == 0)
	{
		url += sizeof(HTTP_PREFIX) - 1;
	}
	else if (strncasecmp(url, HTTPS_PREFIX, sizeof(HTTPS_PREFIX) - 1) == 0)
	{
		is_secured = true;
		url += sizeof(HTTPS_PREFIX) - 1;
	}
	delim = strchr(url, ':');
	if (delim != NULL)
	{
		domain_len = delim - url;
		port = strtoul((delim + 1), NULL, 10);
	}
	else
	{
		if (is_secured)
		{
			port = 443;
		}
		else
		{
			port = 80;
		}
	}
	delim = strchr(url, '/');
	if (delim != NULL)
	{
		if (!domain_len)
		{
			domain_len = delim - url;
		}
		urn = delim + 1;
		urn_len = strlen(urn);
	}
	else
	{
		if (!domain_len)
		{
			domain_len = strlen(url);
		}
		urn = NULL;
		urn_len = 0;
	}
	if (!domain_len)
	{
		return -1;
	}

	domain = malloc(domain_len + 1);
	if (!domain)
	{
		return -2;
	}
	domain[domain_len] = '\0';
	strncpy(domain, url, domain_len);

	res = sl_NetAppDnsGetHostByName((signed char *)domain, domain_len, (unsigned long *)&ip_addr, SL_AF_INET);
	if (res < 0)
	{
		free(domain);
		return -3;
	}

	// Creating a TCP socket
	if (is_secured)
	{
		*sock_id = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
	}
	else
	{
		*sock_id = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_IPPROTO_TCP);
	}
	if (*sock_id < 0)
	{
		return -4;
	}
	// Connecting to TCP server
	SlSockAddrIn_t addr = { 0 };
	addr.sin_family = SL_AF_INET;
	addr.sin_port = sl_Htons((unsigned short)port);
	addr.sin_addr.s_addr = sl_Htonl((unsigned int)ip_addr);
	res = sl_Connect(*sock_id, (SlSockAddr_t *)&addr, sizeof(addr));
	if (res < 0 && res != SL_ESECSNOVERIFY)
	{
		sl_Close(*sock_id);
		dprintf("Failed to connect to the web radio server.\r\n");
		free(domain);
		return -5;
	}

	// Send GET request
	get_request = malloc(sizeof(GET_REQUEST_FORMAT) + domain_len + urn_len);
	if (!get_request)
	{
		sl_Close(*sock_id);
		free(domain);
		return -6;
	}
	sprintf(get_request, GET_REQUEST_FORMAT, urn, domain);
	res = sl_Send(*sock_id, get_request, strlen(get_request), 0 );
	free(get_request);
	free(domain);
	if (res < 0)
	{
		sl_Close(*sock_id);
		return -7;
	}
	return 0;
}

//--------------------------------------------
// Connect to WiFi AP
static int wifi_ap_connect(SlSecParams_t *sec_params, char *ssid)
{
	size_t cnt = 0;
	int32_t res;

	dprintf("Please wait...trying to connect to the AP\n\r");
	dprintf("\n\r");
	res = sl_WlanConnect((signed char*)ssid, strlen(ssid), 0, sec_params, 0);
	if (res < 0)
	{
		return -1;
	}

	while (webradio_state != webradio_link_connected && cnt < IP_ACQUIRED_WAIT_SEC)
	{
		osi_Sleep(1000);
		cnt++;
	}
	if (webradio_state != webradio_link_connected)
	{
		dprintf("Failed to connect to AP %s\n\r", ssid);
		return -2;
	}
	return 0;
}


//============================================
// Tasks functions
//--------------------------------------------
void play_task(void *pvParameters)
{
	bool start;
	static uint8_t buf[PLAY_BUFFER_SIZE];
	size_t size;

	while (1)
	{
		switch (ring_buf_audio_get_percentage_fill())
		{
		case 0:
			start = false;
			break;
		case 100:
			start = true;
			break;
		}
		if (start)
		{
			size = ring_buf_audio_get(buf, sizeof(buf));
			vs1053_write_data(buf, size);
		}
		else
		{
			osi_Sleep(500);
		}
	}
}

//--------------------------------------------
void http_server_task(void *pvParameters)
{
	// Run the application HTTP server
	HttpServerInitAndRun(NULL);
}

//--------------------------------------------
void main_task(void *pvParameters)
{
	static OsiTaskHandle http_server_task_handle;
	static OsiTaskHandle play_task_handle;
	volatile int res;

	// Turn NWP on
	wifi_mode = sl_Start(0, 0, 0);

	if (wifi_mode != ROLE_STA)
	{
		// Switch to STA role and restart 
		res = sl_WlanSetMode(ROLE_STA);
		res = sl_Stop(SL_STOP_TIMEOUT);
		wifi_mode = sl_Start(0, 0, 0);
	}

	ring_buf_audio_init();

	// Start play task
	res = osi_TaskCreate(play_task, (const signed char*)"player", PLAY_TASK_STACK_SIZE, NULL, PLAY_TASK_PRIORITY, &play_task_handle);
	if (res < 0)
	{
		fatal_error();
	}

	// Stop NWP HTTP server
	res = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
	if (res < 0)
	{
		fatal_error();
	}
	SetResources(GET, GET_WIFI_MODE_JS, get_mode_js);
	SetResources(GET, GET_WIFI_AP_CGI, get_wifiap_cgi);
	SetResources(POST, POST_WIFI_AP_CGI, post_wifiap_cgi);
	SetResources(GET, GET_WEBRADIO_CGI, get_webradio_cgi);
	SetResources(POST, POST_WEBRADIO_CGI, post_webradio_cgi);

	// Start the application HTTP server task
	res = osi_TaskCreate(http_server_task, (const signed char*)"http_server", HTTP_SERVER_STACK_SIZE, NULL, HTTP_SERVER_TASK_PRIORITY, &http_server_task_handle);
	if (res < 0)
	{
		fatal_error();
	}

	set_first_webradio();
	webradio_state = webradio_not_connected;

	while (1)
	{
		if (webradio_state == webradio_not_connected)
		{
			set_first_wifi_ap();
			while (1)
			{
				// Connecting to WiFi AP
				if (load_wifi_ap() < 0)
				{
					// Setup as WiFi AP
					res = sl_WlanSetMode(ROLE_AP);

					uint8_t ssid[33];
					memcpy(ssid, WIFI_AP_SSID, sizeof(WIFI_AP_SSID));
					res = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, sizeof(WIFI_AP_SSID), ssid);

					uint8_t open = SL_SEC_TYPE_OPEN;
					res = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, (uint8_t *)&open);

					res = sl_Stop(SL_STOP_TIMEOUT);
    				wifi_mode = sl_Start(NULL, NULL, NULL);

					// Stop the play task
					osi_TaskDelete(&play_task_handle);
					// Stop the application HTTP server task
					osi_TaskDelete(&http_server_task_handle);
					// Start the application HTTP server task
					res = osi_TaskCreate(http_server_task, (const signed char*)"http_server", HTTP_SERVER_STACK_SIZE, NULL, HTTP_SERVER_TASK_PRIORITY, &http_server_task_handle);
					if (res < 0)
					{
						fatal_error();
					}
    				GPIO_IF_LedOn(MCU_RED_LED_GPIO);
					while (1)
					{
						osi_Sleep(1000);
					}
				}
				SlSecParams_t sec_params = { 0 };
				sec_params.Key = (signed char*)wifi_ap.password;
				sec_params.KeyLen = strlen(wifi_ap.password);
				sec_params.Type = SL_SEC_TYPE_WPA_WPA2;
				res = wifi_ap_connect(&sec_params, wifi_ap.ssid);
				if (!res)
				{
					break;
				}
				set_next_wifi_ap();
			}
		}
		// Connecting to the audio stream server
		static short sock_id;
		load_webradio_location();
		res = webradio_connect(webradio.location, &sock_id);
		if (res < 0)
		{
			set_next_webradio();
			dprintf("Failed to connect to %s.\r\n", webradio.location);
			continue;
		}
	    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
		webradio_state = webradio_html_header;
		// Receiving TCP packets
		if (webradio_recv(sock_id) < 0)
		{
			// New audio stream
			ring_buf_audio_clear();
			vs1053_reset();
		}
	    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
	}
}

//--------------------------------------------
// Board Initialization & Configuration
static void BoardInit(void)
{
	// In case of TI-RTOS vector table is initialize by OS itself
#ifndef USE_TIRTOS
	// Set vector table base
#if defined(ccs) || defined(gcc)
	extern void (* const g_pfnVectors[])(void);
	MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
	extern uVectorEntry __vector_table;
	MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif

	// Enable Processor
	MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

	PRCMCC3200MCUInit();
}

//--------------------------------------------
void main()
{
	long res;

	// Board Initialization
	BoardInit();
	// configure the GPIO pins for LEDs,UART
	PinMuxConfig();
	// Configure the UART
#ifndef NOTERM
	InitTerm();
#endif

	vs1053_init_iface();

	// Configure all 3 LEDs
	GPIO_IF_LedConfigure(LED1 | LED2 | LED3);
	// switch off all LEDs
	GPIO_IF_LedOff(MCU_ALL_LED_IND);
    
	// Start the SimpleLink Host
	res = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
	if (res < 0)
	{
		fatal_error();
	}

	// Start the main task
	res = osi_TaskCreate(main_task, (const signed char*)"main", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL);
	if (res < 0)
	{
		fatal_error();
	}

	// Start the task scheduler
	osi_start();
}
