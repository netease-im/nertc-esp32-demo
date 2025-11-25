# GIF表情组件

# 收集所有GIF源文件
if (CONFIG_USE_ANIM_EMOJI_NERTC1)
    file(GLOB LZ4_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/eye1/*.lz4")
elseif (CONFIG_USE_ANIM_EMOJI_NERTC2)
    file(GLOB LZ4_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/eye2/*.lz4")
elseif (CONFIG_USE_ANIM_EMOJI_NERTC3)
    file(GLOB LZ4_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/eye3/*.lz4")
elseif (CONFIG_USE_ANIM_EMOJI_NERTC2_160)
    file(GLOB LZ4_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/eye2_160/*.lz4")
elseif (CONFIG_USE_ANIM_EMOJI_NERTC3_160)
    file(GLOB LZ4_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/eye3_160/*.lz4")
elseif (CONFIG_USE_ANIM_EMOJI_OTTO)
    file(GLOB LZ4_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/lz4/otto/*.lz4")
endif()

# 检查是否找到GIF源文件
list(LENGTH LZ4_FILES GIF_COUNT)
if(GIF_COUNT EQUAL 0)
    message(WARNING "未找到GIF源文件，请确保src目录中包含对应资源的文件")
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
    EMBED_FILES ${LZ4_FILES}
)