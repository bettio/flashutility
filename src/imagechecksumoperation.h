#ifndef IMAGECHECKSUM_OPERATION_
#define IMAGECHECKSUM_OPERATION_

#include <HemeraCore/Operation>

class ImageChecksumOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(ImageChecksumOperation)

public:
    explicit ImageChecksumOperation(const QString &imageFile, const QString &expectedChecksum, QObject *parent = nullptr);
    virtual ~ImageChecksumOperation();

protected:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

#endif
