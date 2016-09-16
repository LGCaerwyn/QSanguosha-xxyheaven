#include "timed-progressbar.h"
#include "clientstruct.h"
#include <QPainter>
#include "skin-bank.h"

void TimedProgressBar::show()
{
    m_mutex.lock();
    if (!m_hasTimer || m_max <= 0) {
        m_mutex.unlock();
        return;
    }
    if (m_timer != 0) {
        killTimer(m_timer);
        m_timer = 0;
    }
    m_timer = startTimer(m_step);
    this->setMaximum(m_max);
    this->setValue(m_val);
    QProgressBar::show();
    m_mutex.unlock();
}

void TimedProgressBar::hide()
{
    m_mutex.lock();
    if (m_timer != 0) {
        killTimer(m_timer);
        m_timer = 0;
    }
    m_mutex.unlock();
    QProgressBar::hide();
}

void TimedProgressBar::timerEvent(QTimerEvent *)
{
    bool emitTimeout = false;
    bool doHide = false;
    int val = 0;
    m_mutex.lock();
    m_val += m_step;
    if (m_val >= m_max) {
        m_val = m_max;
        if (m_autoHide)
            doHide = true;
        else {
            killTimer(m_timer);
            m_timer = 0;
        }
        emitTimeout = true;
    }
    val = m_val;
    m_mutex.unlock();
    this->setValue(val);
    if (doHide) hide();
    if (emitTimeout) emit timedOut();
}

using namespace QSanProtocol;

QSanCommandProgressBar::QSanCommandProgressBar()
{
    m_step = Config.S_PROGRESS_BAR_UPDATE_INTERVAL;
    m_hasTimer = (ServerInfo.OperationTimeout != 0);
    m_instanceType = S_CLIENT_INSTANCE;
}

void QSanCommandProgressBar::setCountdown(CommandType command)
{
    m_mutex.lock();
    m_max = ServerInfo.getCommandTimeout(command, m_instanceType);
    m_mutex.unlock();
}

void QSanCommandProgressBar::paintEvent(QPaintEvent *)
{
    m_mutex.lock();
    int val = this->m_val;
    int max = this->m_max;
    m_mutex.unlock();
    int width = this->width();
    int height = this->height();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (orientation() == Qt::Vertical) {
        painter.translate(0, height);
        qSwap(width, height);
        painter.rotate(-90);
    }

    painter.drawPixmap(0, 0, width, height, G_ROOM_SKIN.getProgressBarPixmap(0));

    double percent = 1 - (double)val / (double)max;
    QRectF rect = QRectF(0, 0, percent * width, height);

    //以rect的右边为中轴，7为半径画一个椭圆
    QRectF ellipseRect;
    ellipseRect.setTopLeft(QPointF(rect.right() - 7, rect.top()));
    ellipseRect.setBottomRight(QPointF(rect.right() + 7, rect.bottom()));

    QPainterPath rectPath;
    QPainterPath ellipsePath;
    rectPath.addRect(rect);
    ellipsePath.addEllipse(ellipseRect);

    QPainterPath polygonPath = rectPath.united(ellipsePath);
    painter.setClipPath(polygonPath);

    painter.drawPixmap(0, 0, width, height, G_ROOM_SKIN.getProgressBarPixmap(100));
}

void QSanCommandProgressBar::setCountdown(Countdown countdown)
{
    m_mutex.lock();
    m_hasTimer = (countdown.type != Countdown::S_COUNTDOWN_NO_LIMIT);
    m_max = countdown.max;
    m_val = countdown.current;
    m_mutex.unlock();
}
