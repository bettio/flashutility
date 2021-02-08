#ifndef FALLBACKSCREEN_H
#define FALLBACKSCREEN_H

#include <HemeraGui/GuiApplication>

class QJsonObject;
class FallbackWindow;

class FallbackScreen : public Hemera::GuiApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(FallbackScreen)

public:
    FallbackScreen();
    virtual ~FallbackScreen();

protected:
    virtual void initImpl() override final;
    virtual void startImpl() override final;
    virtual void stopImpl() override final;
    virtual void prepareForShutdown() override final;

private Q_SLOTS:
    void displayJson(const QJsonObject &jsonText);

private:

    FallbackWindow *m_window;
};

#endif // FALLBACKSCREEN_H

