#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColor>
#include <QCursor>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QPoint>
#include <QPushButton>
#include <QRect>
#include <QSize>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>


// 初始化音量按钮、悬浮滑块和静音切换。
void MainWindow::setupVolumeControl()
{
    // 默认音量为 50%，后续 loadPlaylist() 会恢复上次保存的音量。
    m_player->setVolume(50);
    ui->sliderVolume->setValue(50);
    ui->labelVolume->setText("50%");

    // 平时只显示音量按钮，悬浮面板由鼠标进入事件控制。
    ui->frameVolumePopup->hide();
    ui->btnVolume->setText(QString());
    ui->btnVolume->setIconSize(QSize(22, 22));
    ui->btnVolume->setFocusPolicy(Qt::NoFocus);

    // 为悬浮面板增加轻微阴影，使它与播放器背景区分开。
    QGraphicsDropShadowEffect *shadow =
            new QGraphicsDropShadowEffect(ui->frameVolumePopup);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 90));
    ui->frameVolumePopup->setGraphicsEffect(shadow);

    // 鼠标离开后延迟 180 毫秒隐藏。
    // 这样鼠标从按钮向上移动到滑块时，面板不会在中途消失。
    m_volumeHideTimer = new QTimer(this);
    m_volumeHideTimer->setSingleShot(true);
    m_volumeHideTimer->setInterval(180);

    connect(m_volumeHideTimer, &QTimer::timeout, this, [=]() {
        if(!isMouseInsideVolumeControl())
        {
            ui->frameVolumePopup->hide();
        }
    });

    // 按钮、面板及其子控件都需要监听鼠标进入和离开事件。
    ui->btnVolume->installEventFilter(this);
    ui->frameVolumePopup->installEventFilter(this);
    ui->sliderVolume->installEventFilter(this);
    ui->labelVolume->installEventFilter(this);

    // 点击音量按钮，在静音和恢复声音之间切换。
    connect(ui->btnVolume, &QPushButton::clicked, this, [=]() {
        m_player->setMuted(!m_player->isMuted());
        updateVolumeButton();
    });

    // 拖动悬浮滑块时实时修改音量，并显示百分比。
    connect(ui->sliderVolume, &QSlider::valueChanged, this, [=](int value) {
        m_player->setVolume(value);
        ui->labelVolume->setText(QString::number(value) + "%");

        // 用户主动调节音量时自动退出静音，符合常见播放器的操作习惯。
        if(m_player->isMuted() && value > 0)
        {
            m_player->setMuted(false);
        }

        updateVolumeButton();
    });

    // 静音状态也可能由其他代码改变，因此统一监听后刷新按钮图标。
    connect(m_player, &QMediaPlayer::mutedChanged, this, [=](bool) {
        updateVolumeButton();
    });

    updateVolumeButton();
}


// 将音量悬浮面板定位到按钮正上方并显示。
void MainWindow::showVolumePopup()
{
    if(m_volumeHideTimer)
    {
        m_volumeHideTimer->stop();
    }

    // 面板和按钮都属于 centralWidget，因此使用同一个坐标系计算位置。
    QPoint buttonPosition =
            ui->btnVolume->mapTo(ui->centralWidget, QPoint(0, 0));

    int x = buttonPosition.x()
            + (ui->btnVolume->width() - ui->frameVolumePopup->width()) / 2;
    int y = buttonPosition.y() - ui->frameVolumePopup->height() - 6;

    // 防止窗口尺寸改变后，悬浮面板超出主窗口边缘。
    x = qMax(0, qMin(x,
                     ui->centralWidget->width()
                     - ui->frameVolumePopup->width()));
    y = qMax(0, y);

    ui->frameVolumePopup->move(x, y);
    ui->frameVolumePopup->show();
    ui->frameVolumePopup->raise();
}


// 延时隐藏悬浮面板。
void MainWindow::scheduleHideVolumePopup()
{
    if(m_volumeHideTimer)
    {
        m_volumeHideTimer->start();
    }
}


// 判断鼠标是否还停留在音量按钮或整个悬浮面板内。
bool MainWindow::isMouseInsideVolumeControl() const
{
    QPoint globalMousePosition = QCursor::pos();

    QRect buttonRect(
                ui->btnVolume->mapToGlobal(QPoint(0, 0)),
                ui->btnVolume->size()
                );

    QRect popupRect(
                ui->frameVolumePopup->mapToGlobal(QPoint(0, 0)),
                ui->frameVolumePopup->size()
                );

    return buttonRect.contains(globalMousePosition)
            || (ui->frameVolumePopup->isVisible()
                && popupRect.contains(globalMousePosition));
}


// 根据当前声音状态更新按钮图标和提示文字。
void MainWindow::updateVolumeButton()
{
    bool silent = m_player->isMuted() || m_player->volume() == 0;

    if(silent)
    {
        ui->btnVolume->setIcon(
                    style()->standardIcon(QStyle::SP_MediaVolumeMuted)
                    );
    }
    else
    {
        ui->btnVolume->setIcon(
                    style()->standardIcon(QStyle::SP_MediaVolume)
                    );
    }

    QString stateText = m_player->isMuted() ? "已静音" : "当前音量";
    ui->btnVolume->setToolTip(
                stateText + "："
                + QString::number(ui->sliderVolume->value())
                + "%\n点击切换静音，悬停调节音量"
                );
}
