
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
include_directories("../Model")
set(IO_SRCS         "ModelWriter.cpp"
                    "ModelReader.cpp"
)

ADD_LIBRARY(io ${IO_SRCS})
