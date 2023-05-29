# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(SRCS "app_ota.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_event app_update esp_http_client esp_https_ota nvs_flash
                    EMBED_TXTFILES ${project_dir}/app/app_ota/server_certs/ca_cert.pem)
