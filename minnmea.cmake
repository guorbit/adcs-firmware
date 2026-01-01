# Add library cpp files

if (NOT DEFINED MINNMEA_PATH)
    set(MINNMEA_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/libnmea")
endif()


add_library(minnmea STATIC)

target_sources(minnmea PUBLIC
	${MINNMEA_PATH}/minmea.c
)


# Add include directory
target_include_directories(minnmea PUBLIC 
    ${MINNMEA_PATH}
    )

target_link_libraries(minnmea PUBLIC 
    pico_stdlib
    )


target_compile_definitions(minnmea PUBLIC
	timegm=mktime
)