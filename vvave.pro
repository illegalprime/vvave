QT       += quick
QT       += multimedia
QT       += sql
QT       += websockets
QT       += network
QT       += xml
QT       += qml
QT       += widgets

TARGET = vvave
TEMPLATE = app

CONFIG += ordered
CONFIG += c++11

DESTDIR = $$OUT_PWD/../

linux:unix:!android {
    message(Building for Linux KDE)
    include(kde/kde.pri)

} else:android {
    message(Building helpers for Android)
    include(android/android.pri)
    include(3rdparty/taglib.pri)
    include(android-openssl.pri)
    include(3rdparty/kirigami/kirigami.pri)

    DEFINES += STATIC_KIRIGAMI        

} else {
    message("Unknown configuration")
}

include(mauikit/mauikit.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    db/collectionDB.cpp \
    services/local/taginfo.cpp \
    services/local/player.cpp \
    utils/brain.cpp \
    services/local/socket.cpp \
    pulpo/pulpo.cpp \
    pulpo/htmlparser.cpp \
    services/web/youtube.cpp \
    pulpo/services/deezerService.cpp \
    pulpo/services/lastfmService.cpp \
    pulpo/services/spotifyService.cpp \
    pulpo/services/musicbrainzService.cpp \
    pulpo/services/geniusService.cpp \
    pulpo/services/lyricwikiaService.cpp \ 
    babe.cpp \
    settings/BabeSettings.cpp \
    db/conthread.cpp \
    services/web/babeit.cpp \
    utils/babeconsole.cpp \
    services/local/youtubedl.cpp \
    services/local/linking.cpp \
    settings/fileloader.cpp \
    services/web/Spotify/spotify.cpp


RESOURCES += qml.qrc
RESOURCES += icons.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DISTFILES += \
    db/script.sql


HEADERS += \
    db/collectionDB.h \
    utils/bae.h \
    settings/fileloader.h \
    services/local/taginfo.h \
    services/local/player.h \
    utils/brain.h \
    services/local/socket.h \
    pulpo/enums.h \
    pulpo/pulpo.h \
    pulpo/htmlparser.h \
    services/web/youtube.h \
    pulpo/services/spotifyService.h \
    pulpo/services/geniusService.h \
    pulpo/services/musicbrainzService.h \
    pulpo/services/deezerService.h \
    pulpo/services/lyricwikiaService.h \
    pulpo/services/lastfmService.h \       
    babe.h \
    settings/BabeSettings.h \
    db/conthread.h \
    services/web/babeit.h \
    utils/babeconsole.h \
    utils/singleton.h \
    services/local/youtubedl.h \
    services/local/linking.h \
    services/web/Spotify/spotify.h


#TAGLIB


#INCLUDEPATH += /usr/include/python3.6m

#LIBS += -lpython3.6m
#defineReplace(copyToDir) {
#    files = $$1
#    DIR = $$2
#    LINK =

#    for(FILE, files) {
#        LINK += $$QMAKE_COPY $$shell_path($$FILE) $$shell_path($$DIR) $$escape_expand(\\n\\t)
#    }
#    return($$LINK)
#}

#defineReplace(copyToBuilddir) {
#    return($$copyToDir($$1, $$OUT_PWD))
#}

## Copy the binary files dependent on the system architecture
#unix:!macx {
#    message("Linux")
#    QMAKE_POST_LINK += $$copyToBuilddir($$PWD/library/cat)
#}
