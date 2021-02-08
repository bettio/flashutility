#ifndef FLASHTOOL_H_
#define FLASHTOOL_H_

#include <QtCore/QObject>

class QJsonObject;

namespace Hemera
{
class Operation;
}

class FlashTool : public QObject {
    Q_OBJECT

public:
    enum class Mode {
        FullFlash,
        PartialFlash
    };

    enum class InstallMediaType {
        RecoveryPartition,
        Other
    };

    explicit FlashTool(Mode mode, QObject *parent = nullptr);
    virtual ~FlashTool();

    void parseConfig();

Q_SIGNALS:
    void statusUpdate(const QJsonObject &jsonMessage);

private:
    QList<Hemera::Operation *> prepareActions(const QJsonArray &actions);

    Mode m_mode;
    InstallMediaType m_installMediaType;
    bool m_rebootWhenFinished;
};

#endif
