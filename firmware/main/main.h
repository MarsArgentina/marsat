

//inclues de c
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

//includes externos de c

//inclues de esp-idf
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "cJSON.h"
#include "esp_intr_alloc.h"
#include "esp_event_base.h"
#include "esp_sntp.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_https_ota.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/timer.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "argtable3/argtable3.h"
#include "ping/ping_sock.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

//funciones de freertos
#include "freertos/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/portmacro.h"

//archivos propios
#include "board.h"
#include "xtend.h"