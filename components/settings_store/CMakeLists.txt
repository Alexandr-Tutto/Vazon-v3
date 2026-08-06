idf_component_register(
    SRCS "vazon_settings_store.c"
    INCLUDE_DIRS "include"
    REQUIRES climate_module fan_module humidifier_module light_module
             pot_module runtime_core
    PRIV_REQUIRES esp_hw_support nvs_flash
)
