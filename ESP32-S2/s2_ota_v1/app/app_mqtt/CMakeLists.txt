idf_component_register(SRCS "app_mqtt.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_wifi esp_system nvs_flash esp_event esp_netif mqtt app_smartconfig app_ota json
                    )
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
