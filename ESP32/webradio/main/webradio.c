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

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "wr_socket.h"
#include "vs1053.h"

//--------------------------------------------
#define RING_BUF_ENABLED           1

//--------------------------------------------
#if RING_BUF_ENABLED
#include "ring_buf_audio.h"
#endif

//--------------------------------------------
static const char* TAG = "app_main";

//--------------------------------------------
#define WIFI_AP_SSID               "webradio"

//--------------------------------------------
#define MAX_LOCATION_LENGTH        256
typedef struct
{
	char location[MAX_LOCATION_LENGTH];
	bool use_list;
	size_t list_record;
} webradio_t;
static webradio_t webradio;

//--------------------------------------------
#define MAX_SSID_LENGTH            (32 + 1)
#define MAX_PASSWORD_LENGTH        (63 + 1)
typedef struct
{
	char ssid[MAX_SSID_LENGTH];
	char password[MAX_PASSWORD_LENGTH];
	size_t list_record;
} wifi_ap_t;
static wifi_ap_t wifi_ap;
static int16_t wifi_mode;

//--------------------------------------------
#define GET_ABOUT_HTML             "/about.html"
#define GET_ABOUT_HTML_FILE        "/spiffs/www/about.html"
#define GET_INDEX_HTML             "/"
#define GET_INDEX_HTML_FILE        "/spiffs/www/index.html"
#define GET_OPTIONS_HTML           "/options.html"
#define GET_OPTIONS_HTML_FILE      "/spiffs/www/options.html"
#define GET_STYLE_CSS              "/style.css"
#define GET_STYLE_CSS_FILE         "/spiffs/www/style.css"
#define GET_WEBRADIO_HTML          "/webradio.html"
#define GET_WEBRADIO_HTML_FILE     "/spiffs/www/webradio.html"
#define GET_WEBRADIO_JS            "/webradio.js"
#define GET_WEBRADIO_JS_FILE       "/spiffs/www/webradio.js"
#define GET_WIFIAP_HTML            "/wifiap.html"
#define GET_WIFIAP_HTML_FILE       "/spiffs/www/wifiap.html"
#define GET_WIFIAP_JS              "/wifiap.js"
#define GET_WIFIAP_JS_FILE         "/spiffs/www/wifiap.js"
#define GET_WIFIAP0_HTML           "/wifiap0.html"
#define GET_WIFIAP0_HTML_FILE      "/spiffs/www/wifiap0.html"
#define GET_WIFI_AP_CGI            "/get_wifiap.cgi"
#define POST_WIFI_AP_CGI           "/post_wifiap.cgi"
//#define POST_WIFI0_AP_CGI          "/post_wifiap0.cgi"
#define GET_WEBRADIO_CGI           "/get_webradio.cgi"
#define POST_WEBRADIO_CGI          "/post_webradio.cgi"
#define WIFI_AP_LIST               "/spiffs/options/wifiap.lst"
#define WEBRADIO_LIST              "/spiffs/options/webradio.lst"
#define GET_WIFI_MODE_JS           "/mode.js"
#define GET_WIFI_MODE_JS_CONTENT   "let mode = %d;"

//--------------------------------------------
#define IP_ACQUIRED_WAIT_SEC       6

//--------------------------------------------
#define PLAY_TASK_STACK_SIZE       1024
#define PLAY_TASK_PRIORITY         1

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
	while (1);
}
#else
#define fatal_error()
#endif


//============================================
// Global functions
//--------------------------------------------
void delay_ms(uint32_t time_ms)
{
    uint32_t ticks = time_ms / portTICK_PERIOD_MS;
    if (ticks)
    {
        vTaskDelay(ticks);
    }
    else
    {
        vTaskDelay(1);
    }
}

#if 0
//============================================
// Changing UART baudrate
//--------------------------------------------
static void change_uart_baudrate(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_LOGI(TAG, "UART baudrate is %d", uart_config.baud_rate);
}
#endif

//============================================
// Application static functions
//--------------------------------------------
static void mount_storage_partition(void)
{
	esp_err_t res;
    size_t total;
    size_t used;
    esp_vfs_spiffs_conf_t conf =
    {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 20,
      .format_if_mount_failed = false
    };
    
    ESP_ERROR_CHECK(res = esp_vfs_spiffs_register(&conf));

    if (res != ESP_OK)
    {
        if (res == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (res == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(res));
        }
        return;
    }

    ESP_ERROR_CHECK(res = esp_spiffs_info(NULL, &total, &used));
    if (res == ESP_OK)
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

//--------------------------------------------
static void load_list(const char* name, uint8_t **context, size_t *length)
{
    FILE* fp;
    size_t size;

    ESP_LOGI(TAG, "Reading from file %s", name);
    fp = fopen(name, "rb");
    if (fp == NULL)
    {
        *context = NULL;
        length = 0;
        ESP_LOGE(TAG, "Failed to open %s for reading", name);
		fatal_error();
        return;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);    
	// the last byte is for null (required for string functions in other procedures to count webradio.total_list_records)
	*context = (uint8_t *)malloc(size + 1);
    if (*context == NULL)
    {
        length = 0;
        ESP_LOGE(TAG, "Failed to allocate %d bytes", size);
		fatal_error();
        return;
    }
    fseek(fp, 0L, SEEK_SET);
    fread(*context, size, 1, fp);
    fclose(fp);
	*length = size;
}

//--------------------------------------------
static void save_list(const char* name, uint8_t *context, size_t length)
{
    FILE* fp;

    ESP_LOGI(TAG, "Writing to file %s", name);
    fp = fopen(name, "wb");
    if (fp == NULL)
    {
        ESP_LOGE(TAG, "Failed to open %s for writing", name);
		fatal_error();
        return;
    }
    fwrite(context, length, 1, fp);
    fclose(fp);
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
// ESP event handlers
//--------------------------------------------
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id)
    {
        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
    		webradio_state = webradio_not_connected;
            break;
    }
}
//--------------------------------------------
static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa((const ip4_addr_t *)&event->ip_info.ip));
		    webradio_state = webradio_link_connected;
            break;
        }
    }
}



//============================================
// HTTP server event handlers
//--------------------------------------------
esp_err_t get_about_html(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_ABOUT_HTML_FILE, &context, &length);
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_index_html(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_INDEX_HTML_FILE, &context, &length);
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_options_html(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_OPTIONS_HTML_FILE, &context, &length);
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_style_css(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_STYLE_CSS_FILE, &context, &length);
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_webradio_html(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_WEBRADIO_HTML_FILE, &context, &length);
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_webradio_js(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_WEBRADIO_JS_FILE, &context, &length);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_wifiap_html(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_WIFIAP_HTML_FILE, &context, &length);
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_wifiap_js(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_WIFIAP_JS_FILE, &context, &length);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_wifiap0_html(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(GET_WIFIAP0_HTML_FILE, &context, &length);
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_wifiap_cgi(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(WIFI_AP_LIST, &context, &length);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t post_wifiap_cgi(httpd_req_t *req)
{
    char*  buf = malloc(req->content_len + 1);
    size_t off = 0;
    int    ret;

    if (!buf)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    while (off < req->content_len)
    {
        // Read data received in the request
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_408(req);
            }
            free (buf);
            return ESP_FAIL;
        }
        off += ret;
        ESP_LOGI(TAG, "/echo handler recv length %d", ret);
    }
    buf[off] = '\0';

	save_list(WIFI_AP_LIST, (uint8_t *)buf, req->content_len);
    free(buf);

    httpd_resp_send(req, "Ok", 2);

	set_first_wifi_ap();
	webradio_state = webradio_not_connected;

    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_webradio_cgi(httpd_req_t *req)
{
	uint8_t *context;
	size_t length;

	load_list(WEBRADIO_LIST, &context, &length);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, (const char *)context, length);
	free(context);
    return ESP_OK;
}
//--------------------------------------------
esp_err_t post_webradio_cgi(httpd_req_t *req)
{
    char*  buf = malloc(req->content_len + 1);
    size_t off = 0;
    int    ret;

    if (!buf)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    while (off < req->content_len)
    {
        // Read data received in the request
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_408(req);
            }
            free (buf);
            return ESP_FAIL;
        }
        off += ret;
        ESP_LOGI(TAG, "/echo handler recv length %d", ret);
    }
    buf[off] = '\0';

	save_list(WEBRADIO_LIST, (uint8_t *)buf, req->content_len);
    free(buf);

    httpd_resp_send(req, "Ok", 2);

	set_first_webradio();

    return ESP_OK;
}
//--------------------------------------------
esp_err_t get_mode_js(httpd_req_t *req)
{
	sprintf((char *)resp_context, GET_WIFI_MODE_JS_CONTENT, wifi_mode);
    httpd_resp_send(req, (const char *)resp_context, strlen((const char *)resp_context));
    return ESP_OK;
}

//--------------------------------------------
httpd_uri_t http_server_handlers[] =
{
    {
        .uri      = GET_ABOUT_HTML,
        .method   = HTTP_GET,
        .handler  = get_about_html,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_INDEX_HTML,
        .method   = HTTP_GET,
        .handler  = get_index_html,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_OPTIONS_HTML,
        .method   = HTTP_GET,
        .handler  = get_options_html,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_STYLE_CSS,
        .method   = HTTP_GET,
        .handler  = get_style_css,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WEBRADIO_HTML,
        .method   = HTTP_GET,
        .handler  = get_webradio_html,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WEBRADIO_JS,
        .method   = HTTP_GET,
        .handler  = get_webradio_js,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WIFIAP_HTML,
        .method   = HTTP_GET,
        .handler  = get_wifiap_html,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WIFIAP_JS,
        .method   = HTTP_GET,
        .handler  = get_wifiap_js,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WIFIAP0_HTML,
        .method   = HTTP_GET,
        .handler  = get_wifiap0_html,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WIFI_MODE_JS,
        .method   = HTTP_GET,
        .handler  = get_mode_js,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WIFI_AP_CGI,
        .method   = HTTP_GET,
        .handler  = get_wifiap_cgi,
        .user_ctx = NULL,
    },
    {
        .uri      = POST_WIFI_AP_CGI,
        .method   = HTTP_POST,
        .handler  = post_wifiap_cgi,
        .user_ctx = NULL,
    },
    {
        .uri      = GET_WEBRADIO_CGI,
        .method   = HTTP_GET,
        .handler  = get_webradio_cgi,
        .user_ctx = NULL,
    },
    {
        .uri      = POST_WEBRADIO_CGI,
        .method   = HTTP_POST,
        .handler  = post_webradio_cgi,
        .user_ctx = NULL,
    }
};



//============================================
// Application static functions
//--------------------------------------------
static void feed(uint8_t *buf, size_t size)
{
#if RING_BUF_ENABLED
	for (size_t len = 0; len < size;)
	{
		size_t feed_len;
		feed_len = ring_buf_audio_put(buf + len, size - len);
		len += feed_len;
        if (len == size)
        {
            break;
        }
		delay_ms(10);
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

//        ESP_LOGI(TAG, "Webradio HTML header: %s", (char const *)pdata);
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
        ESP_LOGI(TAG, "icy_metaint: %d", icy_metaint);
        ESP_LOGI(TAG, "icy_metaint_cnt: %d", icy_metaint_cnt);
        ESP_LOGI(TAG, "html_block_length: %d", html_block_length);
        ESP_LOGI(TAG, "len: %d", len);
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
    volatile int res;
	size_t len;
	uint32_t status;

	webradio_recv_cb(NULL, 0, true);
	while (1)
	{
        res = wr_recv(sock_id, &recv_buf, sizeof(recv_buf), 0);
		len = (uint32_t)res;
		if (res < 0)
		{
            ESP_LOGI(TAG, "TCP packet reception error.");
		    res = wr_close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			return -1;
		}
		if (res == 0)
		{
            ESP_LOGI(TAG, "Connection closed.");
		    res = wr_close(sock_id);
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
		    res = wr_close(sock_id);
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
			    res = wr_close(sock_id);
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
		    res = wr_close(sock_id);
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
            ESP_LOGI(TAG, "Error in audio stream parsing.");
		    res = wr_close(sock_id);
			if (res < 0)
			{
				fatal_error();
			}
			set_next_webradio();
			return -5;
		}
        delay_ms(10);
	}
}

//--------------------------------------------
#define HTTP_PREFIX          "http://"
#define HTTPS_PREFIX         "https://"
#define GET_REQUEST_FORMAT   "GET /%s HTTP/1.1\r\nHost:%s\r\nicy-metadata:1\r\n\r\n"

//--------------------------------------------
static int webradio_connect(const char *uri, int *sock_id)
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
	volatile int res;
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
    ESP_LOGI(TAG, "port: %d or 0x%04X", port, port);
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

    ESP_LOGI(TAG, "call gethostbyname for domain: %s", domain);
    struct hostent *host_info = gethostbyname((const char *)domain);
	if (!host_info)
	{
        ESP_LOGE(TAG, "gethostbyname error for domain: %s", domain);
		free(domain);
		return -3;
	}
    ip_addr = (*(((struct in_addr **)host_info->h_addr_list)[0])).s_addr;
    ESP_LOGI(TAG, "ip_addr: 0x%08lX", ip_addr);
	if (ip_addr == IPADDR_NONE)
	{
        ESP_LOGE(TAG, "ip_addr error for domain: %s", domain);
		free(domain);
		return -3;
	}

	// Creating a TCP socket
	if (is_secured)
	{
		*sock_id = wr_socket(AF_INET, WR_HTTPS_SOCKET, IPPROTO_IP);
	}
	else
	{
		*sock_id = wr_socket(AF_INET, WR_HTTP_SOCKET, IPPROTO_IP);
	}
	if (*sock_id < 0)
	{
		return -4;
	}
    ESP_LOGI(TAG, "sock_id: %d", *sock_id);
	// Connecting to TCP server
    struct sockaddr_in addr = { 0 };
    addr.sin_addr.s_addr = ip_addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
	res = wr_connect(*sock_id, (struct sockaddr *)&addr, sizeof(addr));
	if (res != 0)
	{
		wr_close(*sock_id);
        ESP_LOGE(TAG, "Failed to connect to IP: 0x%08lX, port: %d, error %d %s0x%04X", addr.sin_addr.s_addr, port, res, res < 0 ? "-" : "", res < 0 ? -(unsigned)res : res);
		free(domain);
		return -5;
	}

	// Send GET request
	get_request = malloc(sizeof(GET_REQUEST_FORMAT) + domain_len + urn_len);
	if (!get_request)
	{
		wr_close(*sock_id);
		free(domain);
		return -6;
	}
	sprintf(get_request, GET_REQUEST_FORMAT, urn, domain);
	res = wr_send(*sock_id, get_request, strlen(get_request), 0 );
	free(get_request);
	free(domain);
	if (res < 0)
	{
            ESP_LOGE(TAG, "Failed to send to IP: 0x%08lX, port: %d, error %d", addr.sin_addr.s_addr, port, res);
		wr_close(*sock_id);
		return -7;
	}
	return 0;
}

//--------------------------------------------
static void start_webserver(void)
{
	esp_err_t res;
    size_t cnt;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = sizeof(http_server_handlers)/sizeof(httpd_uri_t);

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    ESP_ERROR_CHECK(res = httpd_start(&server, &config));
    if (res != ESP_OK)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return;
    }
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    for (cnt = 0; cnt < sizeof(http_server_handlers)/sizeof(httpd_uri_t); cnt++)
    {
        if (httpd_register_uri_handler(server, &http_server_handlers[cnt]) != ESP_OK)
        {
            ESP_LOGE(TAG, "register uri failed for %d", cnt);
            return;
        }
    }
}

//--------------------------------------------
// Connect to WiFi AP
static int wifi_ap_connect(wifi_config_t *wifi_config)
{
	size_t cnt = 0;

    ESP_LOGI(TAG, "Trying to connect to WiFi AP, SSID=%s, Password=%s", wifi_config->sta.ssid, wifi_config->sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

	while (webradio_state != webradio_link_connected && cnt < IP_ACQUIRED_WAIT_SEC)
	{
        delay_ms(1000);
		cnt++;
	}
	if (webradio_state != webradio_link_connected)
	{
        ESP_LOGI(TAG, "Failed to connect to AP %s", wifi_config->sta.ssid);
		return -1;
	}

	return 0;
}

//============================================
// Tasks functions
//--------------------------------------------
#if RING_BUF_ENABLED
void play_task(void *pvParameters)
{
	bool start = false;
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
			delay_ms(10);
		}
		else
		{
			delay_ms(500);
		}
	}
}
#endif


//--------------------------------------------
void app_main()
{
	esp_err_t res;

    mount_storage_partition();
    vs1053_init_iface();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

#if RING_BUF_ENABLED
	ring_buf_audio_init();
    xTaskCreate(play_task, "player", PLAY_TASK_STACK_SIZE, NULL, PLAY_TASK_PRIORITY, NULL);
#endif

	set_first_webradio();
	webradio_state = webradio_not_connected;
    wifi_mode = 0;

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    start_webserver();
    esp_netif_create_default_wifi_sta();

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
                    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();
                    esp_netif_set_default_netif(esp_netif_ap);

                    wifi_config_t wifi_config =
                    {
                        .ap =
                        {
                            .ssid = WIFI_AP_SSID,
                            .ssid_len = sizeof(WIFI_AP_SSID),
                            .password = "",
                            .max_connection = 3,
                            .authmode = WIFI_AUTH_OPEN
                        },
                    };

                    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
                    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
                    ESP_ERROR_CHECK(esp_wifi_start());

                    ESP_LOGI(TAG, "Start as open WiFi AP %s", WIFI_AP_SSID);
                    wifi_mode = 2;
                    vTaskSuspend(NULL);
				}
                wifi_config_t wifi_config = { 0 };
                strncpy((char *)wifi_config.sta.ssid, wifi_ap.ssid, strlen(wifi_ap.ssid));
                strncpy((char *)wifi_config.sta.password, wifi_ap.password, strlen(wifi_ap.password));
                wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
				res = wifi_ap_connect(&wifi_config);
				if (!res)
				{
					break;
				}
				set_next_wifi_ap();
			}
		}
		// Connecting to the audio stream server
		static int sock_id;
		load_webradio_location();
		res = webradio_connect(webradio.location, &sock_id);
		if (res < 0)
		{
			set_next_webradio();
			ESP_LOGE(TAG, "webradio_connect error %d", res);
			continue;
		}
		webradio_state = webradio_html_header;
		// Receiving TCP packets
		if (webradio_recv(sock_id) < 0)
		{
			// New audio stream
#if RING_BUF_ENABLED
			ring_buf_audio_clear();
#endif
			vs1053_reset();
		}
	}
}
