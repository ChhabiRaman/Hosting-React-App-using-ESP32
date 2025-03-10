#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "webpage.h"
#include "network.h"

// Status LED 
#define LED_RED GPIO_NUM_13

// Functions declarations
void led_config(void);
void led_task(void * pvParameter);

// Variables

// Main application
void app_main() 
{
    webpage_obj_t * server_cred;
    ethernet_obj_t * eth_obj;
    eth_obj = pvPortMalloc(sizeof(ethernet_obj_t));
    server_cred = pvPortMalloc(sizeof(webpage_obj_t));
    led_config();

    // The LED task is used to show the connection status
    xTaskCreate(&led_task, "led_task", 2048, eth_obj, 5, NULL);
    strlcpy(eth_obj->mdns_cred.mdns_host_name, "esp_web_server", 15);
    strlcpy(eth_obj->mdns_cred.mdns_instance_name, "esp_web_server", 15);
    ethernet_init(eth_obj);

    do {} while(true != eth_obj->ethernet_status_flag);
    strlcpy(server_cred->web_mount_point, "/dist", 6); 
    webpage_init(server_cred);
}

void led_config (void)
{
    gpio_reset_pin(LED_RED);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
}

void led_task(void * pvParameter)
{
    bool conn_status = false;
    ethernet_obj_t * eth_obj = (ethernet_obj_t *)pvParameter;

    while (1) 
    {
        if (true == conn_status)
        {
            // We are connected - LED on
            gpio_set_level(LED_RED, 1);
            vTaskDelay(3000);

            if (NULL != eth_obj)
            {
                vPortFree(eth_obj);
                eth_obj = NULL;
            }
        } 
        else 
        {
            conn_status = eth_obj->ethernet_status_flag;
            // We are connecting - blink fast
            gpio_set_level(LED_RED, 0);
            vTaskDelay(200);
            gpio_set_level(LED_RED, 1);
            vTaskDelay(200);
        }
    }
}
