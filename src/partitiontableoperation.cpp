#include "partitiontableoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include <HemeraCore/Literals>

#include <unistd.h>

#define FDISK_PATH "/sbin/fdisk"
#define GDISK_PATH "/sbin/gdisk"

class PartitionTableOperation::Private
{
public:
    Private() {}

    QString device;
    QString type;
    QJsonArray partitions;
};

PartitionTableOperation::PartitionTableOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

PartitionTableOperation::~PartitionTableOperation()
{
    delete d;
}

void PartitionTableOperation::startImpl()
{
    d->device = parameters().value(QStringLiteral("target")).toString();
    d->type = parameters().value(QStringLiteral("table_type")).toString();
    d->partitions = parameters().value(QStringLiteral("partitions")).toArray();

    if (!QFile::exists(d->device)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: %1 disk does not exists").arg(d->device));
        return;
    }

    QStringList commands;
    commands.append(QStringLiteral("o"));
    bool extendedCreated = false;
    int primaryPartitionsCreated = 0;
    // Now, each partition!
    for (const QJsonValue &partitionValue : d->partitions) {
        QJsonObject partition = partitionValue.toObject();
        commands.append(QStringLiteral("n"));
        if (d->type == QStringLiteral("msdos") || d->type == QStringLiteral("mbr")) {
            if (partition.value(QStringLiteral("partition_type")).toString() == QStringLiteral("msdos_extended")) {
                extendedCreated = true;
                commands.append(QStringLiteral("e"));
                if (primaryPartitionsCreated < 3) {
                    // Let fdisk handle the primary partition number here, as we simply don't care.
                    commands.append(QString());
                }
                ++primaryPartitionsCreated;
            } else if (!extendedCreated) {
                commands.append(QStringLiteral("p"));
                if (primaryPartitionsCreated < 3) {
                    commands.append(partition.value(QStringLiteral("target")).toString().mid(8).split(QLatin1Char('p')).last());
                }
                ++primaryPartitionsCreated;
            } else {
                // If we created an extended partition, we have to create a logical one.
                if (primaryPartitionsCreated < 4) {
                    // If all primary partitions are filled, fdisk won't ask us.
                    commands.append(QStringLiteral("l"));
                }
                // We can't decide the partition number.
            }
        }
        // set start sector, QString() (default) otherwise if not set
        QString startSectString;
        if (partition.contains(QStringLiteral("start_sector"))) {
            startSectString = QString::number((qint64) partition.value(QStringLiteral("start_sector")).toDouble());
        }
        commands.append(startSectString);
        // Size
        commands.append(QStringLiteral("+%1M").arg(partition.value(QStringLiteral("size")).toInt()));
        if (d->type == QStringLiteral("gpt")) {
            // FIXME: We have to implement partition types.
            commands.append(QString());
        } else if (partition.contains(QStringLiteral("partition_type")) &&
            partition.value(QStringLiteral("partition_type")).toString() != QStringLiteral("msdos_extended")) {
            // Set the correct type in fdisk, as explicitly requested
            commands.append(QStringLiteral("t"));
            // if we have just one partition it doesn't ask us anything
            if (primaryPartitionsCreated > 1) {
                commands.append(partition.value(QStringLiteral("target")).toString().mid(strlen("/dev/xyz")).split(QLatin1Char('p')).last());
            }
            commands.append(partition.value(QStringLiteral("partition_type")).toString());
        }
    }
    // Write it down.
    commands.append(QStringLiteral("w"));

    // Detach and ignore output.
    QProcess *fdisk = new QProcess(this);
    connect(fdisk, &QProcess::readyReadStandardOutput, [fdisk] () {
        qDebug() << fdisk->readAllStandardOutput();
    });
    connect(fdisk, &QProcess::readyReadStandardError, [fdisk] () {
        qDebug() << fdisk->readAllStandardError();
    });
    QObject::connect(fdisk, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
            // flush all pending writes to disk
            sync();
            // FIXME: worakround useful to give udev some seconds to scan for changes
            QTimer::singleShot(5000, this, [this] () {
                setFinished();
            });
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Failed to write partition table."));
        }
    });
    qDebug() << "Launching: " FDISK_PATH " " << QStringList { d->device, d->type };
    if (d->type == QStringLiteral("msdos") || d->type == QStringLiteral("mbr")) {
        fdisk->start(QStringLiteral(FDISK_PATH), QStringList { d->device });
    } else if (d->type == QStringLiteral("gpt")) {
        fdisk->start(QStringLiteral(GDISK_PATH), QStringList { d->device });
    } else {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Partition table type %1 is not supported!").arg(d->type));
        return;
    }

    // fdisk is now interactive. We have to feed its stdin.
    for (const QString &command : commands) {
        fdisk->write(QStringLiteral("%1\n").arg(command).toLatin1());
    }
}

ROOT_OPERATION_WORKER(PartitionTableOperation, "com.ispirata.Hemera.FlashUtility.PartitionTableOperation")
