#ifndef UBOOTENVBACKUP_OPERATION_
#define UBOOTENVBACKUP_OPERATION_

#include <QtCore/QHash>
#include <QtCore/QString>

#include <HemeraCore/RootOperation>

class UBootEnvBackupOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(UBootEnvBackupOperation)

public:
    explicit UBootEnvBackupOperation(const QString &id, QObject *parent = nullptr);
    virtual ~UBootEnvBackupOperation();

protected:
    virtual void startImpl();
};

#endif
