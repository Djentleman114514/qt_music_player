#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMediaContent>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUrl>


namespace
{
// 判断歌单名称是否已经存在。
// 比较时忽略大小写，避免同时出现“Study”和“study”这类容易混淆的名称。
bool playlistNameExists(const QStringList &playlistNames,
                        const QString &targetName,
                        const QString &ignoredName = QString())
{
    for(const QString &name : playlistNames)
    {
        // 重命名时忽略歌单原来的名称，否则仅改变大小写会被误判为重名。
        if(!ignoredName.isEmpty()
                && QString::compare(name, ignoredName, Qt::CaseInsensitive) == 0)
        {
            continue;
        }

        if(QString::compare(name, targetName, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }

    return false;
}
}


// 初始化多歌单界面的所有操作。
void MainWindow::setupPlaylistManager()
{
    // 用户从下拉框选择其他歌单时，立即切换当前播放队列。
    connect(ui->comboPlaylist,
            &QComboBox::currentTextChanged,
            this,
            [=](const QString &playlistName)
    {
        switchPlaylist(playlistName);
    });

    // 新建一个空歌单。
    connect(ui->btnNewPlaylist, &QPushButton::clicked, this, [=]() {
        createPlaylist();
    });

    // 修改当前歌单名称。
    connect(ui->btnRenamePlaylist, &QPushButton::clicked, this, [=]() {
        renameCurrentPlaylist();
    });

    // 删除当前歌单，但不会删除电脑中的音乐文件。
    connect(ui->btnDeletePlaylist, &QPushButton::clicked, this, [=]() {
        deleteCurrentPlaylist();
    });

    // 把当前选中的歌曲复制到其他歌单。
    connect(ui->btnAddToPlaylist, &QPushButton::clicked, this, [=]() {
        addSelectedMusicToPlaylist();
    });
}


// 将当前播放列表的最新内容保存回歌单映射。
void MainWindow::saveCurrentPlaylistToMap()
{
    if(m_currentPlaylistName.isEmpty())
    {
        return;
    }

    m_playlists[m_currentPlaylistName] = m_musicList;
}


// 使用 m_musicList 重新生成左侧歌曲列表。
void MainWindow::rebuildMusicListWidget()
{
    // 清空和重建列表期间阻止 currentRowChanged 等界面信号，避免产生中间状态。
    QSignalBlocker blocker(ui->listMusic);

    ui->listMusic->clear();

    for(const QString &musicPath : m_musicList)
    {
        QFileInfo fileInfo(musicPath);

        // 界面只显示文件名，完整路径保存在 UserRole 中。
        // 后续拖拽、双击和跨歌单复制都使用路径识别歌曲，避免同名歌曲混淆。
        QListWidgetItem *item =
                new QListWidgetItem(fileInfo.completeBaseName());
        item->setData(Qt::UserRole, musicPath);
        ui->listMusic->addItem(item);
    }

    // 如果当前歌曲仍然属于这个歌单，恢复它在界面中的选中位置。
    int currentIndex = m_musicList.indexOf(m_currentMusicPath);

    if(currentIndex >= 0)
    {
        m_currentIndex = currentIndex;
        ui->listMusic->setCurrentRow(currentIndex);
    }

    // 重建后重新应用搜索、收藏筛选和拖拽可用状态。
    refreshMusicFilter();
}


// 创建一个新的空歌单。
void MainWindow::createPlaylist()
{
    bool ok = false;

    QString playlistName = QInputDialog::getText(
                this,
                "新建歌单",
                "请输入歌单名称：",
                QLineEdit::Normal,
                "",
                &ok
                ).trimmed();

    // 用户取消输入或只输入空格时不创建歌单。
    if(!ok || playlistName.isEmpty())
    {
        return;
    }

    if(playlistNameExists(m_playlistNames, playlistName))
    {
        QMessageBox::warning(this, "无法创建", "已经存在同名歌单");
        return;
    }

    // 切换之前先记录当前歌单，防止刚刚添加或拖动的结果丢失。
    saveCurrentPlaylistToMap();

    m_playlistNames.append(playlistName);
    m_playlists.insert(playlistName, QStringList());

    // 修改下拉框时暂时屏蔽信号，最后只手动切换一次。
    {
        QSignalBlocker blocker(ui->comboPlaylist);
        ui->comboPlaylist->addItem(playlistName);
        ui->comboPlaylist->setCurrentText(playlistName);
    }

    switchPlaylist(playlistName);
}


// 重命名当前歌单，并保持它原来的显示位置和歌曲内容。
void MainWindow::renameCurrentPlaylist()
{
    if(m_currentPlaylistName.isEmpty())
    {
        return;
    }

    bool ok = false;

    QString newName = QInputDialog::getText(
                this,
                "重命名歌单",
                "请输入新的歌单名称：",
                QLineEdit::Normal,
                m_currentPlaylistName,
                &ok
                ).trimmed();

    if(!ok || newName.isEmpty() || newName == m_currentPlaylistName)
    {
        return;
    }

    if(playlistNameExists(m_playlistNames, newName, m_currentPlaylistName))
    {
        QMessageBox::warning(this, "无法重命名", "已经存在同名歌单");
        return;
    }

    saveCurrentPlaylistToMap();

    QString oldName = m_currentPlaylistName;
    QStringList songs = m_playlists.take(oldName);
    m_playlists.insert(newName, songs);

    // 只替换名称，不改变该歌单在下拉框中的顺序。
    int playlistIndex = m_playlistNames.indexOf(oldName);
    if(playlistIndex >= 0)
    {
        m_playlistNames[playlistIndex] = newName;
    }

    m_currentPlaylistName = newName;

    {
        QSignalBlocker blocker(ui->comboPlaylist);
        int comboIndex = ui->comboPlaylist->findText(oldName);

        if(comboIndex >= 0)
        {
            ui->comboPlaylist->setItemText(comboIndex, newName);
            ui->comboPlaylist->setCurrentIndex(comboIndex);
        }
    }

    savePlaylist();
}


// 删除当前歌单；只删除程序中的歌单记录，不会删除任何音乐文件。
void MainWindow::deleteCurrentPlaylist()
{
    if(m_playlistNames.size() <= 1)
    {
        QMessageBox::warning(this, "无法删除", "至少需要保留一个歌单");
        return;
    }

    int result = QMessageBox::question(
                this,
                "删除歌单",
                "确定删除歌单“" + m_currentPlaylistName
                + "”吗？\n\n只会删除歌单记录，不会删除电脑中的音乐文件。"
                );

    if(result != QMessageBox::Yes)
    {
        return;
    }

    QString deletedName = m_currentPlaylistName;
    int deletedIndex = m_playlistNames.indexOf(deletedName);

    m_playlists.remove(deletedName);
    m_playlistNames.removeAll(deletedName);

    // 优先选择被删除位置附近的歌单。
    int nextIndex = deletedIndex;
    if(nextIndex >= m_playlistNames.size())
    {
        nextIndex = m_playlistNames.size() - 1;
    }

    QString nextPlaylistName = m_playlistNames[nextIndex];

    {
        QSignalBlocker blocker(ui->comboPlaylist);
        ui->comboPlaylist->removeItem(deletedIndex);
        ui->comboPlaylist->setCurrentText(nextPlaylistName);
    }

    // 先清空名称，防止 switchPlaylist() 把刚删除的歌单重新保存回来。
    m_currentPlaylistName.clear();
    switchPlaylist(nextPlaylistName);
}


// 将当前选中的歌曲复制到另一个歌单。
void MainWindow::addSelectedMusicToPlaylist()
{
    QList<QListWidgetItem*> selectedItems = ui->listMusic->selectedItems();

    if(selectedItems.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先选择要添加的歌曲");
        return;
    }

    QStringList targetPlaylistNames;
    for(const QString &playlistName : m_playlistNames)
    {
        if(playlistName != m_currentPlaylistName)
        {
            targetPlaylistNames.append(playlistName);
        }
    }

    if(targetPlaylistNames.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先创建另一个歌单");
        return;
    }

    bool ok = false;
    QString targetName = QInputDialog::getItem(
                this,
                "添加到歌单",
                "请选择目标歌单：",
                targetPlaylistNames,
                0,
                false,
                &ok
                );

    if(!ok || targetName.isEmpty())
    {
        return;
    }

    QStringList targetSongs = m_playlists.value(targetName);
    int addedCount = 0;

    for(QListWidgetItem *item : selectedItems)
    {
        QString musicPath = item->data(Qt::UserRole).toString();

        if(!musicPath.isEmpty() && !targetSongs.contains(musicPath))
        {
            targetSongs.append(musicPath);
            addedCount++;
        }
    }

    m_playlists[targetName] = targetSongs;
    savePlaylist();

    if(addedCount > 0)
    {
        QMessageBox::information(
                    this,
                    "添加完成",
                    QString("已将 %1 首歌曲添加到“%2”")
                    .arg(addedCount)
                    .arg(targetName)
                    );
    }
    else
    {
        QMessageBox::information(this, "提示", "所选歌曲已经存在于目标歌单中");
    }
}


// 切换当前歌单，并让播放队列、列表控件和随机顺序保持一致。
void MainWindow::switchPlaylist(const QString &playlistName)
{
    if(playlistName.isEmpty()
            || !m_playlists.contains(playlistName)
            || playlistName == m_currentPlaylistName)
    {
        return;
    }

    // 保存旧歌单的最新顺序和内容。
    saveCurrentPlaylistToMap();

    m_currentPlaylistName = playlistName;
    m_musicList = m_playlists.value(playlistName);

    // 当前播放队列即当前歌单。切换时停止旧歌曲，避免下标指向新歌单中的错误歌曲。
    m_player->stop();
    m_player->setMedia(QMediaContent());

    m_currentIndex = -1;
    m_currentMusicPath.clear();
    m_pendingPosition = 0;
    m_shuffleOrder.clear();
    m_shufflePos = 0;

    rebuildMusicListWidget();

    if(!m_musicList.isEmpty())
    {
        // 默认选中第一首并加载媒体，但不自动播放。
        m_currentIndex = 0;
        m_currentMusicPath = m_musicList[0];
        ui->listMusic->setCurrentRow(0);
        m_player->setMedia(QUrl::fromLocalFile(m_currentMusicPath));

        loadLyrics();
        updateFavoriteButton();

        if(m_playMode == 3)
        {
            generateShuffleOrder();
        }
    }
    else
    {
        clearMusicInfo();
        clearLyrics();
        updateFavoriteButton();
    }

    refreshMusicFilter();
    savePlaylist();
}
