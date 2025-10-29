idf_component_register(
    # 列出所有文件，顺序无所谓
    SRCS "blufi_app.cc"
         "blufi_wifi.c"
         "blufi_init.c"
         "blufi_security.c"
         "ssid_manager.cc"

    INCLUDE_DIRS "."

    # 告诉构建系统需要 C++ 标准库
    REQUIRES bt esp_wifi mbedtls nvs_flash app_update
)

# # 如果 .cc 里用了 C++17/20 特性，可以指定标准
# set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)