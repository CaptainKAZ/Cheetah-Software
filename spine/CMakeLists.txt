include_directories("./")

add_library(socketcan STATIC socketcan.cpp)

add_executable(spine spine.cpp)
target_link_libraries(spine socketcan)