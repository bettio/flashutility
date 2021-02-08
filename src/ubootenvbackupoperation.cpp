#include "ubootenvbackupoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

#include <HemeraCore/CommonOperations>
#include <HemeraCore/Literals>

#define FW_PRINTENV_PATH "/usr/sbin/fw_printenv"
#define FW_ENV_CONFIG "/etc/fw_env.config"

UBootEnvBackupOperation::UBootEnvBackupOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
{
}

UBootEnvBackupOperation::~UBootEnvBackupOperation()
{
}

void UBootEnvBackupOperation::startImpl()
{
    if (!QFile::exists(QStringLiteral(FW_PRINTENV_PATH))) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: " FW_PRINTENV_PATH " is not installed."));
        return;
    }

    if (!QFile::exists(QStringLiteral(FW_ENV_CONFIG))) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: Cannot read u-boot environment config."));
        return;
    }

    QProcess *process = new QProcess(this);
    process->setProgram(QStringLiteral(FW_PRINTENV_PATH));
    connect(new Hemera::ProcessOperation(process, this), &Hemera::Operation::finished, this, [this, process] (Hemera::Operation *op) {
        if (op->isError()) {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Failed to read environment."));
            return;
        }

        // Read and store
        QFile backupFile(QStringLiteral("/tmp/u-boot_backup"));
        if (!backupFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Failed to open backup file."));
            return;
        }

        backupFile.write(process->readAllStandardOutput());
        backupFile.flush();
        setFinished();
    });
}

ROOT_OPERATION_WORKER(UBootEnvBackupOperation, "com.ispirata.Hemera.FlashUtility.UBootEnvBackupOperation")
