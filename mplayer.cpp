#include "mplayer.h"
#include <QKeyEvent>
#include <QDir>
#include <QDebug>

Mplayer::Mplayer() :
    VideoPlayer()
{
 // TODO connect finish slot
}

Mplayer::~Mplayer() {
}

void Mplayer::playFile(QString filepath) {
    paused = false;
    QStringList args;
    args.append(QString("%1").arg(filepath));
    args.append("-fs");
    args.append("-ass");
    args.append("-embeddedfonts");
    args.append("-input");
    args.append(QString("conf=%1").arg(QDir::current().absoluteFilePath("mplayer.inputConfig")));
    process.start("mplayer", args);
    //process.start(QString("mplayer -fs -ass -embeddedfonts -input conf=%1").arg(QDir::current().absoluteFilePath("mplayer.inputConfig")));
    process.waitForStarted();
    qDebug() << "process started" << process.error();
}

void Mplayer::pause() {
    if (!paused) {
        process.write("p");
    }
}

void Mplayer::unPause() {
    if (paused) {
        process.write("p");
    }
}

void Mplayer::stop() {
    //process.kill();
    process.write("q");
    paused = true;
}

// TODO doesn't work find a workaround for mplayer communication
void Mplayer::backwards() {
    process.write("b"); // left arrow key
}

void Mplayer::forwards() {
    process.write("f"); // right arrow key
}

float Mplayer::incrementVolume() {
    process.write("*");
    return -1;
}

float Mplayer::decrementVolume() {
    process.write("/");
    return -1;
}

void Mplayer::onProcessFinished(int exitCode) {
    if (exitCode == 0) {

    }
    paused = true;
}
