cmake_minimum_required(VERSION 3.0)

project(examon_plugin)

add_subdirectory(extern/scorep_plugin_cxx_wrapper)
add_subdirectory(extern/mosquitto-1.5)


include(common/FindScorep.cmake)
if(SCOREP_FOUND)
    include_directories(${SCOREP_INCLUDE_DIRS})
    link_directories(${SCOREP_LIBRARY_DIRS})
    SET(CMAKE_INSTALL_PREFIX ${SCOREP_ROOT_PATH} CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
else()
    message(SEND_ERROR "Scorep was not found but is required")
endif()


# library stuff
add_library(examon_sync_plugin SHARED examon_sync_plugin)
target_link_libraries(examon_sync_plugin PUBLIC scorep-plugin-cxx)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5/lib)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5/client)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5)

#target_include_directories(examon_sync_plugin PUBLIC include)
#set(CMAKE_CXX_COMPILER /usr/bin/gcc -std=c++14)


#include_directories(include)


install(TARGETS examon_sync_plugin
    LIBRARY DESTINATION lib)