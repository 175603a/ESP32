idf_component_register(SRCS "app_smartconfig.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi nvs_flash wpa_supplicant
                    PRIV_REQUIRES main
                    )