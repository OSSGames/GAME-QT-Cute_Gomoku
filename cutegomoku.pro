TEMPLATE = app
TARGET = cutegomoku
QT += core \
    gui 
HEADERS += error.h \
    gomoku.h \
    gomokugame.h \
    gomokugamenode.h \
    labelbutton.h
SOURCES += gomokugamenode.cpp \
    main.cpp \
    gomoku.cpp \
    error.cpp \
    gomokugame.cpp \
    labelbutton.cpp
FORMS += gomoku.ui
RESOURCES += images.qrc
