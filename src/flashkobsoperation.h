#ifndef FLASHKOBS_OPERATION_
#define FLASHKOBS_OPERATION_

#include <HemeraCore/RootOperation>

class FlashKobsOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(FlashKobsOperation)

public:
    explicit FlashKobsOperation(const QString &id, QObject *parent = nullptr);
    virtual ~FlashKobsOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
