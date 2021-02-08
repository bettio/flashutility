#include "flashtool.h"

#include "imagechecksumoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QStorageInfo>
#include <QtCore/QTimer>
#include <QtCore/QVersionNumber>

#include <HemeraCore/CommonOperations>
#include <HemeraCore/DeviceManagement>
#include <HemeraCore/FetchSystemConfigOperation>
#include <HemeraCore/SetSystemConfigOperation>
#include <HemeraCore/RootOperationClient>

#include <unistd.h>

Q_LOGGING_CATEGORY(flashToolDC, "com.ispirata.Hemera.FlashUtility.Logging.FlashTool")

FlashTool::FlashTool(Mode mode, QObject *parent)
    : QObject(parent)
    , m_rebootWhenFinished(false)
    , m_mode(mode)
    , m_installMediaType(InstallMediaType::Other)
{
    qInfo(flashToolDC) << "Reboot when finished: " << m_rebootWhenFinished;
    qInfo(flashToolDC) << "Running in mode: " << (int) m_mode;
    qInfo(flashToolDC) << "Install media type: " << (int) m_installMediaType;
}

FlashTool::~FlashTool()
{

}

QList<Hemera::Operation *> FlashTool::prepareActions(const QJsonArray &actions)
{
    QList<Hemera::Operation *> operations;
    qDebug() << "Preparing actions!";

    for (const QJsonValue &jsonValue : actions) {
        QJsonObject action = jsonValue.toObject();

        switch (m_mode) {
            case Mode::FullFlash:
                if (!action.value(QStringLiteral("run_on_full_flash")).toBool(true)) {
                    continue;
                }
                break;
            case Mode::PartialFlash:
                if (!action.value(QStringLiteral("run_on_partial_flash")).toBool(false)) {
                    continue;
                }
                break;
        }

        switch (m_installMediaType) {
            case InstallMediaType::RecoveryPartition:
                if (!action.value(QStringLiteral("run_in_recovery_mode")).toBool(true)) {
                    qDebug() << "Skipping action since in recovery mode:" << action;
                    continue;
                }
                break;
            default:
                break;
        }

        QString actionType = action.value(QStringLiteral("type")).toString();
        qDebug() << "Will run action of type" << actionType;

        QString operationId;
        QString progressMessage;
        QString successMessage;

        if (actionType == QStringLiteral("partition_table")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.PartitionTableOperation");
            progressMessage = QStringLiteral("Writing partition table to device...");
            successMessage = QStringLiteral("Partition table written successfully.");
        } else if (actionType == QStringLiteral("dd")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.DDOperation");
            progressMessage = QStringLiteral("Writing image to memory...");
            successMessage = QStringLiteral("Image written successfully.");
        } else if (actionType == QStringLiteral("erase_directory")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.EraseDirectoryOperation");
            progressMessage = QStringLiteral("Erasing old data...");
            successMessage = QStringLiteral("Data erased successfully.");
        } else if (actionType == QStringLiteral("mkfs")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.MkfsOperation");
            progressMessage = QStringLiteral("Formatting filesystem...");
            successMessage = QStringLiteral("Filesystem formatted successfully.");
        } else if (actionType == QStringLiteral("copy_recovery")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.CopyRecoveryOperation");
            progressMessage = QStringLiteral("Creating recovery system...");
            successMessage = QStringLiteral("Recovery system created successfully.");
        } else if (actionType == QStringLiteral("ubi_attach")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBIAttachDetachOperation");
            progressMessage = QStringLiteral("Attaching UBI Device...");
            successMessage = QStringLiteral("UBI Device Attached successfully.");
        } else if (actionType == QStringLiteral("ubi_detach")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBIAttachDetachOperation");
            progressMessage = QStringLiteral("Detaching UBI Device...");
            successMessage = QStringLiteral("UBI Device Detached successfully.");
        } else if (actionType == QStringLiteral("ubiformat")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBIFormatOperation");
            progressMessage = QStringLiteral("Formatting NAND...");
            successMessage = QStringLiteral("NAND formatted successfully.");
        } else if (actionType == QStringLiteral("ubiupdatevol")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBIUpdateVolOperation");
            progressMessage = QStringLiteral("Writing image to NAND...");
            successMessage = QStringLiteral("Image written successfully.");
        } else if (actionType == QStringLiteral("nandwrite")) {
            // Do we need to erase just a portion?
            QString device = action.value(QStringLiteral("target")).toString();
            QString image = action.value(QStringLiteral("source")).toString();
            int eraseBlockSize = action.value(QStringLiteral("logical_eraseblock_size")).toInt();
            QString start = action.value(QStringLiteral("start")).toString();

            int blockCount = 0;
            if (!start.isEmpty()) {
                QFile imageFile(image);
                imageFile.open(QIODevice::ReadOnly);
                blockCount = 1 + ((imageFile.size() - 1) / eraseBlockSize);
            } else {
                start = QStringLiteral("0");
            }

            QJsonObject args {{QStringLiteral("start"),start},{QStringLiteral("count"),blockCount},
                              {QStringLiteral("target"),device},{QStringLiteral("jffs2"),false}};
            Hemera::RootOperationClient *eraseOp = new Hemera::RootOperationClient(QStringLiteral("com.ispirata.Hemera.FlashUtility.FlashEraseOperation"),
                                                                                   args, Hemera::Operation::ExplicitStartOption, this);
            operations.append(eraseOp);
            connect(eraseOp, &Hemera::Operation::started, this, [this] {
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Erasing flash...") },
                                                 { QStringLiteral("busy"), true } });
            });
            connect(eraseOp, &Hemera::Operation::finished, this, [this] {
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Flash erased successfully.") } });
            });

            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.NANDWriteOperation");
            progressMessage = QStringLiteral("Writing image to NAND...");
            successMessage = QStringLiteral("Image written successfully.");
        } else if (actionType == QStringLiteral("flash_kobs")) {
            if (!QFile::exists(action.value(QStringLiteral("source")).toString())) {
                qWarning() << "flash_kobs source file doesn't exist, skipping step";
                continue;
            }

            // Set up the flasherase part, just wipe as it's always like that.
            QString device = action.value(QStringLiteral("target")).toString();

            QJsonObject args {{QStringLiteral("start"), QStringLiteral("0")},{QStringLiteral("count"),0},
                              {QStringLiteral("target"),device},{QStringLiteral("jffs2"),false}};
            Hemera::RootOperationClient *eraseOp = new Hemera::RootOperationClient(QStringLiteral("com.ispirata.Hemera.FlashUtility.FlashEraseOperation"),
                                                                                   args, Hemera::Operation::ExplicitStartOption, this);
            operations.append(eraseOp);
            connect(eraseOp, &Hemera::Operation::started, this, [this] {
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Erasing flash...") },
                                                 { QStringLiteral("busy"), true } });
            });
            connect(eraseOp, &Hemera::Operation::finished, this, [this] {
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Flash erased successfully.") } });
            });

            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.FlashKobsOperation");
            progressMessage = QStringLiteral("Writing First-level Bootloader...");
            successMessage = QStringLiteral("Image written successfully.");
        } else if (actionType == QStringLiteral("u-boot_env_update")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBootEnvUpdateOperation");
            progressMessage = QStringLiteral("Updating boot environment...");
            successMessage = QStringLiteral("Environment updated successfully.");
        } else if (actionType == QStringLiteral("backup_u-boot_environment")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBootEnvBackupOperation");
            progressMessage = QStringLiteral("Backing up boot environment...");
            successMessage = QStringLiteral("Environment backed up successfully.");
        } else if (actionType == QStringLiteral("restore_u-boot_environment")) {
            operationId = QStringLiteral("com.ispirata.Hemera.FlashUtility.UBootEnvUpdateOperation");
            action.insert(QStringLiteral("file"), QStringLiteral("/tmp/u-boot_backup"));
            progressMessage = QStringLiteral("Restoring boot environment...");
            successMessage = QStringLiteral("Environment restored successfully.");
        } else if (actionType == QStringLiteral("checksum")) {
            ImageChecksumOperation *imageCheckOp = new ImageChecksumOperation(action.value(QStringLiteral("file")).toString(),
                                                                              action.value(QStringLiteral("checksum")).toString(),
                                                                              this);
            operations.append(imageCheckOp);
            connect(imageCheckOp, &Hemera::Operation::started, this, [this] {
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Verifying image checksum...") },
                                                 { QStringLiteral("busy"),true } });
            });
            connect(imageCheckOp, &Hemera::Operation::finished, this, [this] {
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Checksum verified successfully.") } });
            });
            continue;
        } else {
            qWarning() << actionType << "undefined/unsupported!";
            Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Configuration not validated, aborting.") },
                                             { QStringLiteral("busy"), false },
                                             { QStringLiteral("iconUrl"), QStringLiteral("resource:///images/error.png") } });
            return QList< Hemera::Operation* >();
        }

        Hemera::RootOperationClient *clientOp = new Hemera::RootOperationClient(operationId, action, Hemera::Operation::ExplicitStartOption, this);
        operations.append(clientOp);
        connect(clientOp, &Hemera::Operation::started, this, [this, progressMessage] {
            Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), progressMessage },
                                             { QStringLiteral("busy"),true } });
        });
        connect(clientOp, &Hemera::Operation::finished, this, [this, successMessage] {
            Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), successMessage },
                                             { QStringLiteral("busy"), true } });
        });
    }

    return operations;
}

void FlashTool::parseConfig()
{
    QJsonObject settings;
    QFile settingsFile(QStringLiteral("/boot/sysconfig/sysrestore.json"));
    if (settingsFile.open(QIODevice::Text | QIODevice::ReadOnly)) {
        settings = QJsonDocument::fromJson(settingsFile.readAll()).object();
    }

    if (settings.isEmpty()) {
        qWarning(flashToolDC) << "Error while parsing /boot/sysconfig/sysrestore.json";
        Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Cannot read flash tool configuration!") },
                                         { QStringLiteral("iconUrl"), QStringLiteral("resource:///images/error.png") } });
        return;
    }

    // Get our install media
    if (settings.value(QStringLiteral("has_recovery")).toBool(false)) {
        QStorageInfo installMedia(QStringLiteral("/ramdisk/boot"));
        QString installMediaDevice = QLatin1String(installMedia.device());
        qCInfo(flashToolDC) << "Install media device: " << installMediaDevice;
        QString labelEffectivePath;
        if (settings.contains(QStringLiteral("recovery_device_filesystem_label"))) {
            labelEffectivePath = QFile::symLinkTarget(QStringLiteral("/dev/disk/by-label/%1").arg(settings.value(QStringLiteral("recovery_device_filesystem_label")).toString()));
        }
        if (installMediaDevice == settings.value(QStringLiteral("recovery_device")).toString() ||
            installMediaDevice == labelEffectivePath) {
            qInfo(flashToolDC) << "Detected installation from recovery partition!! Setting install media to recovery.";
            m_installMediaType = InstallMediaType::RecoveryPartition;
            m_rebootWhenFinished = true;
        }
    }

    // Shall we check against downgrading?
    QString versionToInstall = settings.value(QStringLiteral("appliance_version")).toString(QStringLiteral("rolling"));
    if (m_installMediaType != InstallMediaType::RecoveryPartition && versionToInstall != QStringLiteral("rolling")) {
        Hemera::FetchSystemConfigOperation *applianceVersion = new Hemera::FetchSystemConfigOperation(QStringLiteral("hemera_appliance_version"), this);
        if (applianceVersion->synchronize()) {
            QString oldVersion = applianceVersion->value();
            if (oldVersion != QStringLiteral("rolling") && QVersionNumber::fromString(oldVersion) > QVersionNumber::fromString(versionToInstall)) {
                // Ouch! Attempt at downgrading!
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"),
                                                   QStringLiteral("You have installed version %1, you can't downgrade to %2!").arg(oldVersion, versionToInstall) },
                                                 { QStringLiteral("busy"), false },
                                                 { QStringLiteral("iconUrl"), QStringLiteral("resource:///images/error.png") } });
                return;
            }
        } else {
            qDebug() << "No version information available! Not checking against downgrading.";
        }
    }

    QList<Hemera::Operation *> operations = prepareActions(settings.value(QStringLiteral("actions")).toArray());
    if (operations.isEmpty()) {
        return;
    }

    qDebug() << "Loaded operations:" << operations;

    for (const QJsonValue &scriptValue : settings.value(QStringLiteral("scripts")).toArray()) {
        QJsonObject script = scriptValue.toObject();
        Hemera::RootOperationClient *toolOp = new Hemera::RootOperationClient(QStringLiteral("com.ispirata.Hemera.FlashUtility.ToolOperation"),
                                                                                script, Hemera::Operation::ExplicitStartOption, this);
        QString message = script.value(QStringLiteral("message")).toString();
        operations.append(toolOp);
        connect(toolOp, &Hemera::Operation::started, this, [this, message]() {
            Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), message }, { QStringLiteral("busy"),true } });
        });
        connect(toolOp, &Hemera::Operation::finished, this, [this] {
            Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("OK") } });
        });
    }

    // If everything goes well, also set the new installed version and remove recovery boot flag.
    operations.append(new Hemera::SetSystemConfigOperation(QStringLiteral("hemera_appliance_version"), versionToInstall,
                                                           Hemera::Operation::ExplicitStartOption, this));
    operations.append(new Hemera::SetSystemConfigOperation(QStringLiteral("hemera_recovery_boot"), QString::number(0),
                                                           Hemera::Operation::ExplicitStartOption, this));

    Hemera::SequentialOperation *flashSequence = new Hemera::SequentialOperation(operations, this);
    connect(flashSequence, &Hemera::Operation::finished, this, [this](Hemera::Operation *operation) {
        if (!operation->isError()) {
            sync();
            Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Appliance correctly installed.") },
                                             { QStringLiteral("busy"), false },
                                             { QStringLiteral("iconUrl"), QStringLiteral("resource:///images/ok.png") } });

            qCInfo(flashToolDC) << "Appliance correctly installed.";

            if (m_rebootWhenFinished) {
                QTimer::singleShot(10000, []() {
                    qCInfo(flashToolDC) << "Rebooting.";
                    // just to be sure...
                    ::sync();
                    Hemera::DeviceManagement::reboot();
                });
            }

        } else {
            if (operation->errorMessage().isEmpty()) {
                qWarning(flashToolDC) << "Failed due to error.";
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"), QStringLiteral("Failed due to error.") },
                                                 { QStringLiteral("busy"), false },
                                                 { QStringLiteral("iconUrl"), QStringLiteral("resource:///images/error.png") } });
            } else {
                qWarning(flashToolDC) << "Failed due to error: " << operation->errorName() << " " << operation->errorMessage();
                Q_EMIT statusUpdate(QJsonObject{ { QStringLiteral("message"),
                                                   QStringLiteral("Failed: %1, %2.").arg(operation->errorName(), operation->errorMessage()) },
                                                 { QStringLiteral("busy"), false },
                                                 { QStringLiteral("iconUrl"), QStringLiteral("resource:///images/error.png") } });
            }
        }
    });
}
