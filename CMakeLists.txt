set(PROJECT_NAME srgn_config)

cmake_minimum_required(VERSION 3.10)
project(${PROJECT_NAME})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()



# 在下面添加源代码文件，一行一个
add_executable(${PROJECT_NAME}
  src/main.c
  src/device.c
  src/uenv.c
  src/config_registry.c
  src/config_state.c
  src/ui.c


  anbui/pl_linux.c
  anbui/ad_ui.c
  anbui/ad_obj.c
  anbui/ad_text.c
  anbui/anbui.c
)

# 在下面添加头文件目录，一行一个
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/anbui
)

target_link_libraries(${PROJECT_NAME}
)

install(TARGETS ${PROJECT_NAME})