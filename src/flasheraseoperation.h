#ifndef FLASHERASE_OPERATION_
#define FLASHERASE_OPERATION_

#include <HemeraCore/RootOperation>

class FlashEraseOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(FlashEraseOperation)

public:
    explicit FlashEraseOperation(const QString &id, QObject *parent = nullptr);
    virtual ~FlashEraseOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
