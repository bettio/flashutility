#include "ubiformatoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#define UBIFORMAT_PATH "/usr/sbin/ubiformat"

class UBIFormatOperation::Private
{
public:
    Private()
        : subpageSize(0)
        , process(nullptr)
    {}

    QString device;
    QString image;
    int subpageSize;
    QProcess *process;
};

UBIFormatOperation::UBIFormatOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

UBIFormatOperation::~UBIFormatOperation()
{
    delete d;
}

void UBIFormatOperation::startImpl()
{
    d->device = parameters().value(QStringLiteral("target")).toString();
    d->image = parameters().value(QStringLiteral("source")).toString();
    d->subpageSize = parameters().value(QStringLiteral("subpage_size")).toInt(-1);

    if (!d->image.isEmpty() && !QFile::exists(d->image)) {
        qDebug() << d->image << "does not exist!!";
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: image file does not exists."));
        return;
    }

    if (!QFile::exists(d->device)) {
        qDebug() << d->device << "does not exist!!";
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: MTD device %1 does not exists").arg(d->device));
        return;
    }

    d->process = new QProcess(this);

    QStringList args;
    args << d->device;

    if (!d->image.isEmpty()) {
        args << QStringLiteral("-f");
        args << d->image;
    }

    if (d->subpageSize > 0) {
        args << QStringLiteral("-s");
        args << QString::number(d->subpageSize);
    }

    // Non-interactive.
    args << QStringLiteral("-y");

    connect(d->process, &QProcess::readyReadStandardOutput, this, [this] () {
        qDebug() << d->process->readAllStandardOutput();
    });
    connect(d->process, &QProcess::readyReadStandardError, this, [this] () {
        qDebug() << d->process->readAllStandardError();
    });
    QObject::connect(d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
            setFinished();
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()), QStringLiteral("Failed to format MTD."));
        }
    });

    qDebug() << "Launching: " UBIFORMAT_PATH " " << args;
    d->process->start(QStringLiteral(UBIFORMAT_PATH), args);
}

ROOT_OPERATION_WORKER(UBIFormatOperation, "com.ispirata.Hemera.FlashUtility.UBIFormatOperation")
