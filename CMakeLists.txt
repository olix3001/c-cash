cmake_minimum_required(VERSION 3.13.4)
project(c-cash VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link other modules
set (TOKENIZER
    tokenizer/Tokenizer.cpp
)
set (PARSER
    parser/Parser.cpp
    
)
set (COMPILER
    compiler/Compiler.cpp
)


add_executable(${PROJECT_NAME} main.cpp)


add_library(${PROJECT_NAME}-tokenizer STATIC ${TOKENIZER})
add_library(${PROJECT_NAME}-parser STATIC ${PARSER})
add_library(${PROJECT_NAME}-compiler STATIC ${COMPILER})

# Link against LLVM libraries
target_link_libraries(${PROJECT_NAME} ${llvm_libs})


target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-tokenizer)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-parser)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-compiler)


set_target_properties(${PROJECT_NAME}-tokenizer PROPERTIES
                        CXX_STANDARD 17
                        CXX_STANDARD_REQUIRED ON 
)
set_target_properties(${PROJECT_NAME}-parser PROPERTIES
                        CXX_STANDARD 17
                        CXX_STANDARD_REQUIRED ON 
)
set_target_properties(${PROJECT_NAME}-compiler PROPERTIES
                        CXX_STANDARD 17
                        CXX_STANDARD_REQUIRED ON 
)