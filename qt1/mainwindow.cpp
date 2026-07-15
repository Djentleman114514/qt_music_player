#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt 文件与路径
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QFile>
#include <QTextStream>

// Qt 界面控件
#include <QMessageBox>
#include <QSlider>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QAbstractItemView>
#include <QPushButton>
#include <QLineEdit>
#include <QPixmap>
#include <QImage>
#include <QSystemTrayIcon>


// Qt 事件与工具
#include <QMouseEvent>
#include <QStyle>
#include <QSettings>
#include <QCloseEvent>
#include <QRegularExpression>


// Qt 多媒体
#include <QMediaContent>
#include <QMediaMetaData>

// C++ 标准库
#include <algorithm>
#include <random>
#include <ctime>
#include <functional>

//拖动进度条
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 音量按钮和悬浮面板使用同一个事件过滤器。
    // 鼠标进入任意相关控件时显示面板，离开后再延时判断是否隐藏。
    bool isVolumeControl =
            watched == ui->btnVolume
            || watched == ui->frameVolumePopup
            || watched == ui->sliderVolume
            || watched == ui->labelVolume;

    if(isVolumeControl)
    {
        if(event->type() == QEvent::Enter)
        {
            showVolumePopup();
        }
        else if(event->type() == QEvent::Leave)
        {
            scheduleHideVolumePopup();
        }
    }

    if (watched == ui->sliderProgress && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            int value = QStyle::sliderValueFromPosition(
                        ui->sliderProgress->minimum(),
                        ui->sliderProgress->maximum(),
                        mouseEvent->pos().x(),
                        ui->sliderProgress->width()
                        );

            ui->sliderProgress->setValue(value);
            m_player->setPosition(value);

            return false ;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

// 关闭事件
void MainWindow::closeEvent(QCloseEvent *event)
{
    savePlaylist();

    // 从托盘菜单点击“退出程序”时，直接关闭
    if (m_forceQuit) {
        event->accept();
        return;
    }

    QMessageBox messageBox(this);

    messageBox.setWindowTitle("关闭程序");
    messageBox.setText("请选择关闭窗口后的操作：");
    messageBox.setIcon(QMessageBox::Question);

    // 添加“最小化到托盘”按钮
    QPushButton *minimizeButton = messageBox.addButton(
                "最小化到托盘",
                QMessageBox::AcceptRole);

    // 添加“关闭程序”按钮
    QPushButton *closeButton = messageBox.addButton(
                "关闭程序",
                QMessageBox::DestructiveRole);

    // 添加“取消”按钮
    QPushButton *cancelButton = messageBox.addButton(
                "取消",
                QMessageBox::RejectRole);

    // 如果系统托盘不可用，就不能选择最小化到托盘
    if (!m_trayIcon || !m_trayIcon->isVisible()) {
        minimizeButton->setEnabled(false);
    }

    messageBox.exec();

    if (messageBox.clickedButton() == minimizeButton) {
        // 隐藏主窗口，程序继续在系统托盘运行
        hide();
        event->ignore();
    }
    else if (messageBox.clickedButton() == closeButton) {
        // 确认关闭程序
        m_forceQuit = true;
        event->accept();
    }
    else if (messageBox.clickedButton() == cancelButton) {
        // 取消关闭
        event->ignore();
    }
    else {
        // 点击提示窗口右上角关闭按钮时，也取消关闭
        event->ignore();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
