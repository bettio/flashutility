#include "ubiupdatevoloperation.h"

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

Q_LOGGING_CATEGORY(ubiUpdateLog, "com.ispirata.Hemera.FlashUtility.Logging.UBIUpdateVolOperation")

class UBIUpdateVolOperation::Private
{
public:
    QString device;
    QString image;
    QString parentUBI;
    QString name;
    int parentMTD;
    int sizeInMiB;
    bool immutable;
    bool validParentMTD;
    bool needToDetachMTD;

    Private()
        : parentMTD(-1)
        , sizeInMiB(-1)
        , immutable(false)
        , validParentMTD(false)
        , needToDetachMTD(false)
    {}
};

UBIUpdateVolOperation::UBIUpdateVolOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

UBIUpdateVolOperation::~UBIUpdateVolOperation()
{
    delete d;
}

void UBIUpdateVolOperation::startImpl()
{
    bool validParentMTD;

    d->device = parameters().value(QStringLiteral("target")).toString();
    d->image = parameters().value(QStringLiteral("source")).toString();
    d->name = parameters().value(QStringLiteral("name")).toString();
    QString parentDevice = parameters().value(QStringLiteral("parent_device")).toString();
    d->parentMTD = QString(parentDevice).remove(QStringLiteral("/dev/mtd")).toInt(&d->validParentMTD);
    d->sizeInMiB = parameters().value(QStringLiteral("size")).toInt();
    d->immutable = parameters().value(QStringLiteral("immutable")).toBool();

    if (d->sizeInMiB < 1) {
        qCWarning(ubiUpdateLog) << "Error: size in MiB cannot be less than 1, size: " << d->sizeInMiB;
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Size in MiB cannot be less than 1"));
        return;
    }

    if (!d->image.isEmpty() && !QFile::exists(d->image)) {
        qCWarning(ubiUpdateLog) << "Image file does not exists: " << d->image;
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Image file does not exist"));
        return;

    } else if (d->image.isEmpty()) {
        qCDebug(ubiUpdateLog) << "An empty volume will be created.";
    }

    if (!d->device.contains(QLatin1Char('_'))) {
        qCWarning(ubiUpdateLog) << "Error: specified parent UBI volume is not valid. device: " << d->device;
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Specified UBI volume is not valid"));
        return;
    }

    d->parentUBI = d->device.split(QLatin1Char('_')).first();
    if (!QFile::exists(d->parentUBI)) {
        qCDebug(ubiUpdateLog) << d->parentUBI << " doesn't exists. Need to attach it.";

        if (!d->validParentMTD) {
            qCWarning(ubiUpdateLog) << "Error: cannot identify MTD number, parent_device: " << parentDevice;
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Cannot identify MTD number"));
            return;
        }

        d->needToDetachMTD = true;
        connect(attachMTD(d->parentMTD), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
            if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
                prepareVolume();
            } else {
                qCWarning(ubiUpdateLog) << "Error: failed to attach MTD: " << d->parentMTD << " exit status: " << exitStatus << ", code: " << exitCode;
                setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Failed to attach MTD"));
                return;
            }
        });
    } else {
        d->needToDetachMTD = false;
        prepareVolume();
    }
}

void UBIUpdateVolOperation::prepareVolume()
{
    if (QFile::exists(d->device)) {
        qCDebug(ubiUpdateLog) << "No need to create UBI volume";
        doUpdateVol();
    } else {
        bool ok;
        int volID = QString(d->device).remove(d->parentUBI + QLatin1Char('_')).toInt(&ok);
        if (!ok) {
            qWarning(ubiUpdateLog) << "Error: invalid UBI volume ID: device: " << d->device << " parent UBI: " << d->parentUBI;
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Invalid UBI volume ID"));
            return;
        }
        connect(mkVolume(d->parentUBI, volID, d->name, d->sizeInMiB, d->immutable), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
            if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
                qCDebug(ubiUpdateLog) << "UBI volume succesfully created";
                doUpdateVol();
            } else {
                qWarning(ubiUpdateLog) << "Failed to create UBI volume: " << d->device << " exit status: " << exitStatus << ", code: " << exitCode;
                if (d->needToDetachMTD) {
                    connect(detachMTD(d->parentMTD), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
                        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Failed to create UBI volume"));
                    });
                }
            }
        });
    }
}

void UBIUpdateVolOperation::doUpdateVol()
{
    QProcess *ubiUpdateVol = new QProcess(this);

    connect(ubiUpdateVol, &QProcess::readyReadStandardOutput, ubiUpdateVol, [ubiUpdateVol] () {
        qCDebug(ubiUpdateLog) << "update vol: " << ubiUpdateVol->readAllStandardOutput();
    });
    connect(ubiUpdateVol, &QProcess::readyReadStandardError, ubiUpdateVol, [ubiUpdateVol] () {
        qCDebug(ubiUpdateLog) << "update vol: " << ubiUpdateVol->readAllStandardError();
    });

    connect(ubiUpdateVol, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
            qCDebug(ubiUpdateLog) << "update vol succesfully finished";
            if (d->needToDetachMTD) {
                connect(detachMTD(d->parentMTD), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
                    if (!((exitStatus == QProcess::NormalExit) && (exitCode == 0))) {
                        qCWarning(ubiUpdateLog) << "Error: failed to detach MTD: " << d->parentMTD << " exit status: " << exitStatus << ", code: " << exitCode;
                    }
                    setFinished();
                });
            } else {
                setFinished();
            }
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Failed to update UBI volume"));
        }
    });

    if (d->image.isEmpty()) {
        qCDebug(ubiUpdateLog) << "Launching: " UBIUPDATEVOL_PATH " " << d->device;
        ubiUpdateVol->start(QStringLiteral(UBIUPDATEVOL_PATH), QStringList { d->device });
    } else {
        qCDebug(ubiUpdateLog) << "Launching: " UBIUPDATEVOL_PATH " " << d->device << " " << d->image;
        ubiUpdateVol->start(QStringLiteral(UBIUPDATEVOL_PATH), QStringList { d->device, d->image });
    }
}

QProcess *UBIUpdateVolOperation::attachMTD(int mtd)
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

QProcess *UBIUpdateVolOperation::detachMTD(int mtd)
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

QProcess *UBIUpdateVolOperation::mkVolume(const QString &parentUBI, int volID, const QString &name, int sizeInMiB, bool immutable)
{
    QString label = name.isEmpty() ? QStringLiteral("vol%1").arg(volID) : name;

    QProcess *ubiMkVol = new QProcess(this);
    ubiMkVol->setProgram(QStringLiteral(UBIMKVOL_PATH));
    ubiMkVol->setArguments(QStringList { parentUBI,
                                         QStringLiteral("-N"), label,
                                         QStringLiteral("-n"), QString::number(volID),
                                         QStringLiteral("-s"), QString::number(sizeInMiB) + QStringLiteral("MiB"),
                                         QStringLiteral("-t"), immutable ? QStringLiteral("static") : QStringLiteral("dynamic")});

    connect(ubiMkVol, &QProcess::readyReadStandardOutput, ubiMkVol, [ubiMkVol] () {
        qCDebug(ubiUpdateLog) << "mk vol: " << ubiMkVol->readAllStandardOutput();
    });
    connect(ubiMkVol, &QProcess::readyReadStandardError, ubiMkVol, [ubiMkVol] () {
        qCDebug(ubiUpdateLog) << "mk vol: " << ubiMkVol->readAllStandardError();
    });

    QTimer::singleShot(0, ubiMkVol, [ubiMkVol]() {
        qCDebug(ubiUpdateLog) << "Launching: " UBIMKVOL_PATH " " << ubiMkVol->arguments();
        ubiMkVol->start();
    });

    return ubiMkVol;
}

ROOT_OPERATION_WORKER(UBIUpdateVolOperation, "com.ispirata.Hemera.FlashUtility.UBIUpdateVolOperation")
