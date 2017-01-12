TARGET = ProcessController
TEMPLATE = app

QT += qml quick core websockets network widgets
CONFIG += c++11

CONFIG(debug, debug | release) DEFINES += _DEBUG

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

RESOURCES += \
    src/qml/qml.qrc

HEADERS += \
    src/c++/MainController.hpp \
    src/c++/TcpCommunicator.hpp

SOURCES += \
    src/c++/main.cpp \
    src/c++/MainController.cpp \
    src/c++/TcpCommunicator.cpp

