project("libnotify")
set(CMAKE_C_STANDARD 11)
add_library(libnotify SHARED ${CMAKE_CURRENT_SOURCE_DIR}/libnotify/notify.c 
	${CMAKE_CURRENT_SOURCE_DIR}/libnotify/notification.c)

add_library(libnotify_static STATIC ${CMAKE_CURRENT_SOURCE_DIR}/libnotify/notify.c 
	${CMAKE_CURRENT_SOURCE_DIR}/libnotify/notification.c)

 find_package(PkgConfig REQUIRED) # Include functions provided by PkgConfig module.
  pkg_check_modules(GLIB REQUIRED glib-2.0) 
    pkg_check_modules(GIO REQUIRED gio-2.0) 
    pkg_check_modules(PIXBUF REQUIRED gdk-pixbuf-2.0) 
target_include_directories(libnotify PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(libnotify INTERFACE ${GLIB_LDFLAGS})
target_include_directories(libnotify_static PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(libnotify_static INTERFACE ${GLIB_LDFLAGS})

target_include_directories(libnotify PRIVATE ${GIO_INCLUDE_DIRS})
target_link_libraries(libnotify INTERFACE ${GIO_LDFLAGS})
target_include_directories(libnotify_static PRIVATE ${GIO_INCLUDE_DIRS})
target_link_libraries(libnotify_static INTERFACE ${GIO_LDFLAGS})

target_include_directories(libnotify PRIVATE ${PIXBUF_INCLUDE_DIRS})
target_link_libraries(libnotify INTERFACE ${PIXBUF_LDFLAGS})
target_include_directories(libnotify_static PRIVATE ${PIXBUF_INCLUDE_DIRS})
target_link_libraries(libnotify_static INTERFACE ${PIXBUF_LDFLAGS})
