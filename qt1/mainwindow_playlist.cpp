#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileInfo>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QListWidgetItem>
#include <QMediaContent>
#include <QMessageBox>
#include <QModelIndex>
#include <QSettings>
#include <QSignalBlocker>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>
//把用户选择的音乐文件加载到播放器列表里
void MainWindow::loadMusicList(const QStringList &filePaths)
{
    if (filePaths.isEmpty()) {
        return;
    }

    bool wasEmpty = m_musicList.isEmpty();  //在添加新歌曲之前，先判断原来的播放列表是不是空的。

    bool hasNewMusic = false; //记录这次有没有真正添加新歌曲。

    for(int i = 0;i<filePaths.size();i++)
    {
        QString filePath = QFileInfo(filePaths[i]).absoluteFilePath();  //转换为绝对路径

        //文件不存在，直接跳过
        if(!QFileInfo::exists(filePath))
        {
            continue;
        }

        //歌曲已经存在，跳过
        if(m_musicList.contains(filePath))
        {
            continue;
        }

        m_musicList.append(filePath);   // 添加到真正的播放列表

        QFileInfo fileInfo(filePath);

        // 界面显示歌曲名称，完整路径保存在 UserRole 中。
        // 拖拽排序和多歌单复制都使用路径识别歌曲，可以避免同名歌曲混淆。
        QListWidgetItem *item =
                new QListWidgetItem(fileInfo.completeBaseName());
        item->setData(Qt::UserRole, filePath);
        ui->listMusic->addItem(item);

        hasNewMusic = true;
    }

    if(!hasNewMusic)  //如果这次没有添加任何新歌曲,直接结束
    {
        QMessageBox::information(this, "提示", "选择的歌曲已经在播放列表中");
        return ;
    }

    //添加新歌曲后，更新随机播放列表
    m_shuffleOrder.clear();
    m_shufflePos = 0;

    if(m_playMode == 3)
    {
        generateShuffleOrder();
    }

    //如果原来播放列表是空，才默认选中第一首歌
    if(wasEmpty)
    {
        m_currentIndex = 0;
        m_currentMusicPath = m_musicList[0];

        ui->listMusic->setCurrentRow(0);

        // 把第一首歌交给播放器，等待播放
        m_player->setMedia(QUrl::fromLocalFile(m_currentMusicPath));

        loadLyrics();

        updateFavoriteButton();
    }

    // 如果正在只看收藏，新添加的普通歌曲应该被隐藏
    refreshMusicFilter();

    savePlaylist();  //保存当前播放列表
}

//删除列表中的歌曲
void MainWindow::removeMusicAtIndex(int index)
{
    if (index < 0 || index >= m_musicList.size()) {
        return;
    }

    bool removeCurrentMusic = (index == m_currentIndex);

    // 1. 从保存歌曲路径的列表中删除
    m_musicList.removeAt(index);

    // 2. 从界面播放列表中删除
    delete ui->listMusic->takeItem(index);

    // 3. 删除歌曲后，随机播放顺序需要重新生成
    m_shuffleOrder.clear();
    m_shufflePos = 0;

    if (m_playMode == 3 && !m_musicList.isEmpty()) {
        generateShuffleOrder();
    }

    // 4. 如果删除后列表为空
    if (m_musicList.isEmpty()) {
        m_currentIndex = -1;
        m_currentMusicPath.clear();

        ui->sliderProgress->setValue(0);
        ui->labelTime->setText("00:00 / 00:00");

        m_player->stop();
        m_player->setMedia(QMediaContent());

        clearLyrics();

        // 没有当前歌曲后，禁用收藏按钮
        updateFavoriteButton();

        // 刷新筛选结果
        refreshMusicFilter();


        savePlaylist();  //删除到空列表时，也要保存空歌单状态

        return;
    }

    // 5. 如果删除的是当前正在播放的歌曲
    if (removeCurrentMusic) {
        m_player->stop();

        if (index >= m_musicList.size()) {
            index = m_musicList.size() - 1;
        }

        m_currentIndex = index;
        m_currentMusicPath = m_musicList[index];

        ui->listMusic->setCurrentRow(index);

        loadLyrics();

        m_player->setMedia(QUrl::fromLocalFile(m_currentMusicPath));

        updateFavoriteButton();
    }

    // 6. 如果删除的是当前歌曲前面的歌曲，需要修正当前下标
    else if (index < m_currentIndex) {
        m_currentIndex--;
    }

    refreshMusicFilter();

    savePlaylist();
}


//保存全部自定义歌单、当前歌曲、音量、播放模式和播放位置
void MainWindow::savePlaylist()
{
    // m_musicList 表示当前歌单，保存设置前先把它写回总歌单映射。
    saveCurrentPlaylistToMap();

    QSettings settings("Tom", "QtMusicPlayer");

    // QVariantMap 可以让 QSettings 一次保存“歌单名称 -> 歌曲路径列表”。
    QVariantMap savedPlaylists;
    for(const QString &playlistName : m_playlistNames)
    {
        savedPlaylists.insert(
                    playlistName,
                    m_playlists.value(playlistName)
                    );
    }

    settings.setValue("customPlaylists", savedPlaylists);
    settings.setValue("playlistNames", m_playlistNames);
    settings.setValue("currentPlaylistName", m_currentPlaylistName);

    // 继续保存旧版 playlist 键，便于旧配置迁移或临时回退。
    settings.setValue("playlist", m_musicList);

    // 保存当前歌曲路径
    settings.setValue("currentMusicPath", m_currentMusicPath);

    settings.setValue("volume",ui->sliderVolume->value());
    // 单独保存静音状态，重新打开播放器后保持用户上次的选择。
    settings.setValue("muted", m_player->isMuted());
    settings.setValue("playMode",m_playMode);

    settings.setValue("position",m_player->position());

}


//加载全部自定义歌单，并恢复上次使用的歌单和播放状态
void MainWindow::loadPlaylist()
{
    QSettings settings("Tom", "QtMusicPlayer");

    QVariantMap savedPlaylists =
            settings.value("customPlaylists").toMap();

    QStringList savedPlaylistNames =
            settings.value("playlistNames").toStringList();

    m_playlists.clear();
    m_playlistNames.clear();

    // 按之前保存的显示顺序恢复每一个歌单。
    for(const QString &playlistName : savedPlaylistNames)
    {
        if(playlistName.trimmed().isEmpty())
        {
            continue;
        }

        QStringList savedSongs =
                savedPlaylists.value(playlistName).toStringList();

        QStringList validSongs;
        for(const QString &musicPath : savedSongs)
        {
            QString absolutePath =
                    QFileInfo(musicPath).absoluteFilePath();

            // 启动时跳过已经不存在的文件，并清理同一歌单中的重复路径。
            if(QFileInfo::exists(absolutePath)
                    && !validSongs.contains(absolutePath))
            {
                validSongs.append(absolutePath);
            }
        }

        m_playlistNames.append(playlistName);
        m_playlists.insert(playlistName, validSongs);
    }

    // 第一次升级到多歌单版本时没有 customPlaylists，
    // 因此把原来的单个 playlist 自动迁移到“默认歌单”，不会丢失旧列表。
    if(m_playlistNames.isEmpty())
    {
        QStringList oldPlaylist =
                settings.value("playlist").toStringList();

        QStringList validOldPlaylist;
        for(const QString &musicPath : oldPlaylist)
        {
            QString absolutePath =
                    QFileInfo(musicPath).absoluteFilePath();

            if(QFileInfo::exists(absolutePath)
                    && !validOldPlaylist.contains(absolutePath))
            {
                validOldPlaylist.append(absolutePath);
            }
        }

        m_playlistNames.append("默认歌单");
        m_playlists.insert("默认歌单", validOldPlaylist);
    }

    QString savedCurrentPlaylist =
            settings.value(
                "currentPlaylistName",
                "默认歌单"
                ).toString();

    // 配置中的当前歌单已不存在时，退回第一个有效歌单。
    if(!m_playlists.contains(savedCurrentPlaylist))
    {
        savedCurrentPlaylist = m_playlistNames.first();
    }

    m_currentPlaylistName = savedCurrentPlaylist;
    m_musicList = m_playlists.value(m_currentPlaylistName);

    // 填充下拉框时屏蔽信号，防止尚未恢复完状态就触发切换。
    {
        QSignalBlocker blocker(ui->comboPlaylist);
        ui->comboPlaylist->clear();
        ui->comboPlaylist->addItems(m_playlistNames);
        ui->comboPlaylist->setCurrentText(m_currentPlaylistName);
    }

    rebuildMusicListWidget();

    QString currentMusicPath =
            settings.value("currentMusicPath", "").toString();  //读取歌曲路径

    int volume = settings.value("volume", 50).toInt();  //读取保存的音量

    bool muted = settings.value("muted", false).toBool();  //读取上次静音状态

    int playMode = settings.value("playMode",0).toInt();  //读取保存的播放模式

    qint64 position = settings.value("position", 0).toLongLong();  //读取保存的播放位置

    // 恢复音量滑块、播放器音量、百分比和静音状态。
    ui->sliderVolume->setValue(volume);
    m_player->setVolume(volume);
    ui->labelVolume->setText(QString::number(volume) + "%");
    m_player->setMuted(muted);
    updateVolumeButton();

    // 防止保存的模式编号异常；有效范围固定为 0 到 3。
    if(playMode >= 0 && playMode < 4)
    {
        m_playMode = playMode;
    }
    else
    {
        m_playMode = 0;
    }

    // 恢复模式后立即更新按钮上显示的文字。
    updatePlayModeButton();

    if(m_musicList.isEmpty())
    {
        m_currentIndex = -1;
        m_currentMusicPath.clear();
        clearMusicInfo();
        clearLyrics();
        updateFavoriteButton();
        return ;
    }

    int index = m_musicList.indexOf(currentMusicPath);  // 根据路径寻找歌曲现在的位置

    // 上次歌曲不属于当前歌单时，默认选中第一首歌曲。
    if(index < 0 || index >= m_musicList.size())
    {
        index = 0;
    }

    if(index >= 0)
    {
        m_currentIndex = index;

        m_currentMusicPath = m_musicList[index];

        ui->listMusic->setCurrentRow(index);

        m_player->setMedia(QUrl::fromLocalFile(m_currentMusicPath));

        loadLyrics();

        updateFavoriteButton();

        if(position > 0)
        {
            m_pendingPosition = position;  // 先记录下来，等 durationChanged 再真正跳转
            m_player->pause();             //  强制调用 pause() 进行静默预加载，确保触发时长解析
        }
    }

    // 随机播放使用歌曲下标，恢复列表后需要重新生成随机顺序。
    if(m_playMode == 3)
    {
        generateShuffleOrder();
    }
}


// 初始化播放列表内部拖拽排序
void MainWindow::setupPlaylistDragDrop()
{
    // InternalMove 只允许在当前列表内部移动，不会复制出额外项目。
    ui->listMusic->setDragDropMode(QAbstractItemView::InternalMove);
    ui->listMusic->setDefaultDropAction(Qt::MoveAction);
    ui->listMusic->setDragEnabled(true);
    ui->listMusic->setAcceptDrops(true);
    ui->listMusic->setDropIndicatorShown(true);
    ui->listMusic->setDragDropOverwriteMode(false);

    // QListWidget 完成一次内部移动后，底层 model 会发送 rowsMoved 信号。
    connect(ui->listMusic->model(),
            &QAbstractItemModel::rowsMoved,
            this,
            [=](const QModelIndex &,
                int,
                int,
                const QModelIndex &,
                int)
    {
        syncMusicListFromWidget();
    });
}


// 将界面中拖拽后的顺序同步到当前歌单
void MainWindow::syncMusicListFromWidget()
{
    QStringList newMusicOrder;

    for(int row = 0; row < ui->listMusic->count(); row++)
    {
        QListWidgetItem *item = ui->listMusic->item(row);
        if(item == nullptr)
        {
            continue;
        }

        // 每个项目的 UserRole 保存歌曲完整路径，名称相同也能准确区分。
        QString musicPath = item->data(Qt::UserRole).toString();
        if(!musicPath.isEmpty())
        {
            newMusicOrder.append(musicPath);
        }
    }

    // 路径数量异常时恢复原列表，避免因为某个项目缺少路径而丢歌。
    if(newMusicOrder.size() != m_musicList.size())
    {
        QMessageBox::warning(
                    this,
                    "排序失败",
                    "部分歌曲缺少路径信息，无法保存新的顺序"
                    );
        rebuildMusicListWidget();
        return;
    }

    QString currentPath = m_currentMusicPath;
    m_musicList = newMusicOrder;

    // 通过路径重新寻找当前歌曲，保证拖动后继续播放正确歌曲。
    m_currentIndex = m_musicList.indexOf(currentPath);
    if(m_currentIndex >= 0)
    {
        ui->listMusic->setCurrentRow(m_currentIndex);
    }

    // 随机播放顺序保存的是旧下标，列表变化后必须重新生成。
    m_shuffleOrder.clear();
    m_shufflePos = 0;

    if(m_playMode == 3 && !m_musicList.isEmpty())
    {
        generateShuffleOrder();
    }

    savePlaylist();
}


// 搜索或只看收藏时存在隐藏项目，因此筛选状态下暂时禁止拖拽
void MainWindow::updateDragReorderAvailability()
{
    bool isFiltering =
            !ui->lineSearchMusic->text().trimmed().isEmpty()
            || ui->checkFavoriteOnly->isChecked();

    if(isFiltering)
    {
        ui->listMusic->setDragDropMode(QAbstractItemView::NoDragDrop);
        ui->listMusic->setToolTip("搜索或收藏筛选状态下不能调整歌曲顺序");
    }
    else
    {
        ui->listMusic->setDragDropMode(QAbstractItemView::InternalMove);
        ui->listMusic->setToolTip("可以拖动歌曲调整播放顺序");
    }
}
