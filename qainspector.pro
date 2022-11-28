QT += core gui
QT += network xml widgets

CONFIG += c++11

qtHaveModule(xmlpatterns) {
    QT += xmlpatterns
    DEFINES += MO_USE_QXMLPATTERNS
}

SOURCES += \
    PaintedWidget.cpp \
    main.cpp \
    socketconnector.cpp \
    iteminfodialog.cpp \
    treeviewdialog.cpp \
    mytreemodel2.cpp

HEADERS += \
    PaintedWidget.hpp \
    socketconnector.h \
    iteminfodialog.h \
    treeviewdialog.h \
    mytreemodel2.h

TARGET = qainspector

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

