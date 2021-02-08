#include "fallbackwindow.h"

#include <HemeraCore/Application>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtGui/QBackingStore>
#include <QtGui/QExposeEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

#define BACKGROUND_IMG_PATH QStringLiteral("/boot/flash.png")

FallbackWindow::FallbackWindow()
    : QWindow(),
    m_update_pending(false),
    m_showProgress(false), m_progress(0),
    m_showAnimation(false), m_busyAnimationStep(0),
    m_busyAnimationTimer(nullptr)
{
    create();
    m_backingStore = new QBackingStore(this);

    setGeometry(0, 0, 320, 240);

    if (QFile::exists(BACKGROUND_IMG_PATH)) {
        m_backgroundImage.load(BACKGROUND_IMG_PATH);
    }
}

FallbackWindow::~FallbackWindow()
{
}

bool FallbackWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        m_update_pending = false;
        renderNow();
        return true;
    }
    return QWindow::event(event);
}

void FallbackWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

void FallbackWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    m_backingStore->resize(resizeEvent->size());
    if (isExposed())
        renderNow();
}

void FallbackWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed()) {
        renderNow();
    }
}

void FallbackWindow::renderNow()
{
    if (!isExposed())
        return;

    QRect rect(0, 0, width(), height());
    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();
    QPainter painter(device);

    painter.fillRect(0, 0, width(), height(), Qt::black);
    render(&painter);

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

void FallbackWindow::render(QPainter *painter)
{
    if (!m_backgroundImage.isNull()) {
        painter->drawImage(QRect(0, 0, width(), height()), m_backgroundImage);
    }

    painter->setPen(Qt::white);
    QRect textBounds;
    painter->drawText(QRect(0, 0, width(), height()), Qt::AlignCenter | Qt::TextWordWrap, m_text, &textBounds);
    if (!m_icon.isNull()) {
        painter->drawImage(QRect((width() - 64) / 2, textBounds.y() - (64 + 12), 64, 64), m_icon);
    }

    if (m_showProgress) {
        painter->drawRect(30, height() - 60, width() - 60, 20);
        painter->fillRect(34, height() - 55, ((float) (width() - 67) / 100.0) * m_progress, 11, Qt::white);
    }

    if (m_showAnimation) {
        painter->setRenderHint(QPainter::Antialiasing);

        painter->save();
        QRectF bounds(0, 0, 48, 48);
        painter->translate((width() - 48) / 2 + 24, height() - 116 + 24);
        painter->rotate(m_busyAnimationStep);
        painter->translate(-24, -24);

        QPen pen;
        pen.setCapStyle(Qt::RoundCap);
        pen.setWidth(4);

        QLinearGradient gradient;
        gradient.setStart((bounds.x() + bounds.width()) / 2, bounds.y());
        gradient.setFinalStop((bounds.x() + bounds.width()) / 2, bounds.y() + bounds.height());
        gradient.setColorAt(0, "#000000");
        gradient.setColorAt(1, "#FFFFFF");

        QBrush brush(gradient);
        pen.setBrush(brush);
        painter->setPen(pen);

        QRectF rectA = QRectF(bounds.x() + pen.widthF() / 2.0, bounds.y() + pen.widthF() / 2.0, bounds.width() - pen.widthF(), bounds.height() - pen.widthF());
        painter->drawArc(rectA, 90 * 16, 0.65 * -360 * 16);
        painter->restore();
    }
}


void FallbackWindow::showBusyAnimation(bool showAnimation)
{
    if (m_showAnimation != showAnimation) {
        m_showAnimation = showAnimation;
        if (m_showAnimation) {
            m_busyAnimationTimer = new QTimer(this);
            m_busyAnimationTimer->setInterval(100);
            m_busyAnimationTimer->start();
            connect(m_busyAnimationTimer, &QTimer::timeout, this, &FallbackWindow::updateBusyAnimation);
        } else {
            m_busyAnimationTimer->deleteLater();
            m_busyAnimationTimer = nullptr;
        }
        renderLater();
    }
}

void FallbackWindow::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        renderLater();
    }
}

void FallbackWindow::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        renderLater();
    }
}

void FallbackWindow::loadIcon(const QString &icon)
{
    QString iconPath = icon;
    if (iconPath.startsWith(QStringLiteral("resource:///"))) {
        iconPath.remove(0, strlen("resource:///"));
        iconPath = Hemera::Application::resourcePath(iconPath, Hemera::Application::ResourceDomain::Defaults);
    } else {
        iconPath = icon;
    }

    m_icon.load(iconPath);
    renderLater();
}

void FallbackWindow::loadImage(const QString &image)
{
    m_icon.load(image);
    renderLater();
}

void FallbackWindow::showProgress(bool showP)
{
    if (m_showProgress != showP) {
        m_showProgress = showP;
        renderLater();
    }
}

void FallbackWindow::setProgress(int progress)
{
    if (m_progress != progress) {
        m_progress = qMin(100, qMax(0, progress));
        renderLater();
    }
}

void FallbackWindow::updateBusyAnimation()
{
    m_busyAnimationStep = (m_busyAnimationStep + 10) % 360;
    renderLater();
}
