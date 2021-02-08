#include "copyrecoveryoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#include <unistd.h>

#define RECOVERY_MOUNTPOINT "/tmp/recovery"
#define SOURCE_DIR "/ramdisk/boot"
#define MOUNT_PATH "/bin/mount"
#define UMOUNT_PATH "/bin/umount"

Q_LOGGING_CATEGORY(copyRecoveryOperationDC, "com.ispirata.Hemera.FlashUtility.Logging.MkfsOperation")

class CopyRecoveryOperation::Private
{
public:
    Private() {}

    QString device;
    QStringList files;
};

CopyRecoveryOperation::CopyRecoveryOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

CopyRecoveryOperation::~CopyRecoveryOperation()
{
    delete d;
}

void CopyRecoveryOperation::startImpl()
{
    QString diskByLabel = QStringLiteral("/dev/disk/by-label/%1").arg(parameters().value(QStringLiteral("filesystem_label")).toString());
    if (parameters().contains(QStringLiteral("filesystem_label")) && QFile::exists(diskByLabel)) {
        d->device = diskByLabel;
    } else {
        d->device = parameters().value(QStringLiteral("target")).toString();
    }

    for (const QJsonValue &value : parameters().value(QStringLiteral("files")).toArray()) {
        d->files.append(value.toString());
    }

    QDir recoveryDir(QStringLiteral(RECOVERY_MOUNTPOINT));
    if (!QFile::exists(d->device)) {
        qCWarning(copyRecoveryOperationDC) << "Device " << d->device << " does not exist!!";
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: Device %1 is not available").arg(d->device));
        return;
    }
    // Create it
    if (!recoveryDir.mkpath(QStringLiteral(RECOVERY_MOUNTPOINT))) {
        qDebug() << "Could not create directory" << RECOVERY_MOUNTPOINT << "!!";
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Could not create recovery partition mountpoint."));
        return;
    }

    // Mount the device
    QProcess *mountRecovery = new QProcess(this);
    mountRecovery->start(QStringLiteral(MOUNT_PATH), QStringList { d->device, QStringLiteral(RECOVERY_MOUNTPOINT) });
    // Wait for finished.
    mountRecovery->waitForFinished();
    if ((mountRecovery->exitStatus() != QProcess::NormalExit) || (mountRecovery->exitCode() != 0)) {
        qDebug() << "Could not mount recovery partition!!" << mountRecovery->readAll();
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Could not mount recovery partition."));
        return;
    }

    // List files in our source, and copy them into the target directory.
    for (const QString &filename : d->files) {
        QString sourceFile = QStringLiteral("%1/%2").arg(QStringLiteral(SOURCE_DIR), filename);
        QString destinationFile = QStringLiteral("%1/%2").arg(QStringLiteral(RECOVERY_MOUNTPOINT), filename);
        if (!QFile::copy(sourceFile, destinationFile)) {
            qDebug() << "Could not copy files!!" << sourceFile << destinationFile;
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Could not copy %1 to recovery partition.").arg(sourceFile));
            return;
        }
    }

    // Create "partial_flash", so that installer will know not to wipe everything!
    QString partialFlashFile = QStringLiteral("%1/partial_flash").arg(QStringLiteral(RECOVERY_MOUNTPOINT));
    QFile f(partialFlashFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write("created by Hemera installer");
        f.flush();
        f.close();
    } else {
        qWarning() << "Could not create partial_flash!! Recovery will wipe away everything!!";
    }

    // Unmount the device, and ignore possible errors.
    QProcess *unmountRecovery = new QProcess(this);
    unmountRecovery->start(QStringLiteral(UMOUNT_PATH), QStringList { d->device });
    // Wait for finished. We don't care about the outcome.
    unmountRecovery->waitForFinished();

    sync();

    setFinished();
}

ROOT_OPERATION_WORKER(CopyRecoveryOperation, "com.ispirata.Hemera.FlashUtility.CopyRecoveryOperation")
