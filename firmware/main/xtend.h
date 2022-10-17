#ifndef xtend_H_
#define xtend_H_

typedef struct {
    char in_data[128];
    int datasize;
}xtend_modem_uart_msg_t;

esp_err_t xtend_modem_commands(const char *commandToSend, const char *commandResponse, char *responsePointer ,uint32_t timeoutModem);

esp_err_t modem_prepare(char *apn_de_chip);

void xtend_modem(void *pvParameters);

esp_err_t modem_get_rssi(char *respuesta);

#endif