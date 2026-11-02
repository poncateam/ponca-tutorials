cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME ponca-marchingcube-cgal)

project(${PROJECT_NAME})
include(CPM.cmake)


set(CMAKE_CXX_STANDARD 20)

if (NOT DEFINED PONCA_TUTORIALS_VERBOSE)
    option(PONCA_TUTORIALS_VERBOSE "Enable verbose output in tutorials" ON)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

CPMAddPackage("gh:CLIUtils/CLI11@2.6.2")
CPMAddPackage("gh:poncateam/ponca#master")
CPMAddPackage(URI "gh:nmwsharp/happly#master" DOWNLOAD_ONLY TRUE)

if (NOT TARGET PoncaTutorialsBaseLibs)
    add_library(PoncaTutorialsBaseLibs INTERFACE)
    target_link_libraries(PoncaTutorialsBaseLibs INTERFACE CLI11::CLI11)
    target_link_libraries(PoncaTutorialsBaseLibs INTERFACE Ponca::Fitting Ponca::SpatialPartitioning)
    target_include_directories(PoncaTutorialsBaseLibs INTERFACE ${happly_SOURCE_DIR})

    # Optional: the grid loops are parallelized with OpenMP when it is available
    find_package(OpenMP REQUIRED)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(PoncaTutorialsBaseLibs INTERFACE OpenMP::OpenMP_CXX)
    endif()
endif()

# Pull CGal (Boost is dependency.)
CPMAddPackage(NAME CGAL VERSION 6.1 URL https://github.com/CGAL/cgal/releases/download/v6.1/CGAL-6.1.tar.xz DOWNLOAD_ONLY YES)
CPMAddPackage(NAME Boost VERSION 1.89.0 URL https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.bz2 DOWNLOAD_ONLY YES)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC PoncaTutorialsBaseLibs)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CGAL_SOURCE_DIR}/include ${Boost_SOURCE_DIR})

if (PONCA_TUTORIALS_VERBOSE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VERBOSE)
endif()
