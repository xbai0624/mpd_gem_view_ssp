######################################################################
# qmake (3.1) Thu Oct 22 17:42:53 2020
######################################################################

TEMPLATE = app
TARGET = ../bin/data_viewer

######################################################################
# compiler configs

CONFIG += c++14 release
QT += core gui widgets

######################################################################
# headers
INCLUDEPATH += . ./include 

######################################################################
# decoder headers
INCLUDEPATH += ../decoder/include ../decoder/evio-5.2

# decoder lib
LIBS += -L../decoder/lib -ldecoder

######################################################################
# gem headers
INCLUDEPATH += ../gem/include ../gem/third_party

# decoder lib
LIBS += -L../gem/lib -lgem

######################################################################
# epics headers
INCLUDEPATH += ../epics/include

# decoder lib
LIBS += -L../epics/lib -lepics

######################################################################
# ROOT headers libs

INCLUDEPATH += ${ROOTSYS}/include

# root libs
LIBS += -L$(ROOTSYS)/lib -lCore -lRIO -lNet \
        -lHist -lGraf -lGraf3d -lGpad -lTree \
        -lRint -lPostscript -lMatrix -lPhysics \
        -lGui -lRGL

######################################################################
#  dir setting

MOC_DIR = moc
OBJECTS_DIR = obj

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += include/QRootCanvas.h \
           include/QMainCanvas.h \
           include/Viewer.h \
           include/ComponentsSchematic.h \
           include/GEMAnalyzer.h \
           include/GEMReplay.h \
           include/HistoItem.h \
           include/HistoView.h \
           include/Detector2DItem.h \
           include/Detector2DView.h \
           include/HistoWidget.h \
           include/InfoCenter.h \
           include/ColorSpectrum.h \
           include/OnlineAnalysisInterface.h \

SOURCES += src/main.cpp \
           src/QRootCanvas.cpp \
           src/QMainCanvas.cpp \
           src/Viewer.cpp \
           src/ComponentsSchematic.cpp \
           src/GEMAnalyzer.cpp \
           src/GEMReplay.cpp \
           src/HistoItem.cpp \
           src/HistoView.cpp \
           src/Detector2DItem.cpp \
           src/Detector2DView.cpp \
           src/HistoWidget.cpp \
           src/InfoCenter.cpp \
           src/ColorSpectrum.cpp \
           src/OnlineAnalysisInterface.cpp \
