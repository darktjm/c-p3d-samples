# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

set(SAMPLE shader-terrain)

# The project name can be anything; it doesn't affect the results.
project(${SAMPLE} LANGUAGES CXX)

include(../supt/samples.cmake)

#### Build executable
add_executable(${SAMPLE} main.cpp)
precompile_headers()
target_link_libraries(${SAMPLE} ${PANDALIBS} ${PANDA-p3direct} sample_supt)
install(TARGETS ${SAMPLE})
