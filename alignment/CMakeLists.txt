cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

project(alignment CXX)
set(EXE alignment)
set(LIBNAME gem_alignment)

find_package(ROOT REQUIRED CONFIG)
include(${ROOT_USE_FILE})

set(libsrcs
    matrix.cpp
    ToyModel.cpp
    standard_align.cpp
    )

set(exesrcs
    test.cpp
    )

set(libheaders
    matrix.h
    ToyModel.h
    standard_align.h
    )

set(exeheaders
    )

# https://github.com/bincrafters/community/issues/466
# need to add your header files in add_executable, otherwise the moc won't parse them
add_library(${LIBNAME} ${libsrcs})
add_executable(${EXE} ${exesrcs} ${libsrcs} ${exeheaders} ${libheaders})

target_include_directories(${EXE}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_link_libraries(${EXE}
    PUBLIC hctracking_dev
    PUBLIC ${ROOT_LIBRARIES}
    )

install(TARGETS ${EXE}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

target_include_directories(${LIBNAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_link_libraries(${LIBNAME}
    PUBLIC hctracking_dev
    PUBLIC ${ROOT_LIBRARIES}
    )

install(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

install(FILES ${libheaders}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
