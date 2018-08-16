cmake_minimum_required(VERSION 3.8)

project(examon_plugin)

execute_process(
  COMMAND 
    "git" "submodule" "update" "--init" "--recursive"
  WORKING_DIRECTORY
  "${CMAKE_SOURCE_DIR}"
  RESULT_VARIABLE result
  ERROR_VARIABLE error
)
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

add_library(examon_sync_plugin SHARED examon_sync_plugin.cpp examon_metric.cpp examon_mqtt_path.cpp)
# taken from the corresponding wiki
target_link_libraries(examon_sync_plugin PRIVATE Scorep::scorep-plugin-cxx libmosquitto mosquittopp)
target_include_directories(examon_sync_plugin PUBLIC extern/mosquitto-1.5/lib extern/mosquitto-1.5/lib/cpp extern/mosquitto-1.5/client)

add_library(examon_async_plugin SHARED examon_async_plugin.cpp examon_metric.cpp examon_mqtt_path.cpp)
target_link_libraries(examon_async_plugin PRIVATE Scorep::scorep-plugin-cxx libmosquitto mosquittopp)
target_include_directories(examon_async_plugin PUBLIC extern/mosquitto-1.5/lib extern/mosquitto-1.5/lib/cpp extern/mosquitto-1.5/client)
#target_include_directories(examon_async_plugin PUBLIC extern/scorep_plugin_cxx_wrapper/include)


# would only be required if using c-style malloc without casting
#SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive" )


#target_include_directories(examon_sync_plugin PUBLIC include)
#set(CMAKE_CXX_COMPILER /usr/bin/gcc -std=c++14)



install(TARGETS examon_sync_plugin
    LIBRARY DESTINATION lib)
install(TARGETS examon_async_plugin
    LIBRARY DESTINATION lib)