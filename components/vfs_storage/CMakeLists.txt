idf_component_register(SRCS "vfs_storage.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES webpage littlefs fatfs)

if(CONFIG_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/web-demo")
    if(EXISTS ${WEB_SRC_DIR}/dist)
    littlefs_create_partition_image(storage ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
    else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
    endif()
endif()
