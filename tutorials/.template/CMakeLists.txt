cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME ponca-tuto)

project(${PROJECT_NAME})
include(CPM.cmake)

if (NOT DEFINED PONCA_TUTORIALS_VERBOSE)
    option(PONCA_TUTORIALS_VERBOSE "Enable verbose output in tutorials" ON)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()


set(CMAKE_CXX_STANDARD 20)

CPMAddPackage("gh:CLIUtils/CLI11@2.6.2")
CPMAddPackage("gh:poncateam/ponca#master")
CPMAddPackage(URI "gh:nmwsharp/happly#master" DOWNLOAD_ONLY TRUE)

if (NOT TARGET PoncaTutorialsBaseLibs)
    add_library(PoncaTutorialsBaseLibs INTERFACE)
    target_link_libraries(PoncaTutorialsBaseLibs INTERFACE CLI11::CLI11)
    target_link_libraries(PoncaTutorialsBaseLibs INTERFACE Ponca::Fitting Ponca::SpatialPartitioning)
    target_include_directories(PoncaTutorialsBaseLibs INTERFACE ${happly_SOURCE_DIR})

    # Optional
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(PoncaTutorialsBaseLibs INTERFACE OpenMP::OpenMP_CXX)
    endif()
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC PoncaTutorialsBaseLibs)

if (PONCA_TUTORIALS_VERBOSE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VERBOSE)
endif()