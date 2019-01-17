QT       += core

CONFIG   += c++11

LIBS += -lwsock32

SOURCES += \
    $$PWD/src/EEG8.cpp \
    $$PWD/src/GarantEEG_API_CPP.cpp

HEADERS  += $$PWD/include/GarantEEG_API_Types.h \
    $$PWD/include/GarantEEG_API_C.h \ \
    $$PWD/include/GarantEEG_API_CPP.h \
    $$PWD/src/EEG8.h
