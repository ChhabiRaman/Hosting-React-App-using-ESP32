#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_eth.h"

typedef struct
{
    char mdns_host_name[32];
    char mdns_instance_name[64];
}mdns_obj_t;

typedef struct
{
    EventGroupHandle_t wifi_event_group;
    uint8_t CONNECTED_BIT;
    uint8_t FAILED_BIT;
    bool wifi_status_flag;
    char ssid[32];
    char password[64];
    mdns_obj_t mdns_cred;
} wifi_obj_t;

typedef struct
{
    EventGroupHandle_t ethernet_event_group;
    uint8_t CONNECTED_BIT;
    uint8_t FAILED_BIT;
    bool ethernet_status_flag;
    esp_eth_handle_t s_eth_handle;
    esp_eth_mac_t * s_eth_mac;
    esp_eth_phy_t * s_eth_phy;
    esp_eth_netif_glue_handle_t s_eth_glue;
    mdns_obj_t mdns_cred;
}ethernet_obj_t;

void wifi_init (wifi_obj_t *);
void ethernet_init (ethernet_obj_t *);
void ethernet_shutdown (ethernet_obj_t *);
#endif