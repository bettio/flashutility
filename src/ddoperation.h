#ifndef DD_OPERATION_
#define DD_OPERATION_

#include <HemeraCore/RootOperation>

class DDOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(DDOperation)

public:
    explicit DDOperation(const QString &id, QObject *parent = nullptr);
    virtual ~DDOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
