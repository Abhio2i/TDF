# =============================================================================
# AESA plugin SDK — example plugin (separate qmake project).
# Build:
#   mkdir -p ~/Desktop/work/aesa_example_plugin_build
#   cd ~/Desktop/work/aesa_example_plugin_build
#   /home/o2i/Qt/5.15.2/gcc_64/bin/qmake TDF_SRC=<path-to-TDF-repo> \
#       <path-to-TDF-repo>/sdk/examples/aesa_example_plugin/aesa_example_plugin.pro
#   make -j8
# Output: libaesa_example_plugin.so
# =============================================================================
TEMPLATE = lib
CONFIG  += plugin c++17
QT      += core
TARGET   = aesa_example_plugin

# Root of the TDF source tree (for core/Plugins/Interfaces/aesaRadarInterface.h).
isEmpty(TDF_SRC): TDF_SRC = $$PWD/../../..
!exists($$TDF_SRC/core/Plugins/Interfaces/aesaRadarInterface.h) {
    error("TDF_SRC does not point at the TDF source tree: $$TDF_SRC")
}
INCLUDEPATH += $$TDF_SRC

HEADERS += aesaexampleplugin.h
SOURCES += aesaexampleplugin.cpp
