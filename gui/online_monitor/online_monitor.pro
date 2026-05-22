######################################################################
# online_monitor: ET client wrapper library
#
# Builds the CODA ET client sources (read-only, from third_party) together
# with OnlineMonitor.cpp into a single shared library, libonline_monitor.
# All build artifacts (Makefile, obj/, lib/) stay under gui/online_monitor/
# so third_party/ is never written to.
######################################################################

TEMPLATE = lib
TARGET   = lib/online_monitor
target.path += ./lib
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++17
QMAKE_CFLAGS   += -std=gnu99

######################################################################
# ET sources (compiled here, never under third_party)
ET_DIR = ../../third_party/et-16.6.0/libsrc

INCLUDEPATH += . $$ET_DIR

# Compile the full ET source set, with two exclusions:
#  - et_jni.c          : needs a generated JNI header (Java-only build).
#  - et_remoteclient.c : a standalone "remote-only" client variant that
#                        re-defines et_open/et_event_get/... and is meant to
#                        build a SEPARATE library; including it here causes
#                        duplicate-symbol link errors.
ET_SOURCES = $$files($$ET_DIR/*.c)
ET_SOURCES -= $$ET_DIR/et_jni.c
ET_SOURCES -= $$ET_DIR/et_remoteclient.c
SOURCES += $$ET_SOURCES

######################################################################
# decoder + gem headers/libs (for EventParser, MPDSSPRawEventDecoder, APV types)
INCLUDEPATH += ../../decoder/include ../../third_party/evio-5.2 ../../gem/include
LIBS += -L../../decoder/lib -ldecoder
LIBS += -L../../gem/lib -lgem

######################################################################
# ROOT (decoder/gem headers transitively pull ROOT in)
INCLUDEPATH += $$system(root-config --incdir)
LIBS += $$system(root-config --glibs)

######################################################################
# pthreads (ET requirement)
LIBS += -lpthread

######################################################################
# wrapper
HEADERS += OnlineMonitor.h
SOURCES += OnlineMonitor.cpp

MOC_DIR     = moc
OBJECTS_DIR = obj
