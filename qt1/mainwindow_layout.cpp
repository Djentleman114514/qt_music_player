#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

// 建立主窗口的自适应布局。
// 原来的 .ui 文件只负责创建控件，这里负责决定控件之间的位置关系。
void MainWindow::setupMainLayout()
{
    // 避免窗口缩得太小时控件互相挤压，同时保留正常缩放能力。
    setMinimumSize(960, 640);
    resize(1200, 800);

    // 当前菜单栏、工具栏和状态栏没有实际内容，隐藏后界面更加简洁。
    ui->menuBar->hide();
    ui->mainToolBar->hide();
    ui->statusBar->hide();

    // ==================== 整体纵向结构 ====================
    // 从上到下依次为：标题栏、主要内容区、底部播放控制区。
    QVBoxLayout *rootLayout = new QVBoxLayout(ui->centralWidget);
    rootLayout->setContentsMargins(18, 14, 18, 16);
    rootLayout->setSpacing(14);

    // ==================== 顶部标题栏 ====================
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setSpacing(8);

    QLabel *appTitle = new QLabel("Qt Music Player", ui->centralWidget);
    appTitle->setObjectName("appTitle");

    topLayout->addWidget(appTitle);
    topLayout->addStretch();
    topLayout->addWidget(ui->labelTheme);
    topLayout->addWidget(ui->comboTheme);

    rootLayout->addLayout(topLayout);

    // ==================== 中间主要内容区 ====================
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(14);

    // ---------- 左侧：歌单和歌曲列表 ----------
    QFrame *leftPanel = new QFrame(ui->centralWidget);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setMinimumWidth(260);
    leftPanel->setMaximumWidth(340);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(14, 14, 14, 14);
    leftLayout->setSpacing(10);

    QHBoxLayout *playlistSelectLayout = new QHBoxLayout;
    playlistSelectLayout->addWidget(ui->labelPlaylist);
    playlistSelectLayout->addWidget(ui->comboPlaylist, 1);
    leftLayout->addLayout(playlistSelectLayout);

    QHBoxLayout *playlistEditLayout = new QHBoxLayout;
    playlistEditLayout->setSpacing(6);
    playlistEditLayout->addWidget(ui->btnNewPlaylist);
    playlistEditLayout->addWidget(ui->btnRenamePlaylist);
    playlistEditLayout->addWidget(ui->btnDeletePlaylist);
    leftLayout->addLayout(playlistEditLayout);

    leftLayout->addWidget(ui->btnAddToPlaylist);
    leftLayout->addWidget(ui->lineSearchMusic);

    QHBoxLayout *favoriteLayout = new QHBoxLayout;
    favoriteLayout->addWidget(ui->btnFavorite);
    favoriteLayout->addWidget(ui->checkFavoriteOnly);
    favoriteLayout->addStretch();
    leftLayout->addLayout(favoriteLayout);

    // 列表占据左侧面板中剩余的大部分空间。
    leftLayout->addWidget(ui->listMusic, 1);

    QHBoxLayout *listEditLayout = new QHBoxLayout;
    listEditLayout->setSpacing(8);
    listEditLayout->addWidget(ui->btnRemoveMusic);
    listEditLayout->addWidget(ui->btnClearMusic);
    leftLayout->addLayout(listEditLayout);

    QHBoxLayout *openLayout = new QHBoxLayout;
    openLayout->setSpacing(8);
    openLayout->addWidget(ui->btnOpenFolder);
    openLayout->addWidget(ui->btnOpen);
    leftLayout->addLayout(openLayout);

    // ---------- 中间：当前歌曲信息 ----------
    QFrame *nowPlayingPanel = new QFrame(ui->centralWidget);
    nowPlayingPanel->setObjectName("nowPlayingPanel");
    nowPlayingPanel->setMinimumWidth(250);
    nowPlayingPanel->setMaximumWidth(350);

    QVBoxLayout *nowPlayingLayout = new QVBoxLayout(nowPlayingPanel);
    nowPlayingLayout->setContentsMargins(18, 18, 18, 18);
    nowPlayingLayout->setSpacing(10);
    nowPlayingLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *nowPlayingTitle = new QLabel("正在播放", nowPlayingPanel);
    nowPlayingTitle->setObjectName("sectionTitle");
    nowPlayingTitle->setAlignment(Qt::AlignCenter);

    ui->labelCover->setFixedSize(240, 240);
    ui->labelCover->setAlignment(Qt::AlignCenter);

    ui->labelTitle->setWordWrap(true);
    ui->labelArtist->setWordWrap(true);
    ui->labelAlbum->setWordWrap(true);
    ui->labelTitle->setAlignment(Qt::AlignCenter);
    ui->labelArtist->setAlignment(Qt::AlignCenter);
    ui->labelAlbum->setAlignment(Qt::AlignCenter);

    nowPlayingLayout->addWidget(nowPlayingTitle);
    nowPlayingLayout->addSpacing(6);
    nowPlayingLayout->addWidget(ui->labelCover, 0, Qt::AlignHCenter);
    nowPlayingLayout->addSpacing(8);
    nowPlayingLayout->addWidget(ui->labelTitle);
    nowPlayingLayout->addWidget(ui->labelArtist);
    nowPlayingLayout->addWidget(ui->labelAlbum);
    nowPlayingLayout->addStretch();

    // ---------- 右侧：歌词 ----------
    QFrame *lyricsPanel = new QFrame(ui->centralWidget);
    lyricsPanel->setObjectName("lyricsPanel");
    lyricsPanel->setMinimumWidth(320);

    QVBoxLayout *lyricsLayout = new QVBoxLayout(lyricsPanel);
    lyricsLayout->setContentsMargins(14, 14, 14, 14);
    lyricsLayout->setSpacing(10);

    QLabel *lyricsTitle = new QLabel("歌词", lyricsPanel);
    lyricsTitle->setObjectName("sectionTitle");
    lyricsTitle->setAlignment(Qt::AlignCenter);

    lyricsLayout->addWidget(lyricsTitle);
    lyricsLayout->addWidget(ui->listLyrics, 1);

    // 3:3:5 表示窗口变宽时，歌词区域获得更多空间。
    contentLayout->addWidget(leftPanel, 3);
    contentLayout->addWidget(nowPlayingPanel, 3);
    contentLayout->addWidget(lyricsPanel, 5);

    rootLayout->addLayout(contentLayout, 1);

    // ==================== 底部播放控制区 ====================
    QFrame *playerPanel = new QFrame(ui->centralWidget);
    playerPanel->setObjectName("playerPanel");

    QVBoxLayout *playerLayout = new QVBoxLayout(playerPanel);
    playerLayout->setContentsMargins(18, 12, 18, 12);
    playerLayout->setSpacing(8);

    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->setSpacing(12);
    progressLayout->addWidget(ui->sliderProgress, 1);
    progressLayout->addWidget(ui->labelTime);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(10);
    controlLayout->addStretch();
    controlLayout->addWidget(ui->btnPrev);
    controlLayout->addWidget(ui->btnPlay);
    controlLayout->addWidget(ui->btnNext);
    controlLayout->addWidget(ui->btnStop);
    controlLayout->addStretch();
    controlLayout->addWidget(ui->btnPlayMode);
    controlLayout->addWidget(ui->btnVolume);

    ui->btnPlay->setMinimumWidth(92);
    ui->btnPrev->setMinimumWidth(78);
    ui->btnNext->setMinimumWidth(78);
    ui->btnStop->setMinimumWidth(78);
    ui->btnPlayMode->setMinimumWidth(100);
    ui->btnVolume->setFixedSize(38, 38);

    playerLayout->addLayout(progressLayout);
    playerLayout->addLayout(controlLayout);

    rootLayout->addWidget(playerPanel);

    // 音量面板是悬浮控件，不加入布局；需要显示时仍由原来的代码定位。
    ui->frameVolumePopup->raise();
}
