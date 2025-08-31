cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(tracking_dev CXX)
set(EXE gem_tracking)
set(LIBNAME hctracking_dev)

find_package(ROOT REQUIRED CONFIG)
include(${ROOT_USE_FILE})

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets Gui Core)
#find_package(Qt5 REQUIRED Concurrent Core DBus Gui Network OpenGL Sql Widgets Xml)
#find_package(Qt5Widgets REQUIRED)
#find_package(Qt5Gui REQUIRED)
#find_package(Qt5Core REQUIRED)


set(libsrcs
    src/AbstractDetector.cpp
    src/Tracking.cpp
    src/tracking_struct.cpp
    src/TrackingUtility.cpp
    src/TrackingDataHandler.cpp
    src/CoordSystem.cpp
    )

set(exesrcs
    src/Detector2DItem.cpp
    src/Detector2DView.cpp
    src/main.cpp
    src/Viewer.cpp
    )

set(libheaders
    include/Tracking.h
    include/tracking_struct.h
    include/TrackingUtility.h
    include/TrackingDataHandler.h
    include/histos.hpp
    include/CoordSystem.h
    include/AbstractDetector.h
    )

set(exeheaders
    include/Detector2DItem.h
    include/Detector2DView.h
    include/Viewer.h
    )

# https://github.com/bincrafters/community/issues/466
# need to add your header files in add_executable, otherwise the moc won't parse them
add_library(${LIBNAME} ${libsrcs})
add_executable(${EXE} ${exesrcs} ${libsrcs} ${exeheaders} ${libheaders})

target_include_directories(${EXE}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC /usr/include/qt5/QtGui
    PUBLIC /usr/include/qt5
    PUBLIC /usr/include/qt5/QtWidgets
    PUBLIC /usr/include/qt5/QtCore
    )

target_link_libraries(${EXE}
    PUBLIC gem_ana
    PUBLIC gem_decoder
    PUBLIC ${ROOT_LIBRARIES}
    PUBLIC Qt5::Widgets Qt5::Core Qt5::Gui
    )

install(TARGETS ${EXE}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

target_include_directories(${LIBNAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_link_libraries(${LIBNAME}
    PUBLIC gem_ana
    PUBLIC gem_decoder
    PUBLIC ${ROOT_LIBRARIES}
    )

install(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

install(FILES ${libheaders}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
