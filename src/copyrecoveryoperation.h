#ifndef COPYRECOVERY_OPERATION_
#define COPYRECOVERY_OPERATION_

#include <HemeraCore/RootOperation>

class CopyRecoveryOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(CopyRecoveryOperation)

public:
    explicit CopyRecoveryOperation(const QString &id, QObject *parent = nullptr);
    virtual ~CopyRecoveryOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
