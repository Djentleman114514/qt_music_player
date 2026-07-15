#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMediaPlayer>
#include <QStringList>
#include <QVector>
#include <QMap>


// 系统托盘图标
class QSystemTrayIcon;

// 托盘右键菜单中的操作
class QAction;

// 延时隐藏音量悬浮面板使用的定时器
class QTimer;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    QString m_currentMusicPath;    //当前选择的音乐路径
    QMediaPlayer *m_player;         // 音乐播放器对象
    QStringList m_musicList;        //保存所有歌曲路径
    int m_currentIndex;         // 当前播放第几首
    int m_playMode;    // 播放模式：0列表循环，1单曲循环，2顺序播放，3随机播放

    QVector<int> m_shuffleOrder;   // 随机播放顺序
    int m_shufflePos;              // 当前播放到随机序列的第几个位置
    void generateShuffleOrder();   // 生成随机播放顺序


    void playNextByMode();   // 根据当前播放模式播放下一首
    void playPrevByMode();   // 根据当前播放模式播放上一首

    // 根据 m_playMode 更新播放模式按钮上的文字和提示。
    void updatePlayModeButton();

    void playMusicByIndex(int index);  //声明函数：根据歌曲下标播放某一首歌

    void loadMusicList(const QStringList &filePaths);  // 加载播放列表

    void removeMusicAtIndex(int index);  // 根据下标从播放列表移除歌曲

    void savePlaylist();  // 保存播放列表
    void loadPlaylist();  // 读取播放列表

    // ==================== 多歌单管理 ====================
    // m_musicList 始终表示当前正在使用的歌单；这个映射保存所有歌单及其歌曲路径。
    QMap<QString, QStringList> m_playlists;

    // 单独保存歌单名称顺序，避免 QMap 按名称排序后改变界面中的显示顺序。
    QStringList m_playlistNames;

    // 当前正在查看和播放的歌单名称。
    QString m_currentPlaylistName;

    // 初始化新建、重命名、删除、切换和复制歌曲等歌单操作。
    void setupPlaylistManager();

    // 创建一个新的空歌单。
    void createPlaylist();

    // 重命名当前歌单。
    void renameCurrentPlaylist();

    // 删除当前歌单；至少保留一个歌单。
    void deleteCurrentPlaylist();

    // 将选中的歌曲复制到另一个歌单，不会从当前歌单移除。
    void addSelectedMusicToPlaylist();

    // 切换当前歌单，并让 m_musicList 和界面显示新歌单的内容。
    void switchPlaylist(const QString &playlistName);

    // 把当前 m_musicList 的最新内容写回 m_playlists。
    void saveCurrentPlaylistToMap();

    // 根据 m_musicList 重新生成界面中的歌曲项目。
    void rebuildMusicListWidget();

    // ==================== 播放列表拖拽排序 ====================
    // 开启 QListWidget 内部拖动，并监听拖动完成信号。
    void setupPlaylistDragDrop();

    // 把拖动后的界面顺序同步回 m_musicList 和当前歌单。
    void syncMusicListFromWidget();

    // 搜索或只看收藏时存在隐藏项目，因此需要暂时关闭拖拽排序。
    void updateDragReorderAvailability();

    // 保存所有收藏歌曲的绝对路径
    QStringList m_favoriteList;

    // 判断某个歌曲路径是否已经收藏
    bool isFavorite(const QString &filePath) const;

    // 收藏或取消收藏当前歌曲
    void toggleCurrentFavorite();

    // 从 QSettings 读取收藏列表
    void loadFavorites();

    // 将收藏列表保存到 QSettings
    void saveFavorites();

    // 根据当前歌曲更新收藏按钮的文字和状态
    void updateFavoriteButton();

    // 同时处理“歌曲名称搜索”和“只看收藏”
    void refreshMusicFilter();

    qint64 m_pendingPosition;  // 等待恢复的播放位置

    void updateMusicMetaData();  //歌曲元数据

    void updateAlbumCover();  //封面

    void clearMusicInfo();  //清空歌曲信息

    struct LyricLine
    {
        qint64 time;

        QString text;
    };

    QVector<LyricLine> m_lyrics;

    int m_currentLyricIndex;

    void loadLyrics();  //加载当前歌曲对应的 .lrc 文件

    void updateLyric(qint64 position);  //根据当前播放时间寻找歌词

    void clearLyrics();  //清空歌词

    // 系统托盘图标对象
    QSystemTrayIcon *m_trayIcon;

    // 托盘菜单中的“播放/暂停”操作
    // 保存为成员变量，是为了在播放状态变化时修改显示文字
    QAction *m_trayPlayAction;

    // 创建系统托盘图标及右键菜单
    void setupSystemTray();

    bool m_forceQuit;  // 是否确认退出程序

    // 设置明亮主题或暗黑主题
    void applyTheme(int themeIndex);

    // 从 QSettings 中读取上次使用的主题
    void loadTheme();

    // ==================== 悬浮音量控制 ====================
    // 初始化音量按钮、悬浮滑块、静音切换和延时隐藏逻辑。
    void setupVolumeControl();

    // 将音量面板移动到按钮上方并显示。
    void showVolumePopup();

    // 鼠标离开后稍作延时再隐藏，方便鼠标从按钮移动到滑块。
    void scheduleHideVolumePopup();

    // 根据静音状态和当前音量更新按钮图标及提示文字。
    void updateVolumeButton();

    // 判断鼠标当前是否位于音量按钮或悬浮面板内部。
    bool isMouseInsideVolumeControl() const;

    // 防止鼠标经过按钮与面板之间的短距离时，面板立即消失。
    QTimer *m_volumeHideTimer;

};

#endif // MAINWINDOW_H
