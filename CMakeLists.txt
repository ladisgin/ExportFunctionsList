cmake_minimum_required(VERSION 3.20)
project(ExportFunctions)

find_package(Clang REQUIRED)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${LLVM_CMAKE_DIR}"
  )

include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

add_llvm_executable(ExportFunctions main.cpp)
set_property(TARGET ExportFunctions PROPERTY CXX_STANDARD 17)
target_link_libraries(ExportFunctions PRIVATE clangTooling)


