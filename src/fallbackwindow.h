#include <QtGui/QWindow>
#include <QtGui/QImage>

class QBackingStore;
class QExposeEvent;
class QPainter;
class QResizeEvent;
class QTimer;

class FallbackWindow : public QWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(FallbackWindow)

public:
    explicit FallbackWindow();
    virtual ~FallbackWindow();

    virtual void render(QPainter *painter);

    void setTitle(const QString &title);
    void setText(const QString &text);
    void loadIcon(const QString &icon);
    void loadImage(const QString &image);
    void showProgress(bool showP);
    void setProgress(int progress);
    void showBusyAnimation(bool showAnimation);

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void exposeEvent(QExposeEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateBusyAnimation();

private:
    QBackingStore *m_backingStore;
    bool m_update_pending;

    bool m_showProgress;
    int m_progress;
    bool m_showAnimation;
    int m_busyAnimationStep;

    QTimer *m_busyAnimationTimer;
    QString m_title;
    QImage m_icon;
    QString m_text;
    QImage m_image;
    QImage m_backgroundImage;
};
