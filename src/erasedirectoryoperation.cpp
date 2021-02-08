#include "erasedirectoryoperation.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>

#include <HemeraCore/Literals>

#include <unistd.h>

#define MOUNT_PATH "/bin/mount"
#define UMOUNT_PATH "/bin/umount"

Q_LOGGING_CATEGORY(eraseDirOperationDC, "com.ispirata.Hemera.FlashUtility.Logging.EraseDirectoryOperation")

class EraseDirectoryOperation::Private
{
public:
    Private() {}

    void removeAll(QDir baseDir, QString path);
};

EraseDirectoryOperation::EraseDirectoryOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

EraseDirectoryOperation::~EraseDirectoryOperation()
{
    delete d;
}


void EraseDirectoryOperation::Private::removeAll(QDir baseDir, QString path)
{
    QStringList splitPath;
    QString cleanPath;
    if (path.startsWith(QDir::separator())) {
        cleanPath = path.mid(1);
    } else {
        cleanPath = path;
    }
    splitPath = cleanPath.split(QDir::separator());
    QString currentTok = splitPath.at(0);

    if (splitPath.count() == 1) {
        if (baseDir.exists(currentTok)) {
            if (!baseDir.cd(currentTok)) {
                qCWarning(eraseDirOperationDC) << "Failed to change directory from " << baseDir.path() << " to " << currentTok << " for removal.";
                return;
            }
            if (!baseDir.removeRecursively()) {
                qCWarning(eraseDirOperationDC) << "Cannot remove " << baseDir.absolutePath() << " directory recursively.";
            }
            return;

        } else {
            return;
        }
    }

    QStringList dirsList;
    if (currentTok.contains(QLatin1Char('*'))) {
        dirsList = baseDir.entryList(QStringList() << currentTok, QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    } else {
        if (baseDir.exists(currentTok)) {
            dirsList << currentTok;
        } else {
            return;
        }
    }

    for (QString item : dirsList) {
       QDir nextDir(baseDir);
       if (!nextDir.cd(item)) {
           qCWarning(eraseDirOperationDC) << "Failed to change directory from " << baseDir << " to " << item;
           return;
       }
       QString newPath = cleanPath.mid(currentTok.length());

       //TODO: get rid of recursion
       removeAll(nextDir, newPath);
    }
}

void EraseDirectoryOperation::startImpl()
{
    // Create a temporary dir for mounting
    QTemporaryDir dir(QStringLiteral("/tmp/erasedirectory-XXXXXX"));
    if (!dir.isValid()) {
        qCWarning(eraseDirOperationDC) << "Could not create temporary directory: " << dir.errorString();
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Could not create mountpoint temporary directory."));
        return;
    }

    // Mount the device
    QProcess *mountDevice = new QProcess(this);

    QString diskByLabel = QStringLiteral("/dev/disk/by-label/%1").arg(parameters().value(QStringLiteral("filesystem_label")).toString());
    QString target;
    if (!diskByLabel.endsWith(QStringLiteral("/")) && QFile::exists(diskByLabel)) {
        target = diskByLabel;
    } else {
        target = parameters().value(QStringLiteral("target")).toString();
    }

    qCInfo(eraseDirOperationDC) << "Going to mount: " << target;

    if (!QFile::exists(target)) {
        qCWarning(eraseDirOperationDC) << "Target disk doesn't exist, trying to wait 10 more seconds";
        QThread::sleep(10);
    }

    mountDevice->start(QStringLiteral(MOUNT_PATH), QStringList { target, dir.path() });
    // Wait for finished.
    mountDevice->waitForFinished();
    if ((mountDevice->exitStatus() != QProcess::NormalExit) || (mountDevice->exitCode() != 0)) {
        qCWarning(eraseDirOperationDC) << "Could not mount target device for erasing!! reason: " << mountDevice->readAllStandardError();
        qCWarning(eraseDirOperationDC) << "mount exit status: " << mountDevice->exitStatus();
        qCWarning(eraseDirOperationDC) << "mount exit code: " << mountDevice->exitCode();
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Could not mount target device for erasing."));
        return;
    }

    // Get QDir
    QString relativePath = parameters().value(QStringLiteral("relative_path")).toString();
    if (!relativePath.isEmpty()) {
        qCInfo(eraseDirOperationDC) << "Going to remove: " << relativePath << " from: " << dir.path();
        // Recursively remove and live happily.
        d->removeAll(QDir(dir.path()), relativePath);

    } else {
        QDir dirToErase(QStringLiteral("%1/%2").arg(dir.path(), relativePath));
        qCInfo(eraseDirOperationDC) << "Iteratively erase directory: " << dirToErase.absolutePath();
        // We can't really erase the mountpoint. Use a different approach.
        dirToErase.setFilter(QDir::NoDotAndDotDot | QDir::Files);
        for (const QString &dirItem : dirToErase.entryList()) {
            dirToErase.remove(dirItem);
        }

        dirToErase.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
        for (const QString &dirItem : dirToErase.entryList()) {
            QDir subDir(dirToErase.absoluteFilePath(dirItem));
            subDir.removeRecursively();
        }
    }

    // Unmount the device, and ignore possible errors.
    QProcess *unmountDevice = new QProcess(this);
    unmountDevice->start(QStringLiteral(UMOUNT_PATH), QStringList { dir.path() });
    // Wait for finished. We don't care about the outcome.
    unmountDevice->waitForFinished();
    if ((unmountDevice->exitStatus() != QProcess::NormalExit) || (unmountDevice->exitCode() != EXIT_SUCCESS)) {
        qCWarning(eraseDirOperationDC) << "Umount for " << dir.path() << " has failed with exit code: " << unmountDevice->exitCode();
        unmountDevice->start(QStringLiteral(UMOUNT_PATH), QStringList { dir.path(), QStringLiteral("-o"), QStringLiteral("remount,ro") });
    }

    sync();

    setFinished();
}

ROOT_OPERATION_WORKER(EraseDirectoryOperation, "com.ispirata.Hemera.FlashUtility.EraseDirectoryOperation")
