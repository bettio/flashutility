#ifndef UBIUPDATEVOL_OPERATION_
#define UBIUPDATEVOL_OPERATION_

#include <HemeraCore/RootOperation>

class QProcess;

class UBIUpdateVolOperation : public Hemera::RootOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(UBIUpdateVolOperation)

public:
    explicit UBIUpdateVolOperation(const QString &id, QObject *parent = nullptr);
    virtual ~UBIUpdateVolOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;

    QProcess *attachMTD(int mtd);
    QProcess *detachMTD(int mtd);
    QProcess *mkVolume(const QString &parentUBI, int volID, const QString &label, int sizeInMiB, bool immutable);
    void prepareVolume();
    void doUpdateVol();
};

#endif
