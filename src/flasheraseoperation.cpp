#include "flasheraseoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#define FLASH_ERASE_PATH "/usr/sbin/flash_erase"

class FlashEraseOperation::Private
{
public:
    Private()
        : isJFFS2(false),
          success(false),
          process(nullptr)
    {}
    bool isJFFS2;
    QString startBlock;
    int blockCount;
    bool success;
    QString device;
    QProcess *process;
};

FlashEraseOperation::FlashEraseOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

FlashEraseOperation::~FlashEraseOperation()
{
    delete d;
}

void FlashEraseOperation::startImpl()
{
    d->isJFFS2 = parameters().value(QStringLiteral("jffs2")).toBool(false);
    d->startBlock = parameters().value(QStringLiteral("start")).toString();
    d->blockCount = parameters().value(QStringLiteral("count")).toInt();
    d->device = parameters().value(QStringLiteral("target")).toString();

    d->process = new QProcess(this);

    QStringList args;
    if (d->isJFFS2) {
        args << QStringLiteral("--jffs2");
    }
    args << d->device;
    args << d->startBlock;
    args << QString::number(d->blockCount);

    QObject::connect(d->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        d->success = ((exitStatus == QProcess::NormalExit) && (exitCode == 0));
        if (d->success) {
            setFinished();
        } else {
            setFinishedWithError(QStringLiteral("flash_erase_failed"),
                                 QStringLiteral("Failed to erase flash."));
                                 return;
        }
    });
    connect(d->process, &QProcess::readyReadStandardOutput, this, [this] () {
        qDebug() << d->process->readAllStandardOutput();
    });
    connect(d->process, &QProcess::readyReadStandardError, this, [this] () {
        qDebug() << d->process->readAllStandardError();
    });

    qDebug() << "Launching: " FLASH_ERASE_PATH " " << args;
    d->process->start(QStringLiteral(FLASH_ERASE_PATH), args);
}

ROOT_OPERATION_WORKER(FlashEraseOperation, "com.ispirata.Hemera.FlashUtility.FlashEraseOperation")
