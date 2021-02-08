#include "flashkobsoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#define KOBS_PATH "/usr/bin/kobs-ng"

class FlashKobsOperation::Private
{
public:
    Private()
        : process(nullptr),
          searchExponent(2),
          success(false)
    {}
    QString device;
    QString image;
    QProcess *process;
    int searchExponent;
    bool success;
};

FlashKobsOperation::FlashKobsOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

FlashKobsOperation::~FlashKobsOperation()
{
    delete d;
}

void FlashKobsOperation::startImpl()
{
    d->device = parameters().value(QStringLiteral("target")).toString();
    d->image = parameters().value(QStringLiteral("source")).toString();
    d->searchExponent = parameters().value(QStringLiteral("search_exponent")).toInt(2);

    if (!QFile::exists(d->image)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: binary file %1 does not exists").arg(d->device));
        return;
    }

    if (!QFile::exists(d->device)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: NAND device %1 does not exists").arg(d->device));
        return;
    }

    d->process = new QProcess(this);
    d->process->setWorkingDirectory(QStringLiteral("/tmp"));

    QStringList args {QStringLiteral("init"), QStringLiteral("-x"), d->image, QStringLiteral("--search_exponent=%1").arg(d->searchExponent), QStringLiteral("-v") };

    connect(d->process, &QProcess::readyReadStandardOutput, this, [this] () {
        qDebug() << d->process->readAllStandardOutput();
    });
    connect(d->process, &QProcess::readyReadStandardError, this, [this] () {
        qDebug() << d->process->readAllStandardError();
    });
    QObject::connect(d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        d->success = ((exitStatus == QProcess::NormalExit) && (exitCode == 0));
        if (d->success) {
            setFinished();
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Failed to flash kobs image."));
                                 return;
        }
    });
    qDebug() << "Launching: " KOBS_PATH " " << args;
    d->process->start(QStringLiteral(KOBS_PATH), args);
}

ROOT_OPERATION_WORKER(FlashKobsOperation, "com.ispirata.Hemera.FlashUtility.FlashKobsOperation")
