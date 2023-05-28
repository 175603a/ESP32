idf_component_register(SRCS "smartconfig.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_event wpa_supplicant)