#include "main.h"


static const char *TAG = "qrio_modem_refactor.c";

// //sección de manejo y configuración del puerto UART1 que se comunica con el MODEM

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart1_event_queue;
static QueueHandle_t modem_responses_queue;

int lineasEsperadas = 0;
int lineasRecibidas = 0;
char cmd_data[100];
int dataPointer = 0;
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    char* dtmp = (char*) malloc(RD_BUF_SIZE);
    for(;;) {
        if(xQueueReceive(uart1_event_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch(event.type) {

                case UART_DATA:
                //ESP_LOGI(TAG,"event.size = %d", event.size);
                    uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
                    
                    xtend_modem_uart_msg_t modem_incoming_data;
                    if( dtmp[0] == 0x0D && dtmp[1] == 0x0A){
                        if ( event.size >= 6 ){
                            // for( int i = 0; i <= event.size; i++ ){
                            //     printf("%c",dtmp[i]);
                            // }
                            memcpy(modem_incoming_data.in_data, dtmp,event.size );
                            //memcpy(modem_incoming_data.datasize, ( int *)event.size, sizeof(event.size));

                            xQueueSend(modem_responses_queue,(void *)&modem_incoming_data, pdMS_TO_TICKS(500));
                        }
                    }                    
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // // If fifo overflow happened, you should consider adding flow control for your application.
                    // // The ISR has already reset the rx FIFO,
                    // // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart1_event_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    // ESP_LOGI(TAG, "ring buffer full");
                    // // If buffer full happened, you should consider encreasing your buffer size
                    // // As an example, we directly flush the rx buffer here in order to read more data.
                    // uart_flush_input(UART_NUM_1);
                    // xQueueReset(1);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    //ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    //ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    //ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    break;
                //Others
                default:
                    //ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}


void xtend_modem(void *pvParameters)
{
    esp_err_t err;
    char modemResponse[100];
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart1_event_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_NUM_1, xtendTx, xtendRx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //creo la queue de mensajes 
    modem_responses_queue = xQueueCreate(10,sizeof(xtend_modem_uart_msg_t));

    //Create a task to handle UART event from ISR
    TaskHandle_t uart_event_task_handle = NULL;
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, uart_event_task_handle);

    /********************************************************UART CONFIGURADO********************************************************/
    int modemState = 0;
    for(;;){
        switch (modemState)
        {
            case 0:{ //estado cero, lo apago y prendo 

                gpio_set_direction(xtendPWR, GPIO_MODE_OUTPUT);    //configuramos el pin que controla el mosfet que prende y apaga el sim800 

                gpio_set_level(xtendPWR, 0);    //por si el modem estaba prendido lo apagamos

                vTaskDelay(pdMS_TO_TICKS(2000));    //esperamos 2 segundos

                gpio_set_level(xtendPWR, 1);    //encendemos el sim

                vTaskDelay(pdMS_TO_TICKS(10000));    //esperamos que arranque el modem

                modemState = 1;
                break;
            }
            case 1:{ // estado 1, modo comandos

                err = xtend_modem_commands("+++", "", NULL ,500); //desactivar el ECHO de mensajes

                err = xtend_modem_commands("CF0", "", NULL ,500); //cambio la representación de las respuestas del modem
                if ( err == ESP_ERR_TIMEOUT){
                    //uart_driver_delete(UART_NUM_1);
                    //return ESP_ERR_TIMEOUT;
                    modemState = 0;
                }
                
                err = xtend_modem_commands("%%V", "", modemResponse ,500); //voltaje de alimentación
                if ( err == ESP_ERR_TIMEOUT){
                    //uart_driver_delete(UART_NUM_1);
                    //return ESP_ERR_TIMEOUT;
                    modemState = 0;
                }
                ESP_LOGI(TAG,"Voltaje de batería: %s",modemResponse);

                err = xtend_modem_commands("DB", "", modemResponse ,65000); //salir de modo gprs (por las dudas)
                if ( err == ESP_ERR_TIMEOUT){
                    //uart_driver_delete(UART_NUM_1);
                    //return ESP_ERR_TIMEOUT;
                    modemState = 0;
                }
                ESP_LOGI(TAG,"Intensidad de señal: %s",modemResponse);

                uart_write_bytes(UART_NUM_1, "ATCN\r",strlen(ATCN\r)); //lo saco del modo comandos

               
                modemState = 2;
                break;
            }
            case 2:{// modo normal, acá solamente espera que lleguen mensajes por el uart

                xtend_modem_uart_msg_t respuesta_recibida;
                if (xQueueReceive(modem_responses_queue, &respuesta_recibida,portMAX_DELAY) == pdTRUE)
                {
                   ESP_LOGI(TAG,"Mensaje recibido: %s", respuesta_recibida.in_data); 
                }
                break;
            }

            default:
                ESP_LOGE(TAG,"Error desconocido");
                break;
        }

    }
    
}




/**
 * @brief Función para enviar comandos al SIM800 y esperar la respuesta, da ESP_ERR_TIMEOUT en caso que no responda,
 * el timeout lo debe manejar la función llamadora
 * 
 * @param commandToSend el comando sin el "AT", enviar "+CPIN?" por ejemplo
 * @param commandResponse la respuesta esperada
 * @param responsePointer puntero a un buffer donde quiero que la función me copie la respuesta recibida por el modem, si no quiero copiar nada enviar un NULL
 * @param timeoutModem el tiempo de timeout, verificar el tiempo máximo de cada comando en el manual del sim800
 * @return esp_err_t ESP_OK si se recibió la respuesta esperada, en caso de recibir una respuesta más larga para parsear en otro lado tambien devuelve ESP_OK
 */
esp_err_t xtend_modem_commands(const char *commandToSend, const char *commandResponse, char *responsePointer ,uint32_t timeoutModem)
{

    //envio el comando 

    char buffer[100];
    if( strstr(commandToSend,"+++") != NULL){
        strcpy(buffer,commandToSend);//strcat(buffer,"\r");
    }
    else{
        strcpy(buffer,"AT");strcat(buffer,commandToSend);strcat(buffer,"\r");
    }
    
    uart_write_bytes(UART_NUM_1, buffer,strlen(buffer)); 

    //espero la respuesta

    xtend_modem_uart_msg_t respuesta_recibida;
    if (xQueueReceive(modem_responses_queue, &respuesta_recibida,pdMS_TO_TICKS(timeoutModem)) == pdTRUE){

        ESP_LOGI(TAG,"Comando %s Respuesta %s ",commandToSend, respuesta_recibida.in_data);

        //if(strstr( respuesta_recibida.in_data, commandResponse) != NULL){
            
            if(responsePointer != NULL){
                memcpy(responsePointer,respuesta_recibida.in_data,strlen(respuesta_recibida.in_data));
            }
            //ESP_LOGI(TAG,"Respuesta esperada recibida");
            return ESP_OK;
        //}
        // else{
        //     //ESP_LOGE(TAG,"No se recibió la respuesta esperada");
        //     return ESP_FAIL;
        // }
    }
    else{
        //ESP_LOGE(TAG,"TIMEOUT de comando %s", commandToSend);
        return ESP_ERR_TIMEOUT;
    }

}
