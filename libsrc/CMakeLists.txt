# add graph lib source code
file(GLOB UTIL_FILE ${PROJECT_SOURCE_DIR}/libsrc/util/*.cpp)
message(STATUS "UTIL_SOURCES: " ${UTIL_FILE})
add_library(util STATIC  ${UTIL_FILE})

file(GLOB GRAPH_FILE ${PROJECT_SOURCE_DIR}/libsrc/graph/*.cpp)
message(STATUS "GRAPH_SOURCES: " ${GRAPH_FILE})
add_library(graph STATIC ${GRAPH_FILE})

file(GLOB LOCALEXTRACTOR_FILE ${PROJECT_SOURCE_DIR}/libsrc/localExtractor/*.cpp)
message(STATUS "LOCALEXTRACTOR_SOURCES: " ${LOCALEXTRACTOR_FILE})
add_library(localExtractor STATIC ${LOCALEXTRACTOR_FILE})