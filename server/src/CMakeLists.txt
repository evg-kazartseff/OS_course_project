set(HEADERS server.h)
set(SOURCES main.cpp server.cpp)

add_executable(${PROJECT_N} ${HEADERS} ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT_N} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_N} Json pthread)