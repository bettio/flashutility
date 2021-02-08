#include "ubiattachdetachoperation.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include <HemeraCore/Literals>

#define UBIATTACH_PATH "/usr/sbin/ubiattach"
#define UBIDETACH_PATH "/usr/sbin/ubidetach"
#define UBIMKVOL_PATH "/usr/sbin/ubimkvol"
#define UBIUPDATEVOL_PATH "/usr/sbin/ubiupdatevol"

Q_LOGGING_CATEGORY(ubiUpdateLog, "com.ispirata.Hemera.FlashUtility.Logging.UBIAttachDetachOperation")

class UBIAttachDetachOperation::Private
{
public:
    QString device;
    int parentMTD;

    Private() {}
};

UBIAttachDetachOperation::UBIAttachDetachOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

UBIAttachDetachOperation::~UBIAttachDetachOperation()
{
    delete d;
}

void UBIAttachDetachOperation::startImpl()
{
    bool validParentMTD;

    d->device = parameters().value(QStringLiteral("target")).toString();
    QString parentDevice = parameters().value(QStringLiteral("parent_device")).toString();
    d->parentMTD = QString(parentDevice).remove(QStringLiteral("/dev/mtd")).toInt(&validParentMTD);

    QProcess *mtdProcess;

    if (!validParentMTD) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::badRequest()), QStringLiteral("Unable to detect a valid MTD device from %1").arg(parentDevice));
        return;
    }

    if (parameters().value(QStringLiteral("type")).toString() == QStringLiteral("ubi_detach")) {
        mtdProcess = detachMTD(d->parentMTD);
    } else {
        mtdProcess = attachMTD(d->parentMTD);
    }

    connect(mtdProcess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
            setFinished();
        } else {
            qCWarning(ubiUpdateLog) << "Error: failed to attach/detach MTD: " << d->parentMTD << " exit status: " << exitStatus << ", code: " << exitCode;
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Failed to attach/detach MTD (%1)").arg(d->parentMTD));
        }
    });
}

QProcess *UBIAttachDetachOperation::attachMTD(int mtd)
{
    QProcess *ubiAttach = new QProcess(this);
    ubiAttach->setProgram(QStringLiteral(UBIATTACH_PATH));
    ubiAttach->setArguments(QStringList { QStringLiteral("-m"), QString::number(mtd) });

    connect(ubiAttach, &QProcess::readyReadStandardOutput, ubiAttach, [ubiAttach] () {
        qCDebug(ubiUpdateLog) << "attach vol: " << ubiAttach->readAllStandardOutput();
    });
    connect(ubiAttach, &QProcess::readyReadStandardError, ubiAttach, [ubiAttach] () {
        qCDebug(ubiUpdateLog) << "attach vol: " << ubiAttach->readAllStandardError();
    });

    QTimer::singleShot(0, ubiAttach, [ubiAttach]() {
        qCDebug(ubiUpdateLog) << "Launching: " UBIATTACH_PATH " " << ubiAttach->arguments();
        ubiAttach->start();
    });

    return ubiAttach;
}

QProcess *UBIAttachDetachOperation::detachMTD(int mtd)
{
    QProcess *ubiDetach = new QProcess(this);
    ubiDetach->setProgram(QStringLiteral(UBIDETACH_PATH));
    ubiDetach->setArguments(QStringList { QStringLiteral("-m"), QString::number(mtd) });

    connect(ubiDetach, &QProcess::readyReadStandardOutput, ubiDetach, [ubiDetach] () {
        qCDebug(ubiUpdateLog) << "detach vol: " << ubiDetach->readAllStandardOutput();
    });
    connect(ubiDetach, &QProcess::readyReadStandardError, ubiDetach, [ubiDetach] () {
        qCDebug(ubiUpdateLog) << "detach vol: " << ubiDetach->readAllStandardError();
    });

    QTimer::singleShot(0, ubiDetach, [ubiDetach]() {
        qCDebug(ubiUpdateLog) << "Launching: " UBIDETACH_PATH " " << ubiDetach->arguments();
        ubiDetach->start();
    });

    return ubiDetach;
}

ROOT_OPERATION_WORKER(UBIAttachDetachOperation, "com.ispirata.Hemera.FlashUtility.UBIAttachDetachOperation")
