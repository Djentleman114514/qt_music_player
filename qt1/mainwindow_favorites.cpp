#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>

// 这个函数只在当前 cpp 文件内部使用
// 作用是把路径转换为统一的绝对路径
namespace
{
QString normalizedMusicPath(const QString &filePath)
{
    // 防止空字符串被转换成程序当前目录
    if(filePath.trimmed().isEmpty())
    {
        return QString();
    }

    // 转成绝对路径，并清理路径中的多余符号
    return QDir::cleanPath(
                QFileInfo(filePath).absoluteFilePath()
                );
}
}


// 判断一首歌是否已经收藏
bool MainWindow::isFavorite(const QString &filePath) const
{
    QString targetPath = normalizedMusicPath(filePath);

    if(targetPath.isEmpty())
    {
        return false;
    }

    for(const QString &favoritePath : m_favoriteList)
    {
        QString savedPath = normalizedMusicPath(favoritePath);

        // Windows 文件路径不区分大小写，因此这里使用 CaseInsensitive
        if(QString::compare(
                    targetPath,
                    savedPath,
                    Qt::CaseInsensitive
                    ) == 0)
        {
            return true;
        }
    }

    return false;
}


// 收藏或者取消收藏当前歌曲
void MainWindow::toggleCurrentFavorite()
{
    // 当前没有歌曲时，不能进行收藏
    if(m_currentMusicPath.isEmpty())
    {
        return;
    }

    QString currentPath =
            normalizedMusicPath(m_currentMusicPath);

    bool removed = false;

    // 如果当前歌曲已经收藏，就把它从收藏列表中删除
    for(int i = 0; i < m_favoriteList.size(); i++)
    {
        QString favoritePath =
                normalizedMusicPath(m_favoriteList[i]);

        if(QString::compare(
                    currentPath,
                    favoritePath,
                    Qt::CaseInsensitive
                    ) == 0)
        {
            m_favoriteList.removeAt(i);
            removed = true;
            break;
        }
    }

    // 如果原来没有收藏，就添加到收藏列表
    if(!removed)
    {
        m_favoriteList.append(currentPath);
    }

    // 保存新的收藏状态
    saveFavorites();

    // 更新收藏按钮
    updateFavoriteButton();

    // 如果正在“只看收藏”，需要立即刷新歌曲显示
    refreshMusicFilter();
}


// 保存收藏列表
void MainWindow::saveFavorites()
{
    QSettings settings("Tom", "QtMusicPlayer");

    // favorites 是保存收藏列表时使用的键名
    settings.setValue("favorites", m_favoriteList);
}


// 读取收藏列表
void MainWindow::loadFavorites()
{
    QSettings settings("Tom", "QtMusicPlayer");

    QStringList savedFavorites =
            settings.value("favorites").toStringList();

    m_favoriteList.clear();

    for(const QString &filePath : savedFavorites)
    {
        QString normalizedPath =
                normalizedMusicPath(filePath);

        if(normalizedPath.isEmpty())
        {
            continue;
        }

        // 防止设置文件中出现重复路径
        if(!isFavorite(normalizedPath))
        {
            m_favoriteList.append(normalizedPath);
        }
    }
}


// 更新收藏按钮状态
void MainWindow::updateFavoriteButton()
{
    // 没有当前歌曲时禁用收藏按钮
    if(m_currentMusicPath.isEmpty())
    {
        ui->btnFavorite->setEnabled(false);
        ui->btnFavorite->setText("☆ 收藏");
        ui->btnFavorite->setToolTip("请先选择歌曲");
        return;
    }

    ui->btnFavorite->setEnabled(true);

    if(isFavorite(m_currentMusicPath))
    {
        ui->btnFavorite->setText("★ 已收藏");
        ui->btnFavorite->setToolTip("点击取消收藏");
    }
    else
    {
        ui->btnFavorite->setText("☆ 收藏");
        ui->btnFavorite->setToolTip("点击收藏当前歌曲");
    }
}


// 刷新歌曲筛选结果
void MainWindow::refreshMusicFilter()
{
    // 读取搜索框中的文字
    QString keyword =
            ui->lineSearchMusic->text().trimmed();

    // 判断是否勾选了“只看收藏”
    bool favoriteOnly =
            ui->checkFavoriteOnly->isChecked();

    for(int i = 0; i < m_musicList.size(); i++)
    {
        QListWidgetItem *item =
                ui->listMusic->item(i);

        if(item == nullptr)
        {
            continue;
        }

        // 判断歌曲名称是否符合搜索文字
        bool matchesSearch =
                item->text().contains(
                    keyword,
                    Qt::CaseInsensitive
                    );

        // 没有勾选“只看收藏”时，所有歌曲都符合收藏条件
        // 勾选后，只有收藏歌曲符合条件
        bool matchesFavorite =
                !favoriteOnly
                || isFavorite(m_musicList[i]);

        // 必须同时符合搜索条件和收藏条件才显示
        bool shouldShow =
                matchesSearch && matchesFavorite;

        item->setHidden(!shouldShow);
    }

    // 搜索或“只看收藏”时列表中存在隐藏项目。
    // 此时关闭拖拽，避免用户误以为只调整了可见歌曲之间的顺序。
    updateDragReorderAvailability();
}
