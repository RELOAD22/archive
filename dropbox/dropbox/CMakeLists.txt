cmake_minimum_required(VERSION 3.16)

project(dropboxserver)

set(CMAKE_CXX_FLAGS "${CAMKE_CXX_FLAGS} -std=c++11 -pthread -Wall")
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

aux_source_directory(./lib LIB_SRCS)
aux_source_directory(./server SERVER_SRCS)
#aux_source_directory(./pc PC_SRCS)

add_executable(dbs ${SERVER_SRCS} ${LIB_SRCS})
#add_executable(client ${PC_SRCS} ${LIB_SRCS})

target_link_libraries(dbs ssl crypto gflags glog jsoncpp)
#target_link_libraries(client ssl crypto)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build) 