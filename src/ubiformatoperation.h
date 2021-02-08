#ifndef UBIFORMAT_OPERATION_
#define UBIFORMAT_OPERATION_

#include <HemeraCore/RootOperation>

class UBIFormatOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(UBIFormatOperation)

public:
    explicit UBIFormatOperation(const QString &id, QObject *parent = nullptr);
    virtual ~UBIFormatOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
