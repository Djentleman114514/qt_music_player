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


// Qt 事件与工具
#include <QMouseEvent>
#include <QStyle>
#include <QSettings>
#include <QCloseEvent>
#include <QRegularExpression>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>


// Qt 多媒体
#include <QMediaContent>
#include <QMediaMetaData>

// C++ 标准库
#include <algorithm>
#include <random>
#include <ctime>
#include <functional>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 先建立自适应布局，再初始化播放器和各项交互功能。
    setupMainLayout();

    // 初始化托盘相关指针
    m_trayIcon = 0;
    m_trayPlayAction = 0;

    ui->sliderProgress->installEventFilter(this);

    this->setWindowTitle("Qt music player");

    this->resize(1200,800);

    //创建播放器对象
    m_player = new QMediaPlayer(this);

    // 播放器创建完成后，初始化系统托盘
    setupSystemTray();

    //当前歌曲元数据发生变化时，更新歌曲信息
    connect(m_player,QOverload<>::of(&QMediaPlayer::metaDataChanged),this,[=](){

            updateMusicMetaData();
    });

    m_currentIndex = -1;

    m_currentLyricIndex = -1;

    m_playMode = 0;   // 播放模式默认是列表循环

    m_shufflePos = 0;   // 随机播放序列当前位置

    m_pendingPosition = 0;  // 默认没有需要恢复的位置

    m_forceQuit = false;

    // 播放模式现在由一个按钮显示，默认文字为“列表循环”。
    updatePlayModeButton();

    // 播放列表允许多选
    // Ctrl 可以单独多选，Shift 可以连续多选
    ui->listMusic->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // 开启播放列表内部拖拽排序，并监听排序完成事件。
    setupPlaylistDragDrop();

    // 连接新建、重命名、删除、切换歌单以及“添加到歌单”等操作。
    setupPlaylistManager();

    // 初始化音量按钮、悬浮滑块、百分比显示和点击静音功能。
    setupVolumeControl();

    //美化歌词显示
    ui->listLyrics->setFocusPolicy(Qt::NoFocus);
    ui->listLyrics->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listLyrics->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);  //自动滚动时更加平滑
    ui->listLyrics->setWordWrap(true);  //长歌词自动换行
    ui->listLyrics->setSpacing(8);  //每句歌词之间保留间距

    // 给主题选择框添加两个选项
    ui->comboTheme->addItem("明亮主题");
    ui->comboTheme->addItem("暗黑主题");

    // 读取并应用用户上次选择的主题
    // 这一句放在 connect 前面，可以避免读取主题时触发保存操作
    loadTheme();

    // 用户切换主题时立即更新界面
    connect(ui->comboTheme,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [=](int index)
    {
        // 应用用户选择的主题
        applyTheme(index);

        // 保存主题编号，下次启动程序时自动恢复
        QSettings settings("Tom", "QtMusicPlayer");
        settings.setValue("theme", index);
    });


    // 程序刚启动、还没有当前歌曲时，先禁用收藏按钮
    ui->btnFavorite->setEnabled(false);

    // 点击收藏按钮
    connect(ui->btnFavorite,
            &QPushButton::clicked,
            this,
            [=]()
    {
        // 收藏或者取消收藏当前歌曲
        toggleCurrentFavorite();
    });

    //打开文件
    connect(ui->btnOpen,&QPushButton::clicked,this,[=](){
        QStringList filePaths = QFileDialog::getOpenFileNames(
                    this,
                    "选择音乐文件",
                    "",
                    "Audio Files (*.mp3 *.wav *.flac *.m4a);;All Files (*.*)"
                    );
        if(filePaths.isEmpty()){
            return;
        }

        loadMusicList(filePaths);
    });

    //打开文件夹
    connect(ui->btnOpenFolder,&QPushButton::clicked,this,[=](){
        QString folderpath = QFileDialog::getExistingDirectory(
                    this,
                    "选择音乐文件夹",
                    ""
                    );
         if(folderpath.isEmpty())
         {
             return ;
         }

         QDir dir(folderpath);

         QStringList filters;

         filters<< "*.mp3" << "*.wav" << "*.flac" << "*.m4a";

         QStringList fileNames = dir.entryList(filters,QDir::Files);

         if(fileNames.isEmpty())
         {
             QMessageBox::warning(this,"提示","这个文件夹里没有找到音乐文件");

             return ;
         }

         QStringList filePaths;

         for(int i = 0;i<fileNames.size();i++){
             filePaths.append(dir.absoluteFilePath(fileNames[i]));
         }

         loadMusicList(filePaths);

    });


    // 播放 / 暂停 合并按钮
    connect(ui->btnPlay, &QPushButton::clicked, this, [=]() {

        if (m_currentMusicPath.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择音乐文件");
            return;
        }

        if (m_player->state() == QMediaPlayer::PlayingState) {
            m_player->pause();
        } else {
            m_player->play();
        }
    });

    // 播放状态发生变化时，同时更新界面按钮和托盘菜单
    connect(m_player, &QMediaPlayer::stateChanged, this,
            [=](QMediaPlayer::State state) {

        if (state == QMediaPlayer::PlayingState) {
            ui->btnPlay->setText("暂停");

            // 托盘创建成功后，更新托盘菜单文字
            if (m_trayPlayAction) {
                m_trayPlayAction->setText("暂停");
            }
        } else {
            ui->btnPlay->setText("播放");

            if (m_trayPlayAction) {
                m_trayPlayAction->setText("播放");
            }
        }
    });


    //停止音乐
    connect(ui->btnStop,&QPushButton::clicked,this,[=](){
        m_player->stop();
    });

    // 时间格式转换：毫秒 -> 00:00
    auto formatTime = [](qint64 ms) {
        int totalSeconds = ms / 1000;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;

        return QString("%1:%2")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    };

    // 获取音乐总时长
    connect(m_player, &QMediaPlayer::durationChanged, this, [=](qint64 duration) {
        ui->sliderProgress->setRange(0, duration);

        qint64 currentPos = 0; // 默认当前位置为 0

        // 如果有等待恢复的播放位置，并且音乐时长已经加载出来
        if (m_pendingPosition > 0 && duration > 0)
        {
            // 防止保存的位置超过歌曲总时长
            if (m_pendingPosition > duration)
            {
                m_pendingPosition = duration;
            }

            currentPos = m_pendingPosition; // 将当前时间设为恢复的时间
            m_player->setPosition(m_pendingPosition);
            ui->sliderProgress->setValue(m_pendingPosition);

            // 恢复完以后清空，防止重复跳转
            m_pendingPosition = 0;
        }
        else
        {
            ui->sliderProgress->setValue(0);
        }

        //  使文字标签正确显示为 [恢复的进度 / 总时长]
        ui->labelTime->setText(
                    formatTime(currentPos) + " / " + formatTime(duration)
                    );
    });

    // 播放位置变化时，更新进度条
    connect(m_player, &QMediaPlayer::positionChanged, this, [=](qint64 position) {
        if (!ui->sliderProgress->isSliderDown()) {
            ui->sliderProgress->setValue(position);
        }

        qint64 duration = m_player->duration();

        ui->labelTime->setText(   //把当前播放时间和总时长显示出来
                    formatTime(position) + " / " + formatTime(duration)
                    );

        // 根据当前播放位置更新歌词
        updateLyric(position);
    });


    // 拖动进度条，改变播放位置
    connect(ui->sliderProgress, &QSlider::sliderReleased, this, [=]() {
        int position = ui->sliderProgress->value();
        m_player->setPosition(position);
    });

    //双击列表播放
    connect(ui->listMusic, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem *item) {
        if(item == nullptr)
        {
            return;
        }

        // 列表项目的 UserRole 保存完整路径。
        // 通过路径寻找下标可以确保拖拽排序后仍然播放正确歌曲。
        QString musicPath = item->data(Qt::UserRole).toString();
        int musicIndex = m_musicList.indexOf(musicPath);

        if(musicIndex >= 0)
        {
            playMusicByIndex(musicIndex);
        }
    });

    //上一首
    connect(ui->btnPrev,&QPushButton::clicked,this,[=](){
        if(m_musicList.isEmpty()){
             QMessageBox::warning(this, "提示", "请先选择音乐文件");
             return ;
        }

        playPrevByMode();

     });


    //下一首
    connect(ui->btnNext,&QPushButton::clicked,this,[=](){
        if (m_musicList.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择音乐文件");
            return;
        }

        playNextByMode();
     });

    // 当前歌曲播放结束后，自动切换到下一首
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status != QMediaPlayer::EndOfMedia || m_musicList.isEmpty()) {
            return;
        }

        playNextByMode();

        });


    // 播放错误处理
    connect(m_player,QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),this,[=](QMediaPlayer::Error error) {

        if (error == QMediaPlayer::NoError) {
            return;
        }

        // 判断当前音乐文件是否已经不存在

        QFileInfo fileInfo(m_currentMusicPath);

        if(!m_currentMusicPath.isEmpty() && !fileInfo.exists())
        {
            int ret = QMessageBox::question(
                        this,
                        "文件不存在",
                        "当前音乐文件已经不存在：\n"
                        + fileInfo.fileName()
                        + "\n\n是否从播放列表中移除？"
                        );
            if (ret == QMessageBox::Yes)
            {
                removeMusicAtIndex(m_currentIndex);
            }
            return ;
        }

        QString errorMessage;

        switch (error) {
        case QMediaPlayer::ResourceError:
            errorMessage = "无法读取音乐文件，文件可能已被删除或路径无效";
            break;

        case QMediaPlayer::FormatError:
            errorMessage = "音乐格式不受支持或文件可能已经损坏";
            break;

        case QMediaPlayer::NetworkError:
            errorMessage = "网络错误";
            break;

        case QMediaPlayer::AccessDeniedError:
            errorMessage = "没有权限访问这个音乐文件";
            break;

        default:
            errorMessage = "未知播放错误";
            break;
        }

        QMessageBox::warning(
                    this,
                    "播放错误",
                    errorMessage + "\n\n详细信息：" + m_player->errorString()
                    );
    });


    // 点击按钮时按照：列表循环 -> 单曲循环 -> 顺序播放 -> 随机播放 的顺序切换。
    connect(ui->btnPlayMode, &QPushButton::clicked, this, [=]() {
        m_playMode = (m_playMode + 1) % 4;

        // 进入随机播放时，立即为当前歌单生成一轮新的随机顺序。
        if(m_playMode == 3 && !m_musicList.isEmpty())
        {
            generateShuffleOrder();
        }

        // 让按钮立即显示切换后的模式文字。
        updatePlayModeButton();

        // 保存播放模式，下次启动播放器时继续使用。
        savePlaylist();
    });

    //删除列表里的歌曲
    connect(ui->btnRemoveMusic,&QPushButton::clicked,this,[=](){
        if (m_musicList.isEmpty()) {
            QMessageBox::warning(this, "提示", "播放列表为空");
            return;
        }

        // 获取当前选中的所有歌曲项
        QList<QListWidgetItem*> selectedItems = ui->listMusic->selectedItems();

        if(selectedItems.isEmpty())
        {
            QMessageBox::warning(this, "提示", "请先选中要移除的歌曲");
            return ;
        }

        //保存所有要删除的行号
        QVector<int> rows;

        for(QListWidgetItem *item : selectedItems)  //把选中的歌曲转换成对应的行号。
        {
            int row = ui->listMusic->row(item);
            rows.append(row);
        }

        // 必须从大到小删除，防止删除前面的项后，后面的下标变化
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        //逐个删除
        for(int row : rows)
        {
            removeMusicAtIndex(row);
        }
    });

    // 必须先读取收藏列表
    loadFavorites();

    // 再恢复播放列表和当前歌曲
    loadPlaylist();

    // 根据恢复出来的当前歌曲设置收藏按钮
    updateFavoriteButton();

    // 根据搜索框和收藏选项刷新显示
    refreshMusicFilter();

    //清空列表
    connect(ui->btnClearMusic,&QPushButton::clicked,this,[=](){

        if(m_musicList.isEmpty())
        {
            QMessageBox::warning(this, "提示", "播放列表已经为空");
            return ;
        }

        //弹出确认窗口
        int ret = QMessageBox::question(
                    this,
                    "确认清空",
                    "确定清空整个播放列表吗?"
                    );

        if(ret != QMessageBox::Yes)
        {
            return ;
        }

        m_player->stop();  //停止播放器并清掉当前媒体
        m_player->setMedia(QMediaContent());

        m_musicList.clear();  //清空保存歌曲路径的列表
        m_currentIndex = -1;  //表示当前没有选中任何歌曲
        m_currentMusicPath.clear();  //清空当前歌曲路径

        //清空随机播放数据
        m_shufflePos = 0;
        m_shuffleOrder.clear();

        // 清空播放列表界面
        ui->listMusic->clear();

        //清空歌曲信息
        clearMusicInfo();

        //清空歌词
        clearLyrics();

        // 当前歌曲已经被清空，收藏按钮应当禁用
        updateFavoriteButton();

        // 清空界面后同步刷新筛选状态
        refreshMusicFilter();


        savePlaylist();  //保存清空后的状态
    });

    // 搜索文字发生变化时，重新筛选歌曲
    connect(ui->lineSearchMusic,
            &QLineEdit::textChanged,
            this,
            [=](const QString &)
    {
        refreshMusicFilter();
    });

    // 勾选或取消“只看收藏”时，重新筛选歌曲
    connect(ui->checkFavoriteOnly,
            &QCheckBox::toggled,
            this,
            [=](bool)
    {
        refreshMusicFilter();
    });

}

// 创建系统托盘图标及右键菜单
void MainWindow::setupSystemTray()
{
    // 某些系统可能不支持系统托盘，因此先进行判断
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    // 创建系统托盘图标，this 作为父对象，程序结束时会自动释放
    m_trayIcon = new QSystemTrayIcon(this);

    // 优先使用主窗口图标
    QIcon trayIcon = windowIcon();

    // 如果主窗口没有设置图标，就使用 Qt 自带的播放图标
    if (trayIcon.isNull()) {
        trayIcon = style()->standardIcon(QStyle::SP_MediaPlay);
    }

    m_trayIcon->setIcon(trayIcon);

    // 鼠标停留在托盘图标上时显示的文字
    m_trayIcon->setToolTip("Qt Music Player");

    // 创建托盘右键菜单
    QMenu *trayMenu = new QMenu(this);

    // 创建菜单操作
    QAction *showAction = new QAction("显示主窗口", this);
    QAction *previousAction = new QAction("上一首", this);
    m_trayPlayAction = new QAction("播放", this);
    QAction *nextAction = new QAction("下一首", this);
    QAction *stopAction = new QAction("停止", this);
    QAction *quitAction = new QAction("退出程序", this);

    // 按照需要的顺序加入右键菜单
    trayMenu->addAction(showAction);
    trayMenu->addSeparator();

    trayMenu->addAction(previousAction);
    trayMenu->addAction(m_trayPlayAction);
    trayMenu->addAction(nextAction);
    trayMenu->addAction(stopAction);

    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    // 将右键菜单设置给托盘图标
    m_trayIcon->setContextMenu(trayMenu);

    // 点击“显示主窗口”
    connect(showAction, &QAction::triggered, this, [=]() {
        show();                 // 显示窗口
        setWindowState(windowState() & ~Qt::WindowMinimized);
        raise();                // 将窗口放到前面
        activateWindow();       // 激活窗口
    });

    // 托盘的播放按钮直接触发界面上的播放按钮
    connect(m_trayPlayAction, &QAction::triggered, this, [=]() {
        ui->btnPlay->click();
    });

    // 播放上一首
    connect(previousAction, &QAction::triggered, this, [=]() {
        if (m_musicList.isEmpty()) {
            m_trayIcon->showMessage(
                        "提示",
                        "播放列表中还没有歌曲",
                        QSystemTrayIcon::Information,
                        2000);
            return;
        }

        playPrevByMode();
    });

    // 播放下一首
    connect(nextAction, &QAction::triggered, this, [=]() {
        if (m_musicList.isEmpty()) {
            m_trayIcon->showMessage(
                        "提示",
                        "播放列表中还没有歌曲",
                        QSystemTrayIcon::Information,
                        2000);
            return;
        }

        playNextByMode();
    });

    // 停止播放
    connect(stopAction, &QAction::triggered, this, [=]() {
        m_player->stop();
    });

    connect(quitAction, &QAction::triggered, this, [=]() {
        // 从托盘菜单退出时不再弹出询问窗口
        m_forceQuit = true;
        qApp->quit();
    });

    // 双击托盘图标时显示主窗口
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, [=](QSystemTrayIcon::ActivationReason reason) {

        if (reason == QSystemTrayIcon::DoubleClick) {
            show();
            setWindowState(windowState() & ~Qt::WindowMinimized);
            raise();
            activateWindow();
        }
    });

    // 最后显示托盘图标
    m_trayIcon->show();
}


// 读取用户上次选择的主题
void MainWindow::loadTheme()
{
    // 与播放列表、收藏功能使用相同的配置位置
    QSettings settings("Tom", "QtMusicPlayer");

    // 如果以前没有保存过主题，就默认使用明亮主题
    int themeIndex = settings.value("theme", 0).toInt();

    // 防止配置文件中的值不正确
    if (themeIndex < 0 || themeIndex > 1) {
        themeIndex = 0;
    }

    // 设置下拉框当前选项
    ui->comboTheme->setCurrentIndex(themeIndex);

    // 应用读取到的主题
    applyTheme(themeIndex);
}


// 根据编号设置程序主题
void MainWindow::applyTheme(int themeIndex)
{
    QString styleSheet;

    if (themeIndex == 1) {
        //暗黑主题
        styleSheet =
            // 整个程序的基础颜色和字体
            "QWidget {"
            "    background-color: #1e222a;"
            "    color: #e5e7eb;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 14px;"
            "}"

            "QMainWindow {"
            "    background-color: #1e222a;"
            "}"

            // QLabel 不单独显示背景色
            "QLabel {"
            "    background-color: transparent;"
            "    color: #e5e7eb;"
            "}"

            // 按钮正常状态
            "QPushButton {"
            "    background-color: #343b48;"
            "    color: #f3f4f6;"
            "    border: 1px solid #4b5563;"
            "    border-radius: 6px;"
            "    padding: 5px 12px;"
            "}"

            // 鼠标移动到按钮上
            "QPushButton:hover {"
            "    background-color: #3b82f6;"
            "    border-color: #60a5fa;"
            "}"

            // 按下按钮
            "QPushButton:pressed {"
            "    background-color: #2563eb;"
            "}"

            // 禁用状态，例如没有歌曲时的收藏按钮
            "QPushButton:disabled {"
            "    background-color: #292e38;"
            "    color: #6b7280;"
            "    border-color: #374151;"
            "}"

            // 输入框、下拉框和列表
            "QLineEdit, QComboBox, QListWidget {"
            "    background-color: #111827;"
            "    color: #e5e7eb;"
            "    border: 1px solid #4b5563;"
            "    border-radius: 6px;"
            "    padding: 4px;"
            "    selection-background-color: #2563eb;"
            "    selection-color: white;"
            "}"

            "QLineEdit:focus, QComboBox:focus, QListWidget:focus {"
            "    border: 1px solid #60a5fa;"
            "}"

            // 下拉选项弹出后的颜色
            "QComboBox QAbstractItemView {"
            "    background-color: #111827;"
            "    color: #e5e7eb;"
            "    selection-background-color: #2563eb;"
            "}"

            // 播放列表项目
            "QListWidget::item {"
            "    padding: 6px;"
            "}"

            "QListWidget::item:selected {"
            "    background-color: #2563eb;"
            "    color: white;"
            "}"

            // 歌词列表的普通歌词
            "QListWidget#listLyrics::item {"
            "    color: #9ca3af;"
            "    padding: 6px;"
            "}"

            // 歌词列表中正在播放的一句
            "QListWidget#listLyrics::item:selected {"
            "    color: #60a5fa;"
            "    background-color: transparent;"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "}"

            // 专辑封面区域
            "QLabel#labelCover {"
            "    background-color: #111827;"
            "    border: 1px solid #4b5563;"
            "    border-radius: 8px;"
            "}"

            // 音量悬浮面板和圆形音量按钮
            "QFrame#frameVolumePopup {"
            "    background-color: #252b35;"
            "    border: 1px solid #4b5563;"
            "    border-radius: 10px;"
            "}"

            "QFrame#frameVolumePopup QSlider {"
            "    background-color: transparent;"
            "}"

            "QPushButton#btnVolume {"
            "    padding: 0px;"
            "    border-radius: 17px;"
            "}"

            // 水平进度条
            "QSlider::groove:horizontal {"
            "    height: 6px;"
            "    background-color: #374151;"
            "    border-radius: 3px;"
            "}"

            "QSlider::sub-page:horizontal {"
            "    background-color: #3b82f6;"
            "    border-radius: 3px;"
            "}"

            "QSlider::handle:horizontal {"
            "    width: 16px;"
            "    margin: -5px 0;"
            "    background-color: #60a5fa;"
            "    border-radius: 8px;"
            "}"

            // 垂直音量条
            "QSlider::groove:vertical {"
            "    width: 6px;"
            "    background-color: #374151;"
            "    border-radius: 3px;"
            "}"

            // 垂直音量条上方表示尚未达到的音量
            "QSlider::sub-page:vertical {"
            "    background-color: #374151;"
            "    border-radius: 3px;"
            "}"

            // 垂直音量条下方表示当前已有的音量
            "QSlider::add-page:vertical {"
            "    background-color: #3b82f6;"
            "    border-radius: 3px;"
            "}"

            "QSlider::handle:vertical {"
            "    height: 16px;"
            "    margin: 0 -5px;"
            "    background-color: #60a5fa;"
            "    border-radius: 8px;"
            "}"

            // 菜单栏、工具栏和状态栏
            "QMenuBar, QToolBar, QStatusBar {"
            "    background-color: #252b35;"
            "    color: #e5e7eb;"
            "}"

            "QMenu {"
            "    background-color: #252b35;"
            "    color: #e5e7eb;"
            "    border: 1px solid #4b5563;"
            "}"

            "QMenu::item:selected {"
            "    background-color: #2563eb;"
            "}";
    }
    else {
        //明亮主题
        styleSheet =
            "QWidget {"
            "    background-color: #f5f7fb;"
            "    color: #1f2937;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 14px;"
            "}"

            "QMainWindow {"
            "    background-color: #f5f7fb;"
            "}"

            "QLabel {"
            "    background-color: transparent;"
            "    color: #1f2937;"
            "}"

            "QPushButton {"
            "    background-color: white;"
            "    color: #1f2937;"
            "    border: 1px solid #cbd5e1;"
            "    border-radius: 6px;"
            "    padding: 5px 12px;"
            "}"

            "QPushButton:hover {"
            "    background-color: #eaf2ff;"
            "    border-color: #3b82f6;"
            "    color: #2563eb;"
            "}"

            "QPushButton:pressed {"
            "    background-color: #dbeafe;"
            "}"

            "QPushButton:disabled {"
            "    background-color: #e5e7eb;"
            "    color: #9ca3af;"
            "    border-color: #d1d5db;"
            "}"

            "QLineEdit, QComboBox, QListWidget {"
            "    background-color: white;"
            "    color: #1f2937;"
            "    border: 1px solid #cbd5e1;"
            "    border-radius: 6px;"
            "    padding: 4px;"
            "    selection-background-color: #3b82f6;"
            "    selection-color: white;"
            "}"

            "QLineEdit:focus, QComboBox:focus, QListWidget:focus {"
            "    border: 1px solid #3b82f6;"
            "}"

            "QComboBox QAbstractItemView {"
            "    background-color: white;"
            "    color: #1f2937;"
            "    selection-background-color: #3b82f6;"
            "}"

            "QListWidget::item {"
            "    padding: 6px;"
            "}"

            "QListWidget::item:selected {"
            "    background-color: #dbeafe;"
            "    color: #1d4ed8;"
            "}"

            "QListWidget#listLyrics::item {"
            "    color: #6b7280;"
            "    padding: 6px;"
            "}"

            "QListWidget#listLyrics::item:selected {"
            "    color: #2563eb;"
            "    background-color: transparent;"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "}"

            "QLabel#labelCover {"
            "    background-color: white;"
            "    border: 1px solid #d1d5db;"
            "    border-radius: 8px;"
            "}"

            // 音量悬浮面板和圆形音量按钮
            "QFrame#frameVolumePopup {"
            "    background-color: white;"
            "    border: 1px solid #d1d5db;"
            "    border-radius: 10px;"
            "}"

            "QFrame#frameVolumePopup QSlider {"
            "    background-color: transparent;"
            "}"

            "QPushButton#btnVolume {"
            "    padding: 0px;"
            "    border-radius: 17px;"
            "}"

            "QSlider::groove:horizontal {"
            "    height: 6px;"
            "    background-color: #dbe2ea;"
            "    border-radius: 3px;"
            "}"

            "QSlider::sub-page:horizontal {"
            "    background-color: #3b82f6;"
            "    border-radius: 3px;"
            "}"

            "QSlider::handle:horizontal {"
            "    width: 16px;"
            "    margin: -5px 0;"
            "    background-color: #2563eb;"
            "    border-radius: 8px;"
            "}"

            "QSlider::groove:vertical {"
            "    width: 6px;"
            "    background-color: #dbe2ea;"
            "    border-radius: 3px;"
            "}"

            // 垂直音量条上方表示尚未达到的音量
            "QSlider::sub-page:vertical {"
            "    background-color: #dbe2ea;"
            "    border-radius: 3px;"
            "}"

            // 垂直音量条下方表示当前已有的音量
            "QSlider::add-page:vertical {"
            "    background-color: #3b82f6;"
            "    border-radius: 3px;"
            "}"

            "QSlider::handle:vertical {"
            "    height: 16px;"
            "    margin: 0 -5px;"
            "    background-color: #2563eb;"
            "    border-radius: 8px;"
            "}"

            "QMenuBar, QToolBar, QStatusBar {"
            "    background-color: #eef2f7;"
            "    color: #1f2937;"
            "}"

            "QMenu {"
            "    background-color: white;"
            "    color: #1f2937;"
            "    border: 1px solid #cbd5e1;"
            "}"

            "QMenu::item:selected {"
            "    background-color: #dbeafe;"
            "}";
    }

    // 为新的分区面板补充卡片样式，并突出主播放按钮。
    if(themeIndex == 1)
    {
        styleSheet +=
            "QFrame#leftPanel, QFrame#nowPlayingPanel, QFrame#lyricsPanel, QFrame#playerPanel {"
            "    background-color: #252b35;"
            "    border: 1px solid #343b48;"
            "    border-radius: 14px;"
            "}"
            "QLabel#appTitle {"
            "    color: #f8fafc;"
            "    font-size: 24px;"
            "    font-weight: bold;"
            "}"
            "QLabel#sectionTitle {"
            "    color: #f3f4f6;"
            "    font-size: 17px;"
            "    font-weight: bold;"
            "}"
            "QPushButton#btnPlay {"
            "    background-color: #3b82f6;"
            "    color: white;"
            "    border-color: #3b82f6;"
            "    font-weight: bold;"
            "}"
            "QPushButton#btnPlay:hover {"
            "    background-color: #60a5fa;"
            "}"
            "QListWidget#listLyrics {"
            "    background-color: transparent;"
            "    border: none;"
            "}";
    }
    else
    {
        styleSheet +=
            "QFrame#leftPanel, QFrame#nowPlayingPanel, QFrame#lyricsPanel, QFrame#playerPanel {"
            "    background-color: white;"
            "    border: 1px solid #e2e8f0;"
            "    border-radius: 14px;"
            "}"
            "QLabel#appTitle {"
            "    color: #0f172a;"
            "    font-size: 24px;"
            "    font-weight: bold;"
            "}"
            "QLabel#sectionTitle {"
            "    color: #1e293b;"
            "    font-size: 17px;"
            "    font-weight: bold;"
            "}"
            "QPushButton#btnPlay {"
            "    background-color: #2563eb;"
            "    color: white;"
            "    border-color: #2563eb;"
            "    font-weight: bold;"
            "}"
            "QPushButton#btnPlay:hover {"
            "    background-color: #3b82f6;"
            "    color: white;"
            "}"
            "QListWidget#listLyrics {"
            "    background-color: transparent;"
            "    border: none;"
            "}";

    }

    // qApp 表示整个应用程序。因此样式不仅会应用到主窗口，也会应用到提示框和菜单。
    qApp->setStyleSheet(styleSheet);
}
