
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
include_directories("../model")
include_directories("../geom")
set(IO_SRCS         "ModelWriter.cpp"
                    "ModelReader.cpp"
)

ADD_LIBRARY(io ${IO_SRCS})
# Set destination dir
set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/src)

add_custom_command(TARGET io POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:io>
        ${LIB_OUTPUT_DIR}/$<TARGET_FILE_NAME:io>
    COMMENT "Copying io to bin/"
)

