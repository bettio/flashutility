#include "ubootenvupdateoperation.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

#include <HemeraCore/Literals>

#define FW_SETENV_PATH "/usr/sbin/fw_setenv"
#define FW_ENV_CONFIG "/etc/fw_env.config"

class UBootEnvUpdateOperation::Private
{
public:
    Private()
        : process(nullptr),
          success(false)
    {}
    QHash<QString, QString> updates;
    QHash<QString, QString>::iterator keyValueIterator;
    QProcess *process;
    bool success;
};

UBootEnvUpdateOperation::UBootEnvUpdateOperation(const QString &id, QObject *parent)
    : RootOperation(id, parent)
    , d(new Private)
{
}

UBootEnvUpdateOperation::~UBootEnvUpdateOperation()
{
    delete d;
}

void UBootEnvUpdateOperation::startVarUpdate(const QString &key, const QString &value)
{
    QStringList args;
    args << key;
    if (!value.isEmpty()) {
        args << value;
    }

    qDebug() << "Launching: " FW_SETENV_PATH " " << args;
    d->process->start(QStringLiteral(FW_SETENV_PATH), args);
}

void UBootEnvUpdateOperation::startImpl()
{
    if (parameters().contains(QStringLiteral("environment"))) {
        QJsonObject ubootEnvUpdate = parameters().value(QStringLiteral("environment")).toObject();
        for (QJsonObject::const_iterator i = ubootEnvUpdate.constBegin(); i != ubootEnvUpdate.constEnd(); ++i) {
            d->updates.insert(i.key(), i.value().toString());
        }
    } else if (parameters().contains(QStringLiteral("file"))) {
        QFile sourceFile(parameters().value(QStringLiteral("file")).toString());
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                                 QStringLiteral("Error: Could not read from source file."));
            return;
        }

        while (!sourceFile.atEnd()) {
            QString environmentEntry = QString::fromLatin1(sourceFile.readLine()).remove(QLatin1Char('\n'));
            if (environmentEntry.contains(QLatin1Char('='))) {
                d->updates.insert(environmentEntry.mid(0, environmentEntry.indexOf(QLatin1Char('='))),
                                  environmentEntry.mid(environmentEntry.indexOf(QLatin1Char('=')) + 1));
            }
        }
    }

    if (!QFile::exists(QStringLiteral(FW_SETENV_PATH))) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: " FW_SETENV_PATH " is not installed."));
        return;
    }

    if (!QFile::exists(QStringLiteral(FW_ENV_CONFIG))) {
        setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::unhandledRequest()),
                             QStringLiteral("Error: Cannot read u-boot environment config."));
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
        d->keyValueIterator++;

        //TODO: this is highly inefficient, we should just send to fw_setenv -s a tab separated input
        if (d->success && (d->keyValueIterator != d->updates.end())) {
            startVarUpdate(d->keyValueIterator.key(), d->keyValueIterator.value());
        } else if (d->success && (d->keyValueIterator == d->updates.end())) {
            setFinished();
        } else {
            setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest()),
                                 QStringLiteral("Failed to update environment var."));
                                 return;
        }
    });

    if (!d->updates.isEmpty()) {
        d->keyValueIterator = d->updates.begin();
        startVarUpdate(d->keyValueIterator.key(), d->keyValueIterator.value());

    } else {
        qWarning() << "No variables to update! I guess something is wrong...";
        setFinished();
    }
}

ROOT_OPERATION_WORKER(UBootEnvUpdateOperation, "com.ispirata.Hemera.FlashUtility.UBootEnvUpdateOperation")
