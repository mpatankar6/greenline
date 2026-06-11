cmake_minimum_required(VERSION 4.1)

project(greenline
  VERSION 0.0.1
  LANGUAGES C
)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NVML REQUIRED nvidia-ml)

add_executable(greenline
  src/main.c
  src/gpu.c
)

target_compile_features(greenline PRIVATE c_std_23)
target_compile_options(greenline PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  $<$<CONFIG:Debug>:-g -O1 -fsanitize=address,undefined>
  $<$<CONFIG:Release>:-O3>
)
target_link_options(greenline PRIVATE
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

target_include_directories(greenline PRIVATE ${NVML_INCLUDE_DIRS})
target_link_libraries(greenline PRIVATE ${NVML_LINK_LIBRARIES})

install(TARGETS greenline DESTINATION bin)

get_target_property(SOURCES greenline SOURCES)
add_custom_target(analyze
  COMMAND clang-check -analyze -p ${CMAKE_BINARY_DIR} ${SOURCES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL # Allows colored output
)
