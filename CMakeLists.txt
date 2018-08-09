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

add_library(examon_sync_plugin SHARED examon_sync_plugin.cpp examon_metric.cpp)
# taken from the corresponding wiki
target_link_libraries(examon_sync_plugin PRIVATE Scorep::scorep-plugin-cxx)

# try linking mosquitto
#target_link_libraries(examon_sync_plugin PRIVATE extern/mosquitto-1.5/cpp/libmosquittopp.so)
target_link_libraries(examon_sync_plugin PRIVATE libmosquitto)
# TODO: Make this better
target_link_libraries(examon_sync_plugin PRIVATE /home/jitschin/git/scorep_plugin_examon/build/extern/mosquitto-1.5/lib/cpp/libmosquittopp.so)
#target_link_libraries(examon_sync_plugin PRIVATE extern/mosquitto-1.5/lib)
#set_property(TARGET examon_sync_plugin PROPERTY IMPORTED_LOCATION "extern/mosquitto-1.5/cpp/libmosquittopp.so")
#set_property(TARGET examon_sync_plugin PROPERTY IMPORTED_LOCATION "extern/mosquitto-1.5/libmosquitto.so")


#target_include_directories(examon_sync_plugin PUBLIC extern/scorep_plugin_cxx_wrapper/include)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5/lib)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5/lib/cpp)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5/client)


# would only be required if using c-style malloc without casting
#SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive" )


#target_include_directories(examon_sync_plugin PUBLIC include)
#set(CMAKE_CXX_COMPILER /usr/bin/gcc -std=c++14)



install(TARGETS examon_sync_plugin
    LIBRARY DESTINATION lib)