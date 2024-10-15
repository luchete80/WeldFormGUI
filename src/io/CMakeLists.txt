
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set(IO_SRCS         "ModelWriter.cpp"

)

ADD_LIBRARY(io ${IO_SRCS})
