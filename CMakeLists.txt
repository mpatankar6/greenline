cmake_minimum_required(VERSION 4.1)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" APP_VERSION)
string(STRIP "${APP_VERSION}" APP_VERSION)
project(greenline
  VERSION ${APP_VERSION}
  LANGUAGES C
)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NVML REQUIRED IMPORTED_TARGET nvidia-ml)
pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncursesw)

add_executable(greenline
  src/main.c
  src/gpu.c
  src/tui.c
  src/circular_buffer.c
)

target_compile_definitions(greenline PRIVATE VERSION_STRING="${PROJECT_VERSION}")
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

target_link_libraries(greenline PRIVATE PkgConfig::NVML)
target_link_libraries(greenline PRIVATE PkgConfig::NCURSES)

install(TARGETS greenline DESTINATION bin)

get_target_property(SOURCES greenline SOURCES)
add_custom_target(analyze
  COMMAND clang-check -analyze -p ${CMAKE_BINARY_DIR} ${SOURCES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL # Allows colored output
)

enable_testing()
add_subdirectory(test)
