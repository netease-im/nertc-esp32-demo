# GIF表情组件

# 收集所有GIF源文件
if (CONFIG_USE_ANIM_EMOJI_NERTC)
    file(GLOB GIF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/nertc/*.c")
elseif (CONFIG_USE_ANIM_EMOJI_OTTO)
    file(GLOB GIF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/otto/*.c")
elseif (CONFIG_USE_ANIM_EYE_240X240_GIF1)
    file(GLOB GIF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/eye/gif1_240/*.c")
elseif (CONFIG_USE_ANIM_EYE_240X240_GIF2)
    file(GLOB GIF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/eye/gif2_240/*.c")
elseif (CONFIG_USE_ANIM_EYE_160X160_GIF1)
    file(GLOB GIF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/eye/gif1_160/*.c")
elseif (CONFIG_USE_ANIM_EYE_160X160_GIF2)
    file(GLOB GIF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/eye/gif2_160/*.c")
endif()

# 检查是否找到GIF源文件
list(LENGTH GIF_SOURCES GIF_COUNT)
if(GIF_COUNT EQUAL 0)
    message(WARNING "未找到GIF源文件，请确保src目录中包含对应资源的.c文件")
endif()
# 追加 src/ 根目录下的 .c 文件
file(GLOB EXTRA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# 合并两个列表
list(APPEND GIF_SOURCES ${EXTRA_SOURCES})

# 注册组件
idf_component_register(
    SRCS ${GIF_SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES "lvgl"
)