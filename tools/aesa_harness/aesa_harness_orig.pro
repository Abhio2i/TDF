QT       = core
CONFIG  += console c++17
CONFIG  -= app_bundle
TARGET   = aesa_harness
DEFINES += AESA_REGRESSION_TEST

isEmpty(SRC_ROOT): error("Pass SRC_ROOT=<absolute path to source root>")

AESA_LIB = $$SRC_ROOT/core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib
INCLUDEPATH += $$AESA_LIB

SOURCES += main.cpp \
    $$AESA_LIB/radarmodel_aesa.cpp \
    $$AESA_LIB/radarantenna_aesa.cpp \
    $$AESA_LIB/radarscheduler.cpp \
    $$AESA_LIB/radartracker_aesa.cpp \
    $$AESA_LIB/radarsignallibrary_aesa.cpp \
    $$AESA_LIB/radarsignalprocessor_aesa.cpp
