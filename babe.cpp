#include "babe.h"

#include "db/collectionDB.h"
#include "db/conthread.h"
#include "settings/BabeSettings.h"
#include "pulpo/pulpo.h"
#include "utils/babeconsole.h"

#include <QPalette>
#include <QColor>
#include <QIcon>
#include <QGuiApplication>
#include <QDirIterator>
#include <QtQml>
#include <QDesktopServices>
#include <QCursor>

//#include "Python.h"

#if (defined (Q_OS_LINUX) && !defined (Q_OS_ANDROID))
#include <QWidget>
#include "kde/notify.h"
#endif

#if defined(Q_OS_ANDROID)
#include "android/notificationclient.h"
#include "android/android.h"
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QtAndroid>
#include <QException>

#include <QAudioOutput>
#include <QAudioInput>

class InterfaceConnFailedException : public QException
{
    public:
        void raise() const { throw *this; }
        InterfaceConnFailedException *clone() const { return new InterfaceConnFailedException(*this); }
};
#elif defined(Q_OS_WINDOWS)
#elif defined(Q_OS_DARWIN)
#else
#endif

using namespace BAE;

Babe::Babe(QObject *parent) : CollectionDB(parent)
{    
    bDebug::Instance()->msg("CONSTRUCTING ABE INTERFACE");

    this->settings = new BabeSettings(this);
    this->thread = new ConThread;

    connect(bDebug::Instance(), SIGNAL(debug(QString)), this, SLOT(debug(QString)));

    connect(settings, &BabeSettings::refreshTables, [this](int size)
    {
        emit this->refreshTables(size);
    });

    connect(settings, &BabeSettings::refreshATable, [this](BAE::TABLE table)
    {
        switch(table)
        {
            case BAE::TABLE::TRACKS: emit this->refreshTracks(); break;
            case BAE::TABLE::ALBUMS: emit this->refreshAlbums(); break;
            case BAE::TABLE::ARTISTS: emit this->refreshArtists(); break;
        }

    });

    connect(&link, &Linking::parseAsk, this, &Babe::linkDecoder);
    connect(&link, &Linking::bytesFrame, [this](QByteArray array)
    {
        this->player.appendBuffe(array);

    });
    connect(&link, &Linking::arrayReady, [this](QByteArray array)
    {
        qDebug()<<"trying to play the array";
        this->player.playBuffer();
    });

#if (defined (Q_OS_LINUX) && !defined (Q_OS_ANDROID))
    this->nof = new Notify(this);
    connect(this->nof,&Notify::babeSong,[this]()
    {
        emit this->babeIt();
    });

    connect(this->nof,&Notify::skipSong,[this]()
    {
        emit this->skipTrack();
    });
#elif defined (Q_OS_ANDROID)
    this->nof = new NotificationClient(this);
    this->android = new Android(this);

    connect(android, &Android::folderPicked, [this](const QString &url)
    {
       qDebug()<< "Folder picked"<< url;
    });
#endif

}

Babe::~Babe()
{
    delete this->thread;
}


//void Babe::runPy()
//{

//    QFile cat (BAE::CollectionDBPath+"cat");
//    qDebug()<<cat.exists()<<cat.permissions();
//    if(!cat.setPermissions(QFile::ExeGroup | QFile::ExeOther | QFile::ExeOther | QFile::ExeUser))
//        qDebug()<<"Faile dot give cat permissionsa";
//    qDebug()<<cat.exists()<<cat.permissions();

//    QProcess process;
//    process.setWorkingDirectory(BAE::CollectionDBPath);
//    process.start("./cat", QStringList());

//    bool finished = process.waitForFinished(-1);
//    QString p_stdout = process.readAll();
//    qDebug()<<p_stdout<<finished<<process.workingDirectory()<<process.errorString();
//}

QVariantList Babe::get(const QString &queryTxt)
{
    return getDBDataQML(queryTxt);
}

QVariantList Babe::getList(const QStringList &urls)
{
    return Babe::transformData(getDBData(urls));
}

void Babe::set(const QString &table, const QVariantList &wheres)
{
    this->thread->start(table, wheres);
}

void Babe::trackPlaylist(const QStringList &urls, const QString &playlist)
{
    QVariantList data;
    for(auto url : urls)
    {
        QVariantMap map {{KEYMAP[KEY::PLAYLIST],playlist},
                         {KEYMAP[KEY::URL],url},
                         {KEYMAP[KEY::ADD_DATE],QDateTime::currentDateTime()}};

        data << map;
    }

    bDebug::Instance()->msg("Adding "+QString::number(urls.size())+" tracks to playlist : "+playlist);
    this->thread->start(BAE::TABLEMAP[TABLE::TRACKS_PLAYLISTS], data);
}

void Babe::trackLyrics(const QString &url)
{
    auto track = getDBData(QString("SELECT * FROM %1 WHERE %2 = \"%3\"").arg(TABLEMAP[TABLE::TRACKS],
                           KEYMAP[KEY::URL], url));

    if(track.isEmpty()) return;

    if(!track.first()[KEY::LYRICS].isEmpty() && track.first()[KEY::LYRICS] != SLANG[W::NONE])
        emit this->trackLyricsReady(track.first()[KEY::LYRICS], url);
    else
        this->fetchTrackLyrics(track.first());
}

bool Babe::trackBabe(const QString &path)
{
    auto babe = getDBData(QString("SELECT %1 FROM %2 WHERE %3 = \"%4\"").arg(KEYMAP[KEY::BABE],
                          TABLEMAP[TABLE::TRACKS],
            KEYMAP[KEY::URL],path));

    if(!babe.isEmpty())
        return babe.first()[KEY::BABE].toInt();

    return false;
}

QString Babe::artistArt(const QString &artist)
{
    auto artwork = getDBData(QString("SELECT %1 FROM %2 WHERE %3 = \"%4\"").arg(KEYMAP[KEY::ARTWORK],
                             TABLEMAP[TABLE::ARTISTS],
            KEYMAP[KEY::ARTIST],artist));

    if(!artwork.isEmpty())
        if(!artwork.first()[KEY::ARTWORK].isEmpty() && artwork.first()[KEY::ARTWORK] != SLANG[W::NONE])
            return artwork.first()[KEY::ARTWORK];

    return "";
}

QString Babe::artistWiki(const QString &artist)
{
    auto wiki = getDBData(QString("SELECT %1 FROM %2 WHERE %3 = \"%4\"").arg(KEYMAP[KEY::WIKI],
                          TABLEMAP[TABLE::ARTISTS],
            KEYMAP[KEY::ARTIST],artist));

    if(!wiki.isEmpty())
        return wiki.first()[KEY::WIKI];

    return "";
}

QString Babe::albumArt(const QString &album, const QString &artist)
{
    auto queryStr = QString("SELECT %1 FROM %2 WHERE %3 = \"%4\" AND %5 = \"%6\"").arg(KEYMAP[KEY::ARTWORK],
            TABLEMAP[TABLE::ALBUMS],
            KEYMAP[KEY::ALBUM],album,
            KEYMAP[KEY::ARTIST],artist);
    auto albumCover = getDBData(queryStr);

    if(!albumCover.isEmpty())
        if(!albumCover.first()[KEY::ARTWORK].isEmpty() && albumCover.first()[KEY::ARTWORK] != SLANG[W::NONE])
            return albumCover.first()[KEY::ARTWORK];

    return "";
}

void Babe::fetchTrackLyrics(DB &song)
{
    Pulpo pulpo;
    pulpo.registerServices({SERVICES::LyricWikia});
    pulpo.setOntology(PULPO::ONTOLOGY::TRACK);
    pulpo.setInfo(PULPO::INFO::LYRICS);

    connect(&pulpo, &Pulpo::infoReady, [this](const BAE::DB &track, const PULPO::RESPONSE  &res)
    {
        if(!res[PULPO::ONTOLOGY::TRACK][PULPO::INFO::LYRICS].isEmpty())
        {
            auto lyrics = res[PULPO::ONTOLOGY::TRACK][PULPO::INFO::LYRICS][PULPO::CONTEXT::LYRIC].toString();

            lyricsTrack(track, lyrics);
            bDebug::Instance()->msg("Downloaded the lyrics for "+track[KEY::TITLE]+" "+track[KEY::ARTIST]);
            emit this->trackLyricsReady(lyrics, track[KEY::URL]);
        }
    });

    pulpo.feed(song, PULPO::RECURSIVE::OFF);
}

void Babe::linkDecoder(QString json)
{

    qDebug()<<"DECODING LINKER MSG"<<json;
    auto ask = link.decode(json);

    auto code = ask[BAE::SLANG[BAE::W::CODE]].toInt();
    auto msg = ask[BAE::SLANG[BAE::W::MSG]].toString();

    switch(static_cast<LINK::CODE>(code))
    {
        case LINK::CODE::CONNECTED :
        {
            this->link.deviceName = msg;
            emit this->link.serverConReady(msg);
            break;
        }
        case LINK::CODE::QUERY :
        case LINK::CODE::FILTER :
        case LINK::CODE::PLAYLISTS :
        {
            auto res = this->getDBDataQML(msg);
            link.sendToClient(link.packResponse(static_cast<LINK::CODE>(code), res));
            break;
        }
        case LINK::CODE::SEARCHFOR :
        {
            auto res = this->searchFor(msg.split(","));
            link.sendToClient(link.packResponse(static_cast<LINK::CODE>(code), res));
            break;
        }
        case LINK::CODE::PLAY :
        {
            QFile file(msg);    // sound dir
            file.open(QIODevice::ReadOnly);
            QByteArray arr = file.readAll();
            qDebug()<<"Preparing track array"<<msg<<arr.size();
            link.sendArrayToClient(arr);
            break;
        }
        case LINK::CODE::COLLECT :
        {
//            auto devices = getDevices();
//            qDebug()<<"DEVICES:"<< devices;
//            if(!devices.isEmpty())
//                sendToDevice(devices.first().toMap().value("name").toString(),
//                             devices.first().toMap().value("id").toString(), msg);
            break;

        }
        default: break;

    }
}

QString Babe::albumWiki(const QString &album, const QString &artist)
{
    auto queryStr = QString("SELECT %1 FROM %2 WHERE %3 = \"%4\" AND %5 = \"%6\"").arg(KEYMAP[KEY::WIKI],
            TABLEMAP[TABLE::ALBUMS],
            KEYMAP[KEY::ALBUM],album,
            KEYMAP[KEY::ARTIST],artist);
    auto wiki = getDBData(queryStr);

    if(!wiki.isEmpty())
        return wiki.first()[KEY::WIKI];

    return "";
}

bool Babe::babeTrack(const QString &path, const bool &value)
{
    if(update(TABLEMAP[TABLE::TRACKS],
              KEYMAP[KEY::BABE],
              value ? 1 : 0,
              KEYMAP[KEY::URL],
              path)) return true;

    return false;
}

void Babe::notify(const QString &title, const QString &body)
{

#if (defined (Q_OS_LINUX) && !defined (Q_OS_ANDROID))
    this->nof->notify(title, body);
#elif defined (Q_OS_ANDROID)
    this->nof->notify(title+" "+body);
#endif

}

void Babe::notifySong(const QString &url)
{
#if (defined (Q_OS_LINUX) && !defined (Q_OS_ANDROID))
    auto query = QString("select t.*, al.artwork from tracks t inner join albums al on al.album = t.album and al.artist = t.artist where url = \"%1\"").arg(url);
    auto track = getDBData(query);
    this->nof->notifySong(track.first());
#else
    Q_UNUSED(url);
#endif
}

void Babe::sendText(const QString &text)
{

#if defined(Q_OS_ANDROID)
    QAndroidJniEnvironment _env;
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");   //activity is valid
    if (_env->ExceptionCheck()) {
        _env->ExceptionClear();
        throw InterfaceConnFailedException();
    }

    if ( activity.isValid() )
    {
        QAndroidJniObject::callStaticMethod<void>("com/example/android/tools/SendIntent",
                                                  "sendText",
                                                  "(Landroid/app/Activity;Ljava/lang/String;)V",
                                                  activity.object<jobject>(),
                                                  QAndroidJniObject::fromString(text).object<jstring>());
        if (_env->ExceptionCheck())
        {
            _env->ExceptionClear();
            throw InterfaceConnFailedException();
        }
    }else
        throw InterfaceConnFailedException();
#endif

}

void Babe::sendTrack(const QString &url)
{
#if defined(Q_OS_ANDROID)
    bDebug::Instance()->msg("Sharing track "+ url);
    QAndroidJniEnvironment _env;
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");   //activity is valid
    if (_env->ExceptionCheck()) {
        _env->ExceptionClear();
        throw InterfaceConnFailedException();
    }
    if ( activity.isValid() )
    {
        QAndroidJniObject::callStaticMethod<void>("com/example/android/tools/SendIntent",
                                                  "sendTrack",
                                                  "(Landroid/app/Activity;Ljava/lang/String;)V",
                                                  activity.object<jobject>(),
                                                  QAndroidJniObject::fromString(url).object<jstring>());
        if (_env->ExceptionCheck()) {
            _env->ExceptionClear();
            throw InterfaceConnFailedException();
        }
    }else
        throw InterfaceConnFailedException();
#endif

}

void Babe::openFile(const QString &url)
{
#if defined(Q_OS_ANDROID)
    bDebug::Instance()->msg("Opening track "+ url);
    QAndroidJniEnvironment _env;
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");   //activity is valid
    if (_env->ExceptionCheck()) {
        _env->ExceptionClear();
        throw InterfaceConnFailedException();
    }
    if ( activity.isValid() )
    {
        QAndroidJniObject::callStaticMethod<void>("com/example/android/tools/SendIntent",
                                                  "openFile",
                                                  "(Landroid/app/Activity;Ljava/lang/String;)V",
                                                  activity.object<jobject>(),
                                                  QAndroidJniObject::fromString(url).object<jstring>());
        if (_env->ExceptionCheck()) {
            _env->ExceptionClear();
            throw InterfaceConnFailedException();
        }
    }else
        throw InterfaceConnFailedException();
#endif

}

void Babe::fileChooser()
{
#if defined(Q_OS_ANDROID)
    this->android->fileChooser();
#endif

}

void Babe::scanDir(const QString &url)
{
    emit this->settings->collectionPathChanged({url});
}

void Babe::brainz(const bool &on)
{
    this->settings->checkCollectionBrainz(on);
}

bool Babe::brainzState()
{
    return loadSetting("BRAINZ", "BABE", false).toBool();
}

void Babe::refreshCollection()
{
    this->settings->refreshCollection();
}

void Babe::getYoutubeTrack(const QString &message)
{
    this->settings->fetchYoutubeTrack(message);
}

QVariant Babe::loadSetting(const QString &key, const QString &group, const QVariant &defaultValue)
{
    return BAE::loadSettings(key, group, defaultValue);
}

void Babe::saveSetting(const QString &key, const QVariant &value, const QString &group)
{
    bDebug::Instance()->msg("Setting saved: "+ key+" "+value.toString()+" "+group);
    BAE::saveSettings(key, value, group);
}

void Babe::savePlaylist(const QStringList &list)
{
    BAE::saveSettings("PLAYLIST", list, "MAINWINDOW");
}

QStringList Babe::lastPlaylist()
{
    return BAE::loadSettings("PLAYLIST","MAINWINDOW",{}).toStringList();
}

void Babe::savePlaylistPos(const int &pos)
{
    BAE::saveSettings("PLAYLIST_POS", pos, "MAINWINDOW");
}

int Babe::lastPlaylistPos()
{
    return BAE::loadSettings("PLAYLIST_POS","MAINWINDOW",QVariant(0)).toInt();
}

bool Babe::fileExists(const QString &url)
{
    return BAE::fileExists(url);
}

void Babe::showFolder(const QString &url)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(url).dir().absolutePath()));

}

QString Babe::babeColor()
{
    return "#f84172";
}


void Babe::androidStatusBarColor(const QString &color, const bool &contrast)
{
#if defined(Q_OS_ANDROID)

    QtAndroid::runOnAndroidThread([=]() {
        QAndroidJniObject window = QtAndroid::androidActivity().callObjectMethod("getWindow", "()Landroid/view/Window;");
        window.callMethod<void>("addFlags", "(I)V", 0x80000000);
        window.callMethod<void>("clearFlags", "(I)V", 0x04000000);
        window.callMethod<void>("setStatusBarColor", "(I)V", QColor(color).rgba());

        if(contrast)
        {
            QAndroidJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
               decorView.callMethod<void>("setSystemUiVisibility", "(I)V", 0x00002000);
        }

    });
#endif
}

bool Babe::isMobile()
{
    return BAE::isMobile();
}

bool Babe::isAndroid()
{
    return BAE::isAndroid();
}

QString Babe::moodColor(const int &pos)
{
    if(pos < BAE::MoodColors.size())
        return BAE::MoodColors.at(pos);
    else return "";
}

QString Babe::homeDir()
{
#if defined(Q_OS_ANDROID)
    QAndroidJniObject mediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
    QAndroidJniObject mediaPath = mediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
    //    bDebug::Instance()->msg("HOMEDIR FROM ADNROID"+ mediaPath.toString());

    if(BAE::fileExists("/mnt/extSdCard"))
        return "/mnt/sdcard";
    else
        return mediaPath.toString();

    //    QAndroidJniObject mediaDir = QAndroidJniObject::callStaticObjectMethod("android.content.Context", "getExternalFilesDir", "()Ljava/io/File;");
    //    QAndroidJniObject mediaPath = mediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
    //    return mediaPath.toString();
#else
    return BAE::HomePath;
#endif
}

QString Babe::musicDir()
{
    return BAE::MusicPath;
}

QStringList Babe::defaultSources()
{
    return BAE::defaultSources;
}

void Babe::registerTypes()
{
    qmlRegisterUncreatableType<Babe>("Babe", 1, 0, "Babe", "ERROR ABE");
}

QString Babe::loadCover(const QString &url)
{
    auto map = getDBData(QStringList() << url);

    if(map.isEmpty()) return "";

    auto track = map.first();
    auto artist = track[KEY::ARTIST];
    auto album = track[KEY::ALBUM];
    auto title = track[KEY::TITLE];

    auto artistImg = this->artistArt(artist);
    auto albumImg = this->albumArt(album, artist);

    if(!albumImg.isEmpty() && albumImg != BAE::SLANG[W::NONE])
        return albumImg;
    else if (!artistImg.isEmpty() && artistImg != BAE::SLANG[W::NONE])
        return artistImg;
    else
        return this->fetchCoverArt(track);
}

QVariantList Babe::searchFor(const QStringList &queries)
{
    QVariantList mapList;
    bool hasKey = false;
    for(auto searchQuery : queries)
    {
        if(searchQuery.contains(BAE::SearchTMap[BAE::SearchT::LIKE]+":") || searchQuery.startsWith("#"))
        {
            if(searchQuery.startsWith("#"))
                searchQuery=searchQuery.replace("#","").trimmed();
            else
                searchQuery=searchQuery.replace(BAE::SearchTMap[BAE::SearchT::LIKE]+":","").trimmed();


            searchQuery = searchQuery.trimmed();
            if(!searchQuery.isEmpty())
            {
                mapList += getSearchedTracks(BAE::KEY::WIKI, searchQuery);
                mapList += getSearchedTracks(BAE::KEY::TAG, searchQuery);
                mapList += getSearchedTracks(BAE::KEY::LYRICS, searchQuery);
            }

        }else if(searchQuery.contains((BAE::SearchTMap[BAE::SearchT::SIMILAR]+":")))
        {
            searchQuery=searchQuery.replace(BAE::SearchTMap[BAE::SearchT::SIMILAR]+":","").trimmed();
            searchQuery=searchQuery.trimmed();
            if(!searchQuery.isEmpty())
                mapList += getSearchedTracks(BAE::KEY::TAG, searchQuery);

        }else
        {
            BAE::KEY key;

            QMapIterator<BAE::KEY, QString> k(BAE::KEYMAP);
            while (k.hasNext())
            {
                k.next();
                if(searchQuery.contains(QString(k.value()+":")))
                {
                    hasKey=true;
                    key=k.key();
                    searchQuery = searchQuery.replace(k.value()+":","").trimmed();
                }
            }

            searchQuery = searchQuery.trimmed();

            if(!searchQuery.isEmpty())
            {
                if(hasKey)
                    mapList += getSearchedTracks(key, searchQuery);
                else
                {
                    auto queryTxt = QString("SELECT t.*, al.artwork FROM tracks t INNER JOIN albums al ON t.album = al.album AND t.artist = al.artist WHERE t.title LIKE \"%"+searchQuery+"%\" OR t.artist LIKE \"%"+searchQuery+"%\" OR t.album LIKE \"%"+searchQuery+"%\"OR t.genre LIKE \"%"+searchQuery+"%\"OR t.url LIKE \"%"+searchQuery+"%\" ORDER BY strftime(\"%s\", t.addDate) desc LIMIT 1000");
                    mapList += getDBDataQML(queryTxt);
                }
            }
        }
    }

    return  mapList;
}

void Babe::debug(const QString &msg)
{
    emit this->message(msg);
}


QString Babe::fetchCoverArt(DB &song)
{
    if(BAE::artworkCache(song, KEY::ALBUM)) return song[KEY::ARTWORK];
    if(BAE::artworkCache(song, KEY::ARTIST)) return song[KEY::ARTWORK];

    Pulpo pulpo;
    pulpo.registerServices({SERVICES::LastFm, SERVICES::Spotify});
    pulpo.setOntology(PULPO::ONTOLOGY::ALBUM);
    pulpo.setInfo(PULPO::INFO::ARTWORK);

    QEventLoop loop;

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(1000);

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    connect(&pulpo, &Pulpo::infoReady, [&](const BAE::DB &track,const PULPO::RESPONSE  &res)
    {
        Q_UNUSED(track);
        if(!res[PULPO::ONTOLOGY::ALBUM][PULPO::INFO::ARTWORK].isEmpty())
        {
            auto artwork = res[PULPO::ONTOLOGY::ALBUM][PULPO::INFO::ARTWORK][PULPO::CONTEXT::IMAGE].toByteArray();
            BAE::saveArt(song, artwork, BAE::CachePath);
        }
        loop.quit();
    });

    pulpo.feed(song, PULPO::RECURSIVE::OFF);

    timer.start();
    loop.exec();
    timer.stop();

    return  song[KEY::ARTWORK];
}

QVariantList Babe::transformData(const DB_LIST &dbList)
{
    QVariantList res;

    for(auto data : dbList)
    {
        QVariantMap map;
        for(auto key : data.keys())
            map[BAE::KEYMAP[key]] = data[key];

        res << map;
    }

    return res;
}

