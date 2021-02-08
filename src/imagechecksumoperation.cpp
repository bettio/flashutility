#include "imagechecksumoperation.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QProcess>

#include <HemeraCore/Literals>

#define SHA256SUM_PATH "/usr/bin/sha256sum"

class ImageChecksumOperation::Private
{
public:
    Private()
        : process(nullptr),
          success(false)
    {}
    QString image;
    QString expectedChecksum;
    QProcess *process;
    bool success;
};

ImageChecksumOperation::ImageChecksumOperation(const QString &imageFile, const QString &expectedChecksum, QObject *parent)
    : Operation(Operation::ExplicitStartOption, parent)
    , d(new Private)
{
    d->image = imageFile;
    d->expectedChecksum = expectedChecksum;
}

ImageChecksumOperation::~ImageChecksumOperation()
{
    delete d;
}

void ImageChecksumOperation::startImpl()
{
    QFile f(d->image);
    if (!f.open(QFile::ReadOnly)) {
        qDebug() << "Could not open file for reading " << d->image;
        setFinishedWithError(QStringLiteral("ImageChecksumVerifyFailed"),
                             QStringLiteral("Could not open %1 for reading.").arg(d->image));
        return;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f)) {
        qDebug() << "Could not read from file " << d->image;
        setFinishedWithError(QStringLiteral("ImageChecksumVerifyFailed"),
                             QStringLiteral("Could not read from %1.").arg(d->image));
        return;
    }
    if (hash.result() != d->expectedChecksum.toLatin1()) {
        qDebug() << "Calculated checksum was: " << hash.result();
        qDebug() << "Expected checksum is: " << d->expectedChecksum;
        setFinishedWithError(QStringLiteral("ImageChecksumVerifyFailed"),
                             QStringLiteral("Failed to verify image checksum."));
                             return;
    }

    setFinished();
}

