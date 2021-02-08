#ifndef NANDWRITE_OPERATION_
#define NANDWRITE_OPERATION_

#include <HemeraCore/RootOperation>

class NANDWriteOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(NANDWriteOperation)

public:
    explicit NANDWriteOperation(const QString &id, QObject *parent = nullptr);
    virtual ~NANDWriteOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
