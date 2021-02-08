#ifndef TOOL_OPERATION_
#define TOOL_OPERATION_

#include <HemeraCore/RootOperation>

class ToolOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(ToolOperation)

public:
    explicit ToolOperation(const QString &id, QObject *parent = nullptr);
    virtual ~ToolOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
