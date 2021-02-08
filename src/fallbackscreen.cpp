#include "fallbackscreen.h"
// simpleapplicationproperties.h is generated at build time
#include "simpleapplicationproperties.h"

#include "flashtool.h"
#include "fallbackwindow.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

#define PARTIAL_FLASH_FILE QStringLiteral("/ramdisk/boot/partial_flash")

FallbackScreen::FallbackScreen()
    : Hemera::GuiApplication(new SimpleApplicationProperties)
{
}

FallbackScreen::~FallbackScreen()
{
    m_window->deleteLater();
}

void FallbackScreen::initImpl()
{
    setReady();
}

void FallbackScreen::startImpl()
{
    m_window = new FallbackWindow();
    setMainWindow(m_window);

    FlashTool::Mode executionMode;
    // Get correct executionMode
    if (QFile::exists(PARTIAL_FLASH_FILE)) {
        executionMode = FlashTool::Mode::PartialFlash;
    } else {
        executionMode = FlashTool::Mode::FullFlash;
    }

    FlashTool *tool = new FlashTool(executionMode, this);
    m_window->setText(QStringLiteral("Starting flashing and restore utilty"));
    connect(tool, &FlashTool::statusUpdate, this, &FallbackScreen::displayJson);
    tool->parseConfig();

    setStarted();
}

void FallbackScreen::stopImpl()
{
    setStopped();
}

void FallbackScreen::prepareForShutdown()
{
    setReadyForShutdown();
}

void FallbackScreen::displayJson(const QJsonObject &obj)
{
    m_window->setText(obj.value(QStringLiteral("message")).toString());
    QJsonValue iconValue = obj.value(QStringLiteral("iconUrl"));
    if (!iconValue.isUndefined()) {
        m_window->loadIcon(iconValue.toString());
    }
    QJsonValue progressValue = obj.value(QStringLiteral("progress"));
    if (!progressValue.isUndefined()) {
        m_window->showProgress(true);
        m_window->setProgress(progressValue.toInt(0));
    } else {
        m_window->showProgress(false);
    }
    QJsonValue busyValue = obj.value(QStringLiteral("busy"));
    if (!busyValue.isUndefined()) {
        m_window->showBusyAnimation(busyValue.toBool(false));
    } else {
        m_window->showBusyAnimation(false);
    }
}

HEMERA_GUI_APPLICATION_MAIN(FallbackScreen)
