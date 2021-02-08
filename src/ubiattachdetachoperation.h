#ifndef UBIATTACHDETACH_OPERATION_
#define UBIATTACHDETACH_OPERATION_

#include <HemeraCore/RootOperation>

class QProcess;

class UBIAttachDetachOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(UBIAttachDetachOperation)

public:
    explicit UBIAttachDetachOperation(const QString &id, QObject *parent = nullptr);
    virtual ~UBIAttachDetachOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;

    QProcess *attachMTD(int mtd);
    QProcess *detachMTD(int mtd);
};

#endif
