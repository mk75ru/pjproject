QT -= gui
QT += sql

TEMPLATE = lib

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CheckData.cpp \
    TechPgDataKeeper.cpp

HEADERS += \
    TechPgDataKeeper.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/../../Include
INCLUDEPATH += $$PWD/../TsInclude

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../ThSrvObjects-build/release/ -lThSrvObjects
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../ThSrvObjects-build/debug/ -lThSrvObjects
else:unix:!macx: LIBS += -L$$PWD/../ThSrvObjects-build/ -lThSrvObjects

INCLUDEPATH += $$PWD/../ThSrvObjects
DEPENDPATH += $$PWD/../ThSrvObjects

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../AlertSeances-build/release/ -lAlertSeances
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../AlertSeances-build/debug/ -lAlertSeances
else:unix:!macx: LIBS += -L$$PWD/../AlertSeances-build/ -lAlertSeances

INCLUDEPATH += $$PWD/../AlertSeances
DEPENDPATH += $$PWD/../AlertSeances

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../XValue-build/release/ -lXValue
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../XValue-build/debug/ -lXValue
else:unix:!macx: LIBS += -L$$PWD/../../XValue-build/ -lXValue

INCLUDEPATH += $$PWD/../../XValue
DEPENDPATH += $$PWD/../../XValue
