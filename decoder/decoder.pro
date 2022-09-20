######################################################################
# Automatically generated by qmake (3.1) Sat Nov 7 17:18:28 2020
######################################################################

TEMPLATE = lib
TARGET = lib/decoder

#TARGET.path = /home/daq/test/MPD/MPD_GEM_View/decoder/lib/
#TARGET.files = $$TARGET
#INSTALLS += TARGET

QMAKE_CXXFLAGS = -std=c++17
CONFIG += release

# self headers
INCLUDEPATH += . ./include
# coda headers
INCLUDEPATH += ${CODA}/common/include
# root headers
INCLUDEPATH += ${ROOTSYS}/include

# coda libs
LIBS += -L${CODA}/Linux-x86_64/lib -levio
# root libs
LIBS += -L${ROOTSYS}/lib -lCore -lRIO -lNet \
	-lHist -lGraf -lGraf3d -lGpad -lTree \
	-lRint -lPostscript -lMatrix -lPhysics \
	-lGui -lRGL

MOC_DIR = moc
OBJECTS_DIR = obj

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS #DEBUG

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += include/EvioFileReader.h \
           include/EventParser.h \
           include/GeneralEvioStruct.h \
           include/MPDVMERawEventDecoder.h \
           include/MPDSSPRawEventDecoder.h \
           include/RolStruct.h \
           include/MPDDataStruct.h \
           include/AbstractRawDecoder.h \
           include/sspApvdec.h \
           include/TriggerDecoder.h \
           include/SRSRawEventDecoder.h \

SOURCES += src/EvioFileReader.cpp \ 
           src/EventParser.cpp \ 
           src/MPDVMERawEventDecoder.cpp \
           src/MPDSSPRawEventDecoder.cpp \
           src/AbstractRawDecoder.cpp \
           src/MPDDataStruct.cpp \
           src/TriggerDecoder.cpp \
           src/SRSRawEventDecoder.cpp \

