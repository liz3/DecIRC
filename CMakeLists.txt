cmake_minimum_required(VERSION 3.16)
project("DecIrc")
set(CMAKE_CXX_STANDARD 17)
set(WEBP_BUILD_ANIM_UTILS "Build animation utilities." OFF)
set(WEBP_BUILD_CWEBP "Build the cwebp command line tool." OFF)
set(WEBP_BUILD_DWEBP "Build the dwebp command line tool." OFF)
set(WEBP_BUILD_GIF2WEBP "Build the gif2webp conversion tool." OFF)
set(WEBP_BUILD_IMG2WEBP "Build the img2webp animation tool." OFF)
set(WEBP_BUILD_VWEBP "Build the vwebp viewer tool." OFF)
set(WEBP_BUILD_WEBPINFO "Build the webpinfo command line tool." OFF)
set(WEBP_BUILD_LIBWEBPMUX "Build the libwebpmux library." OFF)
set(WEBP_BUILD_WEBPMUX "Build the webpmux command line tool." OFF)
set(WEBP_BUILD_EXTRAS "Build extras." OFF)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RTAUDIO_BUILD_TESTING OFF)
set(USE_TLS ON)
set(ENABLE_TESTING OFF)
set(ENABLE_PROGRAMS OFF)
set(WEBP_BUILD_DWEBP OFF)
set(WEBP_BUILD_CWEBP OFF)
set(WEBP_BUILD_LIBWEBPMUX OFF)
set(WEBP_BUILD_GIF2WEBP OFF)
set(WEBP_BUILD_IMG2WEBP OFF)

set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON)
set(SKIP_INSTALL_ALL ON)
set(IXWEBSOCKET_INSTALL OFF)
set(MBEDTLS_VERSION_GREATER_THAN_3 ON)
set(FT_WITH_PNG ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DEC_SOURCES src/irc/irc_socket.cc 
  src/irc/irc_client.cc src/irc/irc_pool.cc 
  src/rendering/opengl_state.cc src/rendering/glad.c 
  src/rendering/font_atlas.cc src/gui_components.cc 
  src/AppState.cc src/rendering/text_box.cc 
  src/rendering/la.cc src/rendering/text.cc
  src/rendering/shader.cc src/components/search_list.cc 
  src/main.cc src/components/user_info.cc 
  src/components/user_list.cc src/components/channel_list.cc 
  src/components/text_field.cc src/components/message_list.cc 
  src/irc/irc_event_handler.cc src/irc/image_cache.cc 
  src/irc/message_state.cc src/irc/message_util.cc third-party/png/lodepng.cpp
  third-party/gifdec/gifdec.c src/components/image_overlay.cc 
  src/utils/notifications.cc src/utils/config.cc src/utils/web_util.cc src/utils/url_handler.cc src/utils/url_parser.cc)

if(WIN32)
  list(APPEND DEC_SOURCES
    third-party/wintoast/wintoastlib.cc
    )
endif()
if(APPLE)
  list(APPEND DEC_SOURCES
    src/utils/notifications_macos/MacNotifications.mm
    src/utils/notifications_macos/ProtocolHandler.mm
    )
endif()

option(DEC_LIZ_PNG_UPLOAD "Add the code for my personal png upload" OFF)

if(DEC_LIZ_PNG_UPLOAD)
   add_definitions(-DDEC_LIZ_PNG_UPLOAD)
endif()

add_executable(DecIrc ${DEC_SOURCES})

target_include_directories(DecIrc PRIVATE third-party/IXWebSocket/ixwebsocket)
target_include_directories(DecIrc PRIVATE third-party/json)
target_include_directories(DecIrc PRIVATE third-party/zlib ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib)

add_subdirectory(third-party/zlib)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib)
set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib)


add_subdirectory(third-party/lpng1639)
set(PNG_LIBRARIES png_static)
set(PNG_LIBRARY png_static)
set(PNG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lpng1639 ${CMAKE_CURRENT_BINARY_DIR}/third-party/lpng1639/)
set(PNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lpng1639 ${CMAKE_CURRENT_BINARY_DIR}/third-party/lpng1639/)
set(PNG_PNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/lpng1639 ${CMAKE_CURRENT_BINARY_DIR}/third-party/lpng1639/)


if(WIN32)
  set(OPENSSL_NO_ASM ON)
endif()
if(NOT APPLE)
  set(USE_OPEN_SSL ON)
add_subdirectory(third-party/boringssl)
endif()
# set(MBEDTLS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third-party/mbedtls/include)
# set(MBEDTLS_LIBRARY mbedtls)
# set(MBEDX509_LIBRARY mbedx509)
# set(MBEDCRYPTO_LIBRARY mbedcrypto)

add_subdirectory(third-party/libjpeg)
add_subdirectory(third-party/libwebp)
if(UNIX AND NOT APPLE)
    if(GLFW_USE_WAYLAND)
        set(GLFW_BUILD_X11 OFF)
        set(GLFW_BUILD_WAYLAND ON)
    else()
        set(GLFW_BUILD_X11 ON)
        set(GLFW_BUILD_WAYLAND OFF)
    endif()
endif()
add_subdirectory(third-party/glfw)
add_subdirectory(third-party/freetype2)
set(USE_ZLIB OFF)

set(OPENSSL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/boringssl/include)
set(OPENSSL_FOUND ON)
set(OPENSSL_LIBRARIES crypto ssl)

add_subdirectory(third-party/IXWebSocket)
if(APPLE)
  target_link_libraries(DecIrc PUBLIC "-framework Foundation -framework Cocoa -framework Carbon")
  add_definitions(-DDEC_MAC_PATH_BASE)
endif()
if(WIN32)
  target_link_libraries(DecIrc PUBLIC glfw freetype ixwebsocket webp libjpeg ws2_32 crypt32)
else()
  target_link_libraries(DecIrc PUBLIC glfw freetype ixwebsocket webp libjpeg)
endif()

if(UNIX AND NOT APPLE)
   find_package(PkgConfig REQUIRED) # Include functions provided by PkgConfig module.

 add_subdirectory(third-party/libnotify)
target_include_directories(DecIrc PRIVATE third-party/libnotify)
target_link_libraries(DecIrc PUBLIC libnotify)
  pkg_check_modules(GLIB REQUIRED glib-2.0) 
  target_include_directories(DecIrc PRIVATE ${GLIB_INCLUDE_DIRS})
    pkg_check_modules(PIXBUF REQUIRED gdk-pixbuf-2.0) 
target_include_directories(DecIrc PRIVATE ${PIXBUF_INCLUDE_DIRS})

endif()
file(COPY assets DESTINATION .)
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
