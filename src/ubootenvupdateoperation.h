#ifndef UBOOTENVUPDATE_OPERATION_
#define UBOOTENVUPDATE_OPERATION_

#include <QtCore/QHash>
#include <QtCore/QString>

#include <HemeraCore/RootOperation>

class UBootEnvUpdateOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(UBootEnvUpdateOperation)

public:
    explicit UBootEnvUpdateOperation(const QString &id, QObject *parent = nullptr);
    virtual ~UBootEnvUpdateOperation();

protected:
    virtual void startImpl();

private:
    void startVarUpdate(const QString &key, const QString &value);

    class Private;
    Private * const d;
};

#endif
