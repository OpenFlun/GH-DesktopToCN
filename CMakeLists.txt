# CMakeList.txt: GitHubDesktopToCN 的 CMake 项目，仅支持 x64 构建
#
cmake_minimum_required (VERSION 3.30)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

add_compile_options(/utf-8)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(/bigobj)
    message("当前是以Debug模式编译")
endif()

message("当前是以x64模式编译")
add_definitions(-DCURRENT_PLATFORM_ISX64)
set(CURRENT_PLATFORM_ISX64 1)

if(NOT DEFINED FILE_VERSION)
    if(EXISTS "${CMAKE_SOURCE_DIR}/version.json")
        file(READ "${CMAKE_SOURCE_DIR}/version.json" VER_FILE_CONTENT)
        string(REGEX MATCH "\"version\"[ \t]*:[ \t]*\"([0-9]+\.[0-9]+\.[0-9]+)\"" _match "${VER_FILE_CONTENT}")
        if(_match)
            set(FILE_VERSION "${CMAKE_MATCH_1}")
        else()
            set(FILE_VERSION "0.0.0-Dev.0")
        endif()
    else()
        set(FILE_VERSION "0.0.0-Dev.0")
    endif()
endif()
add_definitions(-DFILEVERSION="${FILE_VERSION}")
message("程序内嵌版本: ${FILE_VERSION}")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

project ("GitHubDesktopToCN")

# 永久本地化 FetchContent 下载目录
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.fetched" CACHE PATH "Directory where FetchContent downloads are stored")

# 添加 Qt 支持（x64），需要 Network 用于检查更新
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)
include_directories(include)
include_directories(Utils)
include_directories(openssl-proj/include)

# ====================== 预汉化文件嵌入开始 ======================
# 自动收集 prebuilt 目录下的所有文件（包括 localization.json）
file(GLOB_RECURSE PREBUILT_RESOURCE_FILES
     RELATIVE ${CMAKE_SOURCE_DIR}
     ${CMAKE_SOURCE_DIR}/prebuilt/*)
list(TRANSFORM PREBUILT_RESOURCE_FILES REPLACE "\\\\" "/")
list(SORT PREBUILT_RESOURCE_FILES)

# 直接写入 resources.qrc，不需要 .in 文件
set(QRC_PATH ${CMAKE_SOURCE_DIR}/resources.qrc)
file(WRITE ${QRC_PATH} "<RCC>\n    <qresource prefix=\"/\">\n")
foreach(file ${PREBUILT_RESOURCE_FILES})
    file(APPEND ${QRC_PATH} "        <file>${file}</file>\n")
endforeach()
file(APPEND ${QRC_PATH} "    </qresource>\n</RCC>\n")

# 收集版本号（子目录名）
file(GLOB PREBUILT_VERSION_DIRS
     LIST_DIRECTORIES true
     RELATIVE ${CMAKE_SOURCE_DIR}/prebuilt
     ${CMAKE_SOURCE_DIR}/prebuilt/*)
set(PREBUILT_VERSIONS "")
foreach(dir ${PREBUILT_VERSION_DIRS})
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/prebuilt/${dir})
        list(APPEND PREBUILT_VERSIONS ${dir})
    endif()
endforeach()
list(SORT PREBUILT_VERSIONS)
string(REPLACE ";" "\\\;" PREBUILT_VERSIONS_STR "${PREBUILT_VERSIONS}")
add_definitions(-DPREBUILT_VERSIONS="${PREBUILT_VERSIONS_STR}")
# ====================== 预汉化文件嵌入结束 ======================

add_executable (GitHubDesktopToCN
    "MainWindow.cpp"
    "MainWindow.h"
    ${CMAKE_SOURCE_DIR}/resources.qrc
)

set_target_properties(GitHubDesktopToCN PROPERTIES WIN32_EXECUTABLE TRUE)

# 链接 OpenSSL 导入库（.lib 文件，用于链接；运行时需要对应的 .dll）
target_link_libraries(GitHubDesktopToCN PRIVATE
    ${CMAKE_SOURCE_DIR}/openssl-proj/lib/libssl.lib
    ${CMAKE_SOURCE_DIR}/openssl-proj/lib/libcrypto.lib)

# 链接 Qt
target_link_libraries(GitHubDesktopToCN PRIVATE Qt6::Widgets Qt6::Network)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GitHubDesktopToCN PROPERTY CXX_STANDARD 20)
endif()

# 安装 OpenSSL 动态库（从项目目录或 Qt 目录）
if(WIN32)
    # 优先使用项目内的 DLL
    if(EXISTS "${CMAKE_SOURCE_DIR}/openssl-proj/lib/libcrypto-3-x64.dll" AND EXISTS "${CMAKE_SOURCE_DIR}/openssl-proj/lib/libssl-3-x64.dll")
        install(FILES
            ${CMAKE_SOURCE_DIR}/openssl-proj/lib/libcrypto-3-x64.dll
            ${CMAKE_SOURCE_DIR}/openssl-proj/lib/libssl-3-x64.dll
            DESTINATION .)
        message(STATUS "已配置从项目 openssl-proj/lib 安装 OpenSSL DLL")
    else()
        # 后备：从 Qt bin 目录查找
        find_file(LIB_CRYPTO_DLL "libcrypto-3-x64.dll" HINTS "${QT6_INSTALL_PREFIX}/bin")
        find_file(LIB_SSL_DLL    "libssl-3-x64.dll"    HINTS "${QT6_INSTALL_PREFIX}/bin")
        if(LIB_CRYPTO_DLL AND LIB_SSL_DLL)
            install(FILES ${LIB_CRYPTO_DLL} ${LIB_SSL_DLL} DESTINATION .)
            message(STATUS "已配置从 Qt bin 目录安装 OpenSSL DLL")
        else()
            message(WARNING "未找到 OpenSSL DLL，HTTPS 可能不可用；请手动将 libcrypto-3-x64.dll 和 libssl-3-x64.dll 放置于可执行文件目录")
        endif()
    endif()
endif()

install(TARGETS GitHubDesktopToCN RUNTIME DESTINATION .)