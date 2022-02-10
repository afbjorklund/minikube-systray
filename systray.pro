HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp
RESOURCES     = systray.qrc

QT += widgets
requires(qtConfig(combobox))

DISTFILES += \
    LICENSE

win32: DEFINES += QT_NO_TERMWIDGET

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += qtermwidget5
