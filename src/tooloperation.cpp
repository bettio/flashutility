#include "tooloperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

class ToolOperation::Private
{
public:
    Private()
        : process(nullptr),
          success(false)
    {}
    QString toolPath;
    QStringList toolArgs;
    QProcess *process;
    bool success;
};

ToolOperation::ToolOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

ToolOperation::~ToolOperation()
{
    delete d;
}

void ToolOperation::startImpl()
{
    d->toolPath = parameters().value(QStringLiteral("path")).toString();
    for (const QJsonValue &argValue : parameters().value(QStringLiteral("args")).toArray()) {
        d->toolArgs.append(argValue.toString());
    }

    if (!QFile::exists(d->toolPath)) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: %1 is missing.").arg(d->toolPath));
        return;
    }

    d->process = new QProcess(this);

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
            setFinishedWithError(QStringLiteral("tool_execution_failed"),
                                 QStringLiteral("Failed to execute %1").arg(d->toolPath));
                                 return;
        }
    });
    qDebug() << "Launching: " << d->toolPath << " " << d->toolArgs;
    d->process->start(d->toolPath, d->toolArgs);
}

ROOT_OPERATION_WORKER(ToolOperation, "com.ispirata.Hemera.FlashUtility.ToolOperation")
