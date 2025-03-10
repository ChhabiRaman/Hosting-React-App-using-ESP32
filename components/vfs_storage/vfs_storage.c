#include "esp_vfs_semihost.h"
#include "esp_littlefs.h"
#include "esp_vfs_fat.h"
#include "vfs_storage.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#if CONFIG_WEB_DEPLOY_SD
#include "driver/sdmmc_host.h"
#endif

#define LOG_TAG     "[vfs_storage]"

esp_err_t init_vfs(webpage_obj_t * server_cred)
{
    esp_err_t err_ret = ESP_FAIL;

    #if CONFIG_WEB_DEPLOY_SEMIHOST
    err_ret = esp_vfs_semihost_register(server_cred->web_mount_point);

    if (ESP_OK != err_ret) 
    {
        ESP_LOGE(LOG_TAG, "Failed to register semihost driver (%s)!", esp_err_to_name(err_ret));
    }
    #elif CONFIG_WEB_DEPLOY_SF
    size_t total = 0, used = 0;
    
    esp_vfs_littlefs_conf_t littlefs_conf = 
    {
        .base_path = NULL,
        .partition_label = "storage",
        .dont_mount = false,
        .format_if_mount_failed = true
    };

    littlefs_conf.base_path = server_cred->web_mount_point;
    err_ret = esp_vfs_littlefs_register(&littlefs_conf);

    if (err_ret != ESP_OK) 
    {
        if (err_ret == ESP_FAIL) 
        {
            ESP_LOGE(LOG_TAG, "Failed to mount or format filesystem");
        }
        else if (err_ret == ESP_ERR_NOT_FOUND) 
        {
            ESP_LOGE(LOG_TAG, "Failed to find LITTLEFS partition");
        }
        else 
        {
            ESP_LOGE(LOG_TAG, "Failed to initialize LITTLEFS (%s)", esp_err_to_name(err_ret));
        }
    }

    else
    {
        err_ret = esp_littlefs_info(littlefs_conf.partition_label, &total, &used);

        if (err_ret != ESP_OK) 
        {
            ESP_LOGE(LOG_TAG, "Failed to get LITTLEFS partition information (%s)", 
                        esp_err_to_name(err_ret));
        }
        else 
        {
            ESP_LOGI(LOG_TAG, "Partition size: total: %d, used: %d, base path: %s", 
                                    total, used, littlefs_conf.base_path);
        }
    }
    #elif CONFIG_WEB_DEPLOY_SD
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = true,
        .max_files = 20,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t * card;
    err_ret = esp_vfs_fat_sdmmc_mount(server_cred->web_mount_point, &host, 
                                                &slot_config, &mount_config, &card);
    if (err_ret != ESP_OK) 
    {
        if (err_ret == ESP_FAIL) 
        {
            ESP_LOGE(LOG_TAG, "Failed to mount filesystem.");
        }
        else 
        {
            ESP_LOGE(LOG_TAG, "Failed to initialize the card (%s)", esp_err_to_name(err_ret));
        }
    }
    else
    {
        ESP_LOGI(LOG_TAG, "Mount Successful.");
        /* print card info if mount successfully */
        sdmmc_card_print_info(stdout, card);

        DIR * dir = opendir(server_cred->web_mount_point);

        if (dir == NULL) 
        {
            ESP_LOGE("SD", "Failed to open directory for listing.");
        }
        else
        {
            struct dirent * entry;
            while ((entry = readdir(dir)) != NULL) 
            {
                ESP_LOGI(LOG_TAG, "Found file: %s", entry->d_name);
            }
        
            closedir(dir);
        }
    }
    #endif
    return err_ret;
}