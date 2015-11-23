cmake_minimum_required(VERSION 2.8.4)
project(media-endpoint)

include(../cmake/mingw.cmake)
#include(../cmake/darwin.cmake)
include(../cmake/utils.cmake)

set(SRCS 
    src/endpoint.c 
    ../common/src/my-pjlib-utils.c 
    ../common/src/my-pjmedia-utils.c)

set(RECEIVER_SRCS 
    test/app-receiver.c 
    ${SRCS})

set(STREAMER_SRCS 
    test/app-streamer.c 
    ${SRCS})

set(INC_DIRS 
    include 
    ../common/include)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(EXT ".exe")
endif()

my_find_pjproject()

set(INC_DIRS include ../common/include ${PJPROJECT_INCLUDE_DIRS})
add_definitions(${PJPROJECT_CFLAGS})
include_directories(${INC_DIRS})

set(RECEIVER app-receiver${EXT})
add_executable(${RECEIVER} ${RECEIVER_SRCS})

set(STREAMER app-streamer${EXT})
add_executable(${STREAMER} ${STREAMER_SRCS})

my_target_link_pjproject(${RECEIVER})
my_target_link_pjproject(${STREAMER})


set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
install(TARGETS ${STREAMER} ${RECEIVER} RUNTIME DESTINATION output)

set(CPACK_GENERATOR "ZIP")
include(CPack)
