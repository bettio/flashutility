#include "nandwriteoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#define NANDWRITE_PATH "/usr/sbin/nandwrite"

class NANDWriteOperation::Private
{
public:
    Private()
        : process(nullptr),
          success(false)
    {}
    QString device;
    QString image;
    QString startOffset;
    QProcess *process;
    bool success;
};

NANDWriteOperation::NANDWriteOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

NANDWriteOperation::~NANDWriteOperation()
{
    delete d;
}

void NANDWriteOperation::startImpl()
{
    d->device = parameters().value(QStringLiteral("target")).toString();
    d->image = parameters().value(QStringLiteral("source")).toString();
    d->startOffset = parameters().value(QStringLiteral("start")).toString();

    if (!QFile::exists(d->image)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: image file %1 does not exists").arg(d->device));
        return;
    }

    if (!QFile::exists(d->device)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: NAND device %1 does not exists").arg(d->device));
        return;
    }

    d->process = new QProcess(this);

    QStringList args;
    args << QStringLiteral("-p"); //It looks like a good idea to autopad images to blocksize. If the image is not padded it will fail.
    args << d->device;
    if (!d->startOffset.isEmpty()) {
        args << QStringLiteral("-s");
        args << d->startOffset;
    }
    args << d->image;

    connect(d->process, &QProcess::readyReadStandardOutput, this, [this] () {
        qDebug() << d->process->readAllStandardOutput();
    });
    connect(d->process, &QProcess::readyReadStandardError, this, [this] () {
        qDebug() << d->process->readAllStandardError();
    });
    QObject::connect(d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        d->success = ((exitStatus == QProcess::NormalExit) && (exitCode == 0));
        if (d->success) {
            setFinished();
        } else {
            setFinishedWithError(QStringLiteral("nandwrite_failed"),
                                 QStringLiteral("Failed to flash image."));
                                 return;
        }
    });
    qDebug() << "Launching: " NANDWRITE_PATH " " << args;
    d->process->start(QStringLiteral(NANDWRITE_PATH), args);
}

ROOT_OPERATION_WORKER(NANDWriteOperation, "com.ispirata.Hemera.FlashUtility.NANDWriteOperation")
