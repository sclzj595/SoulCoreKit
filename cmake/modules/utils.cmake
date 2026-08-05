# utils.cmake — 工具函数模块 [v2.5.0]
# 依赖: core + Qt::Xml + nlohmann_json

add_library(soul_utils STATIC
    src/soul/utils/clipboard/clipboard_utils.cpp
    src/soul/utils/compress/compress_utils.cpp
    src/soul/utils/crypto/crypto_utils.cpp
    src/soul/utils/datetime/datetime_utils.cpp
    src/soul/utils/file/file_utils.cpp
    src/soul/utils/image/image_utils.cpp
    src/soul/utils/json/json_utils.cpp
    src/soul/utils/process/process_utils.cpp
    src/soul/utils/string/string_utils.cpp
    src/soul/utils/xml/xml_utils.cpp
)

target_include_directories(soul_utils PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_utils PUBLIC soul_core Qt6::Xml nlohmann_json::nlohmann_json)

if(ZLIB_FOUND)
    target_link_libraries(soul_utils PRIVATE ZLIB::ZLIB)
endif()