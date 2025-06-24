idf_component_register(
    INCLUDE_DIRS "include"
)

add_prebuilt_library(nertc_sdk "${CMAKE_CURRENT_SOURCE_DIR}/libs/${CONFIG_IDF_TARGET}/libnertc_sdk.a"
                    REQUIRES mbedtls lwip)
target_link_libraries(${COMPONENT_LIB} INTERFACE nertc_sdk)