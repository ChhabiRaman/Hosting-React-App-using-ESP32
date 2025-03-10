/*|------------------------------------------------------------------------|*/
/*|WiFi connection in STA mode for ESP32 under ESP-IDF - Wokwi simulator   |*/
/*|Edited by: Chhabi                                                       |*/
/*|------------------------------------------------------------------------|*/

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "string.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "network.h"

#define LOG_TAG		"[network]"
#if CONFIG_LWIP_IPV6
/* types of ipv6 addresses to be displayed on ipv6 events */
const char * ipv6_addr_types_to_str[6] = 
{
    "ESP_IP6_ADDR_IS_UNKNOWN",
    "ESP_IP6_ADDR_IS_GLOBAL",
    "ESP_IP6_ADDR_IS_LINK_LOCAL",
    "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
    "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
};
#endif

// Functions declaration
void network_app_init (void);
void initialise_mdns (mdns_obj_t *);
void wifi_status_task (void * pvParameter);
void wifi_event_handler (void * arg, esp_event_base_t event_base, int32_t event_id,
                                void * event_data);
void ethernet_event_handler (void * arg, esp_event_base_t event_base,int32_t event_id, 
                                void * event_data);

//Variables declaration

void network_app_init (void)
{
    // Initialize NVS
    esp_err_t err_ret = nvs_flash_init();

    if (err_ret == ESP_ERR_NVS_NO_FREE_PAGES || err_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err_ret = nvs_flash_init();
    }

    if(ESP_OK == err_ret)
    {
        // Initialize the TCP/IP stack
        esp_netif_init();
        // Create the default event loop
        err_ret = esp_event_loop_create_default();
    }
    else
    {
        ESP_LOGE(LOG_TAG, "Failed to initialize NVS Flash");
    }
}

void wifi_init (wifi_obj_t * wifi_obj)
{
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    // Initialize wifi object
    wifi_obj->CONNECTED_BIT = BIT0;
    wifi_obj->FAILED_BIT = BIT1;
    wifi_obj->wifi_status_flag = false;

    network_app_init();
    initialise_mdns(&wifi_obj->mdns_cred);
    netbiosns_init();
    netbiosns_set_name(wifi_obj->mdns_cred.mdns_host_name);
    // Create the default Wi-Fi station
    esp_netif_create_default_wifi_sta();
    // Create the event group to handle Wi-Fi events
    wifi_obj->wifi_event_group = xEventGroupCreate();
    // Initialize the Wi-Fi driver
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err_ret  = esp_wifi_init(&wifi_init_config);

    if (ESP_OK == err_ret)
    {
        // Register event handlers
        esp_event_handler_instance_register(WIFI_EVENT,
                        ESP_EVENT_ANY_ID, &wifi_event_handler,
                        wifi_obj, NULL);
        esp_event_handler_instance_register(IP_EVENT,
                        IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                        wifi_obj, NULL);
        // Configure Wi-Fi connection settings
        memcpy(wifi_config.sta.ssid, wifi_obj->ssid, strlen(wifi_obj->ssid));
        memcpy(wifi_config.sta.password, wifi_obj->password, strlen(wifi_obj->password));
        // Set Wi-Fi mode to STA (Station)
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        // Start Wi-Fi
        err_ret = esp_wifi_start();

        if (ESP_OK != err_ret)
        {
            ESP_LOGE(LOG_TAG, "Failed to start WiFi");
        }
        xTaskCreate(&wifi_status_task, "connect_status_task", 2048, wifi_obj, 5, NULL);
    }
    else
    {
        ESP_LOGE(LOG_TAG, "Failed to Initialize WiFi");
    }
}

// Main task
void wifi_status_task (void * pvParameter) 
{
    wifi_obj_t * wifi_obj = (wifi_obj_t *)pvParameter;
    /* Waiting until either the connection is established (CONNECTED_BIT) or connection failed for
    * the maximum number of re-tries (FAILED_BIT). The bits are set by wifi_event_handler() */
    EventBits_t bits = xEventGroupWaitBits(wifi_obj->wifi_event_group, wifi_obj->CONNECTED_BIT 
                                        | wifi_obj->FAILED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which
     * event actually happened. */
    if (bits & wifi_obj->CONNECTED_BIT) 
    {
        wifi_obj->wifi_status_flag = true;
        ESP_LOGI(LOG_TAG, "connected to SSID:%s", wifi_obj->ssid);
        // Get and print the local IP address
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
        printf("IP Address:  " IPSTR "\n", IP2STR(&ip_info.ip));
        printf("Subnet mask: " IPSTR "\n", IP2STR(&ip_info.netmask));
        printf("Gateway:     " IPSTR "\n", IP2STR(&ip_info.gw));
    }
    else if (bits & wifi_obj->FAILED_BIT) 
    {
        ESP_LOGI(LOG_TAG, "Failed to connect to SSID:%s", wifi_obj->ssid);
    }
    else
    {
        ESP_LOGE(LOG_TAG, "UNEXPECTED EVENT");
    }

    while (1) 
    {
        vTaskDelete(NULL);
    }
}

// WiFi event handler
void wifi_event_handler (void * arg, esp_event_base_t event_base,
                               int32_t event_id, void * event_data) 
{
    wifi_obj_t * wifi_obj = (wifi_obj_t *)arg; 

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        xEventGroupClearBits(wifi_obj->wifi_event_group, wifi_obj->CONNECTED_BIT);
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        xEventGroupSetBits(wifi_obj->wifi_event_group, wifi_obj->CONNECTED_BIT);
    }
}

void ethernet_init (ethernet_obj_t * eth_obj)
{
    network_app_init();
    initialise_mdns(&eth_obj->mdns_cred);
    netbiosns_init();
    netbiosns_set_name(eth_obj->mdns_cred.mdns_host_name);

    // Initialize ethernet object
    eth_obj->CONNECTED_BIT = BIT0;
    eth_obj->FAILED_BIT = BIT1;
    eth_obj->ethernet_status_flag = false;
    // Create the event group to handle Ethernet events
    eth_obj->ethernet_event_group = xEventGroupCreate();
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    // Warning: the interface desc is used in tests to capture actual connection
    //  details (IP, gw, mask)
    esp_netif_config.if_desc = "netif_eth";
    esp_netif_config.route_prio = 64;
    esp_netif_config_t netif_config = 
    {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    esp_netif_t * p_netif = esp_netif_new(&netif_config);
    assert(p_netif);

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.rx_task_stack_size = 2048;
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = -1;
    phy_config.reset_gpio_num = -1;
    //Only for OpenCores Ethernet MAC (for use with QEMU) and does not run on real chip
    phy_config.autonego_timeout_ms = 100;
    eth_obj->s_eth_mac = esp_eth_mac_new_openeth(&mac_config);
    eth_obj->s_eth_phy = esp_eth_phy_new_dp83848(&phy_config);
    // Install Ethernet driver
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(eth_obj->s_eth_mac, eth_obj->s_eth_phy);
    esp_err_t err_ret = esp_eth_driver_install(&eth_config, &eth_obj->s_eth_handle);

    if (ESP_OK == err_ret)
    {
        uint8_t eth_mac[6] = {0};
        esp_read_mac(eth_mac, ESP_MAC_ETH);
        esp_eth_ioctl(eth_obj->s_eth_handle, ETH_CMD_S_MAC_ADDR, eth_mac);
        // combine driver with netif
        eth_obj->s_eth_glue = esp_eth_new_netif_glue(eth_obj->s_eth_handle);
        esp_netif_attach(p_netif, eth_obj->s_eth_glue);
        // Register user defined event handers
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &ethernet_event_handler, eth_obj);
    #ifdef CONFIG_LWIP_IPV6
        esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED, 
                                        &ethernet_event_handler, p_netif);
        esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &ethernet_event_handler, eth_obj);
    #endif
        esp_eth_start(eth_obj->s_eth_handle);
        xEventGroupWaitBits(eth_obj->ethernet_event_group, 
                                    eth_obj->CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    }
    else
    {
        ESP_LOGE(LOG_TAG, "Failed to install ethernet driver");
    }
}

// WiFi event handler
void ethernet_event_handler (void * arg, esp_event_base_t event_base,
                                int32_t event_id, void * event_data) 
{
    ethernet_obj_t * eth_obj = NULL;

    if (ETHERNET_EVENT_CONNECTED != event_id)
    {
        eth_obj = (ethernet_obj_t *)arg;
    }

    if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED) 
    {
        ethernet_shutdown(eth_obj);
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) 
    {
        ip_event_got_ip_t * event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(LOG_TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, 
                    esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(eth_obj->ethernet_event_group, eth_obj->CONNECTED_BIT);
        eth_obj->ethernet_status_flag = true;
    }
    #if CONFIG_LWIP_IPV6
    else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) 
    {
        ip_event_got_ip6_t * event = (ip_event_got_ip6_t *)event_data;
        esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
        ESP_LOGI(LOG_TAG, "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s", 
                            esp_netif_get_desc(event->esp_netif), IPV62STR(event->ip6_info.ip),
                            ipv6_addr_types_to_str[ipv6_type]);

        if (ipv6_type == ESP_IP6_ADDR_IS_LINK_LOCAL) 
        {
            xEventGroupSetBits(eth_obj->ethernet_event_group, eth_obj->CONNECTED_BIT);
            eth_obj->ethernet_status_flag = true;
        }
    }
    else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED) 
    {
        ESP_LOGI(LOG_TAG, "Ethernet Link Up");
        ESP_ERROR_CHECK(esp_netif_create_ip6_linklocal(arg));
    }
    #endif
}

/* tear down connection, release resources */
void ethernet_shutdown (ethernet_obj_t * eth_obj)
{
    esp_eth_stop(eth_obj->s_eth_handle);
    esp_eth_del_netif_glue(eth_obj->s_eth_glue);
    esp_eth_driver_uninstall(eth_obj->s_eth_handle);
    // esp_netif_destroy(eth_netif);
    esp_netif_deinit();
    eth_obj->s_eth_handle = NULL;
    eth_obj->s_eth_phy->del(eth_obj->s_eth_phy);
    eth_obj->s_eth_mac->del(eth_obj->s_eth_mac);

    esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, &ethernet_event_handler);
    #if CONFIG_LWIP_IPV6
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &ethernet_event_handler);
        esp_event_handler_unregister(ETH_EVENT, ETHERNET_EVENT_CONNECTED, &ethernet_event_handler);
    #endif
    esp_event_loop_delete_default();
    ESP_LOGI(LOG_TAG, "Ethernet Shut Down");
}

void initialise_mdns(mdns_obj_t * mdns_cred)
{
    mdns_init();
    mdns_hostname_set(mdns_cred->mdns_host_name);
    mdns_instance_name_set(mdns_cred->mdns_instance_name);

    mdns_txt_item_t serviceTxtData[] = 
    {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}
