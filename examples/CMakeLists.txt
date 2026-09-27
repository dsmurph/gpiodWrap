


file(GLOB EASY_EXAMPLES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(SOURCE_FILE ${EASY_EXAMPLES_SOURCES})
    
    get_filename_component(SOURCE_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${SOURCE_NAME} ${SOURCE_FILE})
    
    target_link_libraries(${SOURCE_NAME}
        ${GPIOD_LIBRARY}
        gpiodWrap
    )
 
    set_target_properties(${SOURCE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        ${GPIODWRAP_BIN}/${SOURCE_NAME}
    )
    
endforeach()
