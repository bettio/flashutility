#ifndef MKFS_OPERATION_
#define MKFS_OPERATION_

#include <HemeraCore/RootOperation>

class MkfsOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(MkfsOperation)

public:
    explicit MkfsOperation(const QString &id, QObject *parent = nullptr);
    virtual ~MkfsOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
