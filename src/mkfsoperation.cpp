#include "mkfsoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonObject>

#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#define MKFS_PATH "/sbin/mkfs."

Q_LOGGING_CATEGORY(mkfsOperationDC, "com.ispirata.Hemera.FlashUtility.Logging.MkfsOperation")

class MkfsOperation::Private
{
public:
    Private() {}

    QString device;
    QString filesystem;
    QString label;
};

MkfsOperation::MkfsOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

MkfsOperation::~MkfsOperation()
{
    delete d;
}

void MkfsOperation::startImpl()
{
    d->device = parameters().value(QStringLiteral("target")).toString();
    d->filesystem = parameters().value(QStringLiteral("filesystem")).toString();
    d->label = parameters().value(QStringLiteral("filesystem_label")).toString();

    qCInfo(mkfsOperationDC) << "Starting mkfs operation to " << d->device << " using " << d->filesystem << " and label " << d->label;

    if (!QFile::exists(d->device)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: Target device %1 does not exists").arg(d->device));
        return;
    }

    // Detach and ignore output.
    QProcess *mkfs = new QProcess(this);
    connect(mkfs, &QProcess::readyReadStandardOutput, [mkfs] () {
        qDebug() << mkfs->readAllStandardOutput();
    });
    connect(mkfs, &QProcess::readyReadStandardError, [mkfs] () {
        qDebug() << mkfs->readAllStandardError();
    });
    QObject::connect(mkfs, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
            setFinished();
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Failed to flash image."));
        }
    });

    QStringList mkfsArgs;

    if (parameters().contains(QStringLiteral("filesystem_label"))) {
        if (d->filesystem.startsWith(QStringLiteral("ext"))) {
            mkfsArgs.append(QStringLiteral("-L"));
        } else {
            mkfsArgs.append(QStringLiteral("-n"));
        }
        mkfsArgs.append(d->label);
    }

    mkfsArgs.append(d->device);

    qDebug() << "Launching: " MKFS_PATH << d->filesystem << " " << mkfsArgs;
    mkfs->start(QStringLiteral("%1%2").arg(QStringLiteral(MKFS_PATH), d->filesystem), mkfsArgs);
}

ROOT_OPERATION_WORKER(MkfsOperation, "com.ispirata.Hemera.FlashUtility.MkfsOperation")
