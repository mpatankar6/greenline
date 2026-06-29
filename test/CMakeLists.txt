function(add_unit_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.c ${ARGN})
    target_include_directories(${TEST_NAME} PRIVATE ../src)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_unit_test(test_circular_buffer ../src/circular_buffer.c)
