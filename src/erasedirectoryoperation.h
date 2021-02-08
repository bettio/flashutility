#ifndef ERASEDIRECTORYCONTENTS_OPERATION_
#define ERASEDIRECTORYCONTENTS_OPERATION_

#include <HemeraCore/RootOperation>

class EraseDirectoryOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(EraseDirectoryOperation)

public:
    explicit EraseDirectoryOperation(const QString &id, QObject *parent = nullptr);
    virtual ~EraseDirectoryOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
