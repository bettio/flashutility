#ifndef PARTITIONTABLE_OPERATION_
#define PARTITIONTABLE_OPERATION_

#include <HemeraCore/RootOperation>

class QJsonArray;

class PartitionTableOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PartitionTableOperation)

public:
    explicit PartitionTableOperation(const QString &id, QObject *parent = nullptr);
    virtual ~PartitionTableOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
