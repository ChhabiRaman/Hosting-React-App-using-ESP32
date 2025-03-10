#include "esp_tls_crypto.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "vfs_storage.h"
#include "esp_vfs.h"
#include "webpage.h"
#include "esp_log.h"
#include "fcntl.h"
#include "cJSON.h"

#define LOG_TAG		    "[webpage app]"
#define ENABLE_AUTH     0 
#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], \
                                                ext) == 0)

// Variables

#if ENABLE_AUTH
typedef struct 
{
    char * username;
    char * password;
} basic_auth_info_t;
#endif

//Functions
esp_err_t stop_webserver(httpd_handle_t server);
esp_err_t basic_auth_get_handler(httpd_req_t * req);
esp_err_t system_info_get_handler(httpd_req_t * req);
esp_err_t rest_common_get_handler (httpd_req_t * req);
esp_err_t temperature_data_get_handler(httpd_req_t * req);
esp_err_t light_brightness_post_handler(httpd_req_t * req);
void httpd_register_basic_auth(httpd_handle_t server_handle);
char * encrypt_auth_credentials(const char * username, const char * password);
esp_err_t set_content_type_from_file(httpd_req_t * req, const char * filepath);

void webpage_init(webpage_obj_t * server_cred)
{
    esp_err_t err_ret = init_vfs(server_cred);

    if (ESP_OK == err_ret)
    {
        httpd_handle_t server_handle = NULL;
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.uri_match_fn = httpd_uri_match_wildcard;
        // config.lru_purge_enable = true;
    
        if (ESP_OK == httpd_start(&server_handle, &config)) 
        {
            // Set URI handlers
            /* URI handler for fetching system info */
            httpd_uri_t system_info_get_uri = webpage_handler("/api/v1/system/info", HTTP_GET,
                                                                system_info_get_handler, server_cred);
            /* URI handler for fetching temperature data */
            httpd_uri_t temperature_data_get_uri = webpage_handler("/api/v1/temp/raw", HTTP_GET,
                                                            temperature_data_get_handler, server_cred);
            /* URI handler for light brightness control */
            httpd_uri_t light_brightness_post_uri = webpage_handler("/api/v1/light/brightness", HTTP_POST,
                                                                light_brightness_post_handler, server_cred);
            /* URI handler for getting web server files */
            httpd_uri_t common_get_uri = webpage_handler("/*", HTTP_GET, rest_common_get_handler, 
                                                            server_cred);
    
            httpd_register_uri_handler(server_handle, &system_info_get_uri);
            httpd_register_uri_handler(server_handle, &temperature_data_get_uri);
            httpd_register_uri_handler(server_handle, &light_brightness_post_uri);
            httpd_register_uri_handler(server_handle, &common_get_uri);
            #if ENABLE_AUTH
            httpd_register_basic_auth(server_handle);
            #endif
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Error starting server!");
        }
    }

    else
    {
        ESP_LOGE(LOG_TAG, "Error initializing VFS storage: %s", esp_err_to_name(err_ret));
    }
}

// Stop the httpd server
esp_err_t stop_webserver(httpd_handle_t server)
{
    return httpd_stop(server);
}

/* Handler for getting system handler */
esp_err_t system_info_get_handler(httpd_req_t * req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Send HTTP response with the contents of the requested file */
esp_err_t rest_common_get_handler(httpd_req_t * req)
{
    char filepath[ESP_VFS_PATH_MAX + 128];
    webpage_obj_t * server_context = (webpage_obj_t *)req->user_ctx;

    strlcpy(filepath, server_context->web_mount_point, sizeof(filepath));

    if (req->uri[strlen(req->uri) - 1] == '/') 
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else 
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);

    if (fd == -1) 
    {
        ESP_LOGE(LOG_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);
    char * chunk = server_context->scratch;
    ssize_t read_bytes;

    do 
    {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, sizeof(server_context->scratch));

        if (read_bytes == -1) 
        {
            ESP_LOGE(LOG_TAG, "Failed to read file : %s", filepath);
        }
        else if (read_bytes > 0) 
        {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) 
            {
                close(fd);
                ESP_LOGE(LOG_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);

    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(LOG_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Handler for getting temperature data */
esp_err_t temperature_data_get_handler(httpd_req_t * req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "raw", esp_random() % 20);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Simple handler for light brightness control */
esp_err_t light_brightness_post_handler(httpd_req_t * req)
{
    webpage_obj_t * server_context = (webpage_obj_t *)req->user_ctx;
    int total_len = req->content_len;
    int cur_len = 0;
    char * buf = server_context->scratch;
    int received = 0;

    if (total_len >= sizeof(server_context->scratch)) 
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);

        if (received <= 0) 
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, 
                                    "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int red = cJSON_GetObjectItem(root, "red")->valueint;
    int green = cJSON_GetObjectItem(root, "green")->valueint;
    int blue = cJSON_GetObjectItem(root, "blue")->valueint;
    ESP_LOGI(LOG_TAG, "Light control: red = %d, green = %d, blue = %d", red, green, blue);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

/* Set HTTP response content type according to file extension */
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char * type = "text/plain";

    if (CHECK_FILE_EXTENSION(filepath, ".html")) 
    {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js")) 
    {
        type = "application/javascript";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css")) 
    {
        type = "text/css";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png")) 
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico")) 
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg")) 
    {
        type = "text/xml";
    }

    return httpd_resp_set_type(req, type);
}

#if ENABLE_AUTH
void httpd_register_basic_auth(httpd_handle_t server_handle)
{
    basic_auth_info_t * basic_auth_info = calloc(1, sizeof(basic_auth_info_t));

    if (basic_auth_info) 
    {
        basic_auth_info->username = "chhabi9585";
        basic_auth_info->password = "Adhikari9585@";
        httpd_uri_t basic_auth = webpage_handler("/", HTTP_GET, basic_auth_get_handler, 
                                                    basic_auth_info);
        httpd_register_uri_handler(server_handle, &basic_auth);
    }

    else
    {
        ESP_LOGE(LOG_TAG, "Error in registering authorization");
    }
}

/* An HTTP GET handler */
esp_err_t basic_auth_get_handler(httpd_req_t * req)
{
    char * buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t * basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) 
    {
        buf = calloc(1, buf_len);

        if (!buf) 
        {
            ESP_LOGE(LOG_TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) 
        {
            ESP_LOGI(LOG_TAG, "Found header => Authorization: %s", buf);
        }
        else 
        {
            ESP_LOGE(LOG_TAG, "No auth value received");
        }

        char * auth_credentials = encrypt_auth_credentials(basic_auth_info->username, 
                                                            basic_auth_info->password);
        
        if (!auth_credentials) 
        {
            ESP_LOGE(LOG_TAG, "No enough memory for basic authorization credentials");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len)) 
        {
            ESP_LOGE(LOG_TAG, "Not authenticated");
            httpd_resp_set_status(req, "401 UNAUTHORIZED");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
        }
        else 
        {
            ESP_LOGI(LOG_TAG, "Authenticated!");
            char *basic_auth_resp = NULL;
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            int rc = asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", 
                                                    basic_auth_info->username);
            
            if (rc < 0) 
            {
                ESP_LOGE(LOG_TAG, "asprintf() returned: %d", rc);
                free(auth_credentials);
                return ESP_FAIL;
            }

            if (!basic_auth_resp) 
            {
                ESP_LOGE(LOG_TAG, "No enough memory for basic authorization response");
                free(auth_credentials);
                free(buf);
                return ESP_ERR_NO_MEM;
            }
            httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            free(basic_auth_resp);
        }
        free(auth_credentials);
        free(buf);
    }
    else 
    {
        ESP_LOGE(LOG_TAG, "No auth header received");
        httpd_resp_set_status(req, "401 UNAUTHORIZED");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

char * encrypt_auth_credentials (const char * username, const char * password)
{
    size_t out;
    char * user_info = NULL;
    char * digest = NULL;
    size_t n = 0;
    int rc = asprintf(&user_info, "%s:%s", username, password);

    if (rc < 0) 
    {
        ESP_LOGE(LOG_TAG, "asprintf() returned: %d", rc);
        return NULL;
    }

    if (!user_info) 
    {
        ESP_LOGE(LOG_TAG, "No enough memory for user information");
        return NULL;
    }

    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);

    if (digest) 
    {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, &out, 
                                    (const unsigned char *)user_info, strlen(user_info));
    }

    free(user_info);
    return digest;
}
#endif

inline httpd_uri_t webpage_handler (const char * p_uri, httpd_method_t e_method, 
                                        http_uri_handler fp_handler, void * p_user_ctx)
{
	httpd_uri_t s_httpd_uri;
	s_httpd_uri.uri      = p_uri;
	s_httpd_uri.method   = e_method;
	s_httpd_uri.handler  = fp_handler;
	s_httpd_uri.user_ctx = p_user_ctx;

	return s_httpd_uri;
}
