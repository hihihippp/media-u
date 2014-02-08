#include "config.h"
#include <QFile>
#include <QDir>
#include <iostream>
#include <QDebug>
#include <QStandardPaths>
#include "systemutils.h"


Config::Config(int argc, char* argv[]) {
    setDefaults();
    this->fromArgs(argc, argv);
}

Config::Config(QString initPath)
{
    this->setDefaults();
    this->init(initPath);
}

void Config::setDefaults() {
    mServerPort = -1;
    initialized = false;
    noGui = false;
    fullScreen = false;
    autoOpenBrowser = false;
}

void Config::fromArgs(int argc, char* argv[]) {
    QString configPath;

    for (int i=0; i < argc; ++i) {

        if (strcmp(argv[i], "--configdir") == 0) {
            ++i;
            if (i < argc) {
                configPath = (QString(argv[i]));

                QDir dir(configPath);
                if (!dir.exists() && !QDir::root().mkpath(QString(argv[i]))) {
                    configPath = QString();
                    qDebug() << "config dir does not exist and couldn't be created";
                }
            }
        } else if (strcmp(argv[i], "--fullscreen") == 0) {
            fullScreen = true;
        } else if (strcmp(argv[i], "--nogui") == 0) {
            noGui = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            std::cout << "A Qt Application to manage and play your media library.\n\n";
            std::cout << "available Arguments:\n";
            std::cout << "--fullscreen\n";
            std::cout << "    launch qt gui in fullscreen.\n";
            std::cout << "--nogui\n";
            std::cout << "    launch without showing a window. The Api and mplayer should still work.\n";
            std::cout << "--configdir %dir\n";
            std::cout << "    %dir path to a directory where configs shall be written/read\n";
            std::cout << "    if %dir does not exist it will be created. If it can't be created\n";
            std::cout << "    .media-u in the home directory will be the default configdir.\n";
            std::cout.flush();
            exit(0);
        } else if (i > 0) {
            std::cout << "unknown parameter " << i << ": " << argv[i] << '\n';
            std::cout << "use --help for a parameter list." << std::endl;
            exit(0);
        }
    }

    init(configPath);
}

bool Config::init(QString path) {
    QDir dir;
    if (path.isNull() || path.isEmpty()) {
        dir = this->configPath();
        if (!dir.exists() && !dir.mkpath(dir.absoluteFilePath(".mediaU"))) {
            throw "could not create config dir";
        }
    } else {
        dir = QDir(path);
        mConfigPath = path;
    }

    QFile configFile(dir.absoluteFilePath("config.json"));
    if (configFile.exists() && configFile.open(QFile::ReadOnly) && configFile.size() > 0) {
        return this->parse(configFile.readAll());
    } else {
        return this->createNewConfig(dir.absoluteFilePath("config.json"));
    }
}

bool Config::parse(const QString& jsonData)
{
    std::istringstream input(jsonData.toStdString());
    nw::JsonReader jr(input);
    this->describe(&jr);
    jr.close();

    initialized = true;
    return true;
}

void Config::describe(nw::Describer* de) {
    de->describe("port", mServerPort);
    de->describe("noGui", noGui);
    de->describe("fullScreen", fullScreen);
    de->describe("autoOpenBrowser", autoOpenBrowser);

    de->push("library");
    NwUtils::describe(*de, "path", mLibraryPath);
    de->pop();

    de->push("snapshot");
    snapshotConfig.describe(*de);
    de->pop();

    de->push("mplayer");
    mplayerConfig.describe(de);
    de->pop();

    de->push("rss");
    rssConfig.describe(*de);
    de->pop();
}

bool Config::createNewConfig(QString filepath)
{
    mLibraryPath = libraryPath();
    mServerPort = 8082;

    nw::JsonWriter jw(filepath.toStdString());
    this->describe(&jw);
    jw.close();

    qDebug() << libraryPath();

    initialized = true;
    return true;
}

QDir Config::configPath() {
    if (mConfigPath.isNull() || mConfigPath.isEmpty()) {
        return QDir(QDir::home().absoluteFilePath(".mediaU"));
    }
    return QDir(mConfigPath);
}

QString Config::libraryPath() {
    if (initialized) {
        return this->mLibraryPath;
    }

    QDir dir = configPath();
    return dir.absoluteFilePath("library");
}

QString Config::malConfigFilePath() {
    return configPath().absoluteFilePath("malConfig.json");
}

QString Config::mplayerLocation() {
    return mplayerConfig.path;
}

QString Config::omxplayerLocation() {
    return "/usr/bin/omxplayer";
}

bool Config::omxPlayerIsInstalled() {
    return SystemUtils::commandExists(omxplayerLocation());
}

bool Config::mplayerIsInstalled() {
    return SystemUtils::commandExists(mplayerLocation());
}

const SnapshotConfig&Config::getSnapshotConfigConstRef() const {
    return snapshotConfig;
}

int Config::serverPort() {
    if (initialized && mServerPort > 0) {
        return mServerPort;
    }
    return 8082;
}

void MplayerConfig::describe(nw::Describer *de) {
    if (de->isInWriteMode() && needsInit()) {
        this->setDefaultValues();
    }
    NwUtils::describe(*de, "path", path);
    NwUtils::describe(*de, "arguments", arguments, ' ');

    if (de->isInReadMode() && needsInit()) {
        this->setDefaultValues();
    }
}

bool MplayerConfig::needsInit() {
    return this->path.isEmpty();
}

void MplayerConfig::setDefaultValues() {
        this->path = "mplayer";
    this->arguments = QStringList() <<
        "-fs" <<
        "-ass" <<
#ifdef __linux__
        // workaround for a pulse audio bug
        "-ao" <<
        "alsa" <<
#endif
        "-embeddedfonts";
}

const MplayerConfig& Config::getMplayerConfigConstRef() const
{
    return mplayerConfig;
}

const RssConfig& Config::getRssConfigConstRef() const {
    return rssConfig;
}

bool Config::getFullScreen() const{
    return fullScreen;
}

bool Config::getNoGui() const {
    return noGui;
}

bool Config::getAutoOpenBrowser() const {
    return autoOpenBrowser;
}


RssConfig::RssConfig() :
    autoDownload(true),
    requireFavouriteReleaseGroup(true),
    includeEnglish(true),
    includeRaw(false)
{

}

void RssConfig::describe(nw::Describer& de) {
    NwUtils::describe(de, "autoDownload", autoDownload);
    NwUtils::describe(de, "requireFavouriteReleaseGroup", requireFavouriteReleaseGroup);
    NwUtils::describe(de, "includeEnglish", includeEnglish);
    NwUtils::describe(de, "includeRaw", includeRaw);
}


void SnapshotConfig::describe(nw::Describer& de) {
    if (de.isInWriteMode() && needsInit()) {
        this->setDefaultValues();
    }
    NwUtils::describe(de, "snapshotDir", snapshotDir);
    NwUtils::describe(de, "snapshotFormat", snapshotFormat);
    NwUtils::describe(de, "snapshotQuality", snapshotQuality);
    NwUtils::describe(de, "snapshotName", snapshotName);

    NwUtils::describe(de, "gifDir", gifDir);
    NwUtils::describe(de, "gifResolutionX", gifResolutionX);
    NwUtils::describe(de, "gifResolutionY", gifResolutionY);
    NwUtils::describe(de, "gifMaxSizeMiB", gifMaxSizeMiB);
    NwUtils::describe(de, "gifFramesDropped", gifFramesDropped);

    this->snapshotQuality = std::min(this->snapshotQuality, (qint8)100);
    this->snapshotQuality = std::max(this->snapshotQuality, (qint8)-1);

    if (de.isInReadMode() && needsInit()) {
        this->setDefaultValues();
    }
}

bool SnapshotConfig::needsInit() {
    return snapshotName.isEmpty() || snapshotDir.isEmpty() || gifDir.isEmpty() || gifName.isEmpty();
}

void SnapshotConfig::setDefaultValues() {
    this->snapshotFormat = "jpg";
    this->snapshotName = "$(tvShow)/$(file) - $(progressM)m$(progressS)s - $(nowDate).$(ext)";
    this->snapshotQuality = 100;

    this->gifName = "$(tvShow)/$(file) - $(startM)m$(startS)s:$(endM)m$(endS)s - $(nowDate).$(ext)";
    this->gifMaxSizeMiB = 3.f;
    this->gifFramesDropped = 2;
    this->gifResolutionX = 420;
    this->gifResolutionY = 270;

    QStringList imageDirs = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

    if (!imageDirs.isEmpty()) {
        QString imageDir = imageDirs.first();
        this->snapshotDir = createSaveDir(imageDir, "snapshots");
        this->gifDir = createSaveDir(imageDir, "snapshots_gif");
    }
}

QString SnapshotConfig::createSaveDir(const QString parentDir, const QString dirname) {
    QDir dir = QDir(parentDir);
    dir.mkpath(dirname);
    dir.cd(dirname);
    if (dir.exists()) {
        return dir.absolutePath();
    }
    return QString();
}
