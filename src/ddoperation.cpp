#include "ddoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include <HemeraCore/Literals>

#include <unistd.h>

#define BUNZIP2_PATH "/usr/bin/bunzip2"
#define DD_PATH "/bin/dd"

class DDOperation::Private
{
public:
    Private()
        : process(nullptr),
          success(false)
    {}
    QString device;
    QString image;
    QProcess *process;
    bool success;
};

DDOperation::DDOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

DDOperation::~DDOperation()
{
    delete d;
}

void DDOperation::startImpl()
{
    // FIXME: QFile::exists here is a workaround, we should change it
    QString diskByLabel = QStringLiteral("/dev/disk/by-label/%1").arg(parameters().value(QStringLiteral("filesystem_label")).toString());
    if (parameters().contains(QStringLiteral("filesystem_label")) && QFile::exists(diskByLabel)) {
        d->device = diskByLabel;
    } else {
        d->device = parameters().value(QStringLiteral("target")).toString();
    }
    d->image = parameters().value(QStringLiteral("source")).toString();

    d->process = new QProcess(this);

    QStringList bunzip2Args;
    QProcess *bunzip2 = nullptr;

    QStringList args;

    if (!QFile::exists(d->image)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: image file %1 does not exists").arg(d->device));
        return;
    }

    if (!QFile::exists(d->device)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: block device %1 does not exists").arg(d->device));
        return;
    }

    bool compressedInput = d->image.endsWith(QStringLiteral("bz2"));
    if (compressedInput) {
        bunzip2 = new QProcess(this);
        bunzip2->setStandardOutputProcess(d->process);
        bunzip2Args << QStringLiteral("-c");
        bunzip2Args << d->image;
        //dd is reading from stdin
    } else {
        //input is not piped here
        args << QStringLiteral("if=") + d->image;
    }

    args << QStringLiteral("of=") + d->device;
    args << QStringLiteral("bs=8192"); //FIX: find better block size

    connect(d->process, &QProcess::readyReadStandardOutput, this, [this] () {
        qDebug() << d->process->readAllStandardOutput();
    });
    connect(d->process, &QProcess::readyReadStandardError, this, [this] () {
        qDebug() << d->process->readAllStandardError();
    });
    if (bunzip2) {
        connect(bunzip2, &QProcess::readyReadStandardError, this, [this] () {
            qDebug() << d->process->readAllStandardError();
        });
    }
    QObject::connect(d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if (isFinished()) {
            //Already finished and failed due to bunzip2 error
            return;
        }
        d->success = ((exitStatus == QProcess::NormalExit) && (exitCode == 0));
        if (d->success) {
            // flush all pending writes to disk
            sync();
            // FIXME: worakround useful to give udev some seconds to scan for changes
            QTimer::singleShot(5000, this, [this] () {
                setFinished();
            });
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Failed to flash image."));
                                 return;
        }
    });

    if (compressedInput) {
        QObject::connect(bunzip2, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                         this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
            if ((exitStatus != QProcess::NormalExit) || (exitCode != 0)) {
                qDebug() << "Failed bunzip2";
                d->success = false;
                setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                    QStringLiteral("Failed to decompress image."));
                                    return;
            }
        });
        qDebug() << "Launching: " BUNZIP2_PATH " " << bunzip2Args;
        bunzip2->start(QStringLiteral(BUNZIP2_PATH), bunzip2Args);
    }

    qDebug() << "Launching: " DD_PATH " " << args;
    d->process->start(QStringLiteral(DD_PATH), args);
}

ROOT_OPERATION_WORKER(DDOperation, "com.ispirata.Hemera.FlashUtility.DDOperation")
