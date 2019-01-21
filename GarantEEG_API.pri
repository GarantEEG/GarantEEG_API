QT       += core

CONFIG   += c++11

LIBS += -lwsock32

SOURCES += \
    $$PWD/src/EEG8.cpp \
    $$PWD/src/GarantEEG_API_CPP.cpp \
    $$PWD/src/Filtering/BaseFilter.cpp \
    $$PWD/src/Filtering/dspfilter/Bessel.cpp \
    $$PWD/src/Filtering/dspfilter/Biquad.cpp \
    $$PWD/src/Filtering/dspfilter/Butterworth.cpp \
    $$PWD/src/Filtering/dspfilter/Cascade.cpp \
    $$PWD/src/Filtering/dspfilter/ChebyshevI.cpp \
    $$PWD/src/Filtering/dspfilter/ChebyshevII.cpp \
    $$PWD/src/Filtering/dspfilter/Custom.cpp \
    $$PWD/src/Filtering/dspfilter/Design.cpp \
    $$PWD/src/Filtering/dspfilter/Documentation.cpp \
    $$PWD/src/Filtering/dspfilter/Elliptic.cpp \
    $$PWD/src/Filtering/dspfilter/Filter.cpp \
    $$PWD/src/Filtering/dspfilter/Legendre.cpp \
    $$PWD/src/Filtering/dspfilter/Param.cpp \
    $$PWD/src/Filtering/dspfilter/PoleFilter.cpp \
    $$PWD/src/Filtering/dspfilter/RBJ.cpp \
    $$PWD/src/Filtering/dspfilter/RootFinder.cpp \
    $$PWD/src/Filtering/dspfilter/State.cpp

HEADERS  += $$PWD/include/GarantEEG_API_Types.h \
    $$PWD/include/GarantEEG_API_C.h \ \
    $$PWD/include/GarantEEG_API_CPP.h \
    $$PWD/src/EEG8.h \
    $$PWD/src/Filtering/BaseFilter.h \
    $$PWD/src/Filtering/dspfilter/Bessel.h \
    $$PWD/src/Filtering/dspfilter/Biquad.h \
    $$PWD/src/Filtering/dspfilter/Butterworth.h \
    $$PWD/src/Filtering/dspfilter/Cascade.h \
    $$PWD/src/Filtering/dspfilter/ChebyshevI.h \
    $$PWD/src/Filtering/dspfilter/ChebyshevII.h \
    $$PWD/src/Filtering/dspfilter/Common.h \
    $$PWD/src/Filtering/dspfilter/Custom.h \
    $$PWD/src/Filtering/dspfilter/Design.h \
    $$PWD/src/Filtering/dspfilter/Dsp.h \
    $$PWD/src/Filtering/dspfilter/Elliptic.h \
    $$PWD/src/Filtering/dspfilter/Filter.h \
    $$PWD/src/Filtering/dspfilter/Layout.h \
    $$PWD/src/Filtering/dspfilter/Legendre.h \
    $$PWD/src/Filtering/dspfilter/MathSupplement.h \
    $$PWD/src/Filtering/dspfilter/Params.h \
    $$PWD/src/Filtering/dspfilter/PoleFilter.h \
    $$PWD/src/Filtering/dspfilter/RBJ.h \
    $$PWD/src/Filtering/dspfilter/RootFinder.h \
    $$PWD/src/Filtering/dspfilter/SmoothedFilter.h \
    $$PWD/src/Filtering/dspfilter/State.h \
    $$PWD/src/Filtering/dspfilter/Types.h \
    $$PWD/src/Filtering/dspfilter/Utilities.h \
    $$PWD/src/Filtering/ButterworthFilter.hpp
