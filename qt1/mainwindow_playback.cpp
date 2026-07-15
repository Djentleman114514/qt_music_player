#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QUrl>

#include <algorithm>
#include <ctime>
#include <random>


// 根据当前模式更新按钮文字。
// 模式编号保持原来的定义：0列表循环，1单曲循环，2顺序播放，3随机播放。
void MainWindow::updatePlayModeButton()
{
    QString modeText;

    switch(m_playMode)
    {
    case 0:
        modeText = "列表循环";
        break;
    case 1:
        modeText = "单曲循环";
        break;
    case 2:
        modeText = "顺序播放";
        break;
    case 3:
        modeText = "随机播放";
        break;
    default:
        // 设置文件中的值异常时退回列表循环。
        m_playMode = 0;
        modeText = "列表循环";
        break;
    }

    ui->btnPlayMode->setText(modeText);
    ui->btnPlayMode->setToolTip(
                "当前模式：" + modeText + "\n点击切换下一种播放模式"
                );
}

//根据歌曲下标播放对应歌曲
void MainWindow::playMusicByIndex(int index)
{
    if (index < 0 || index >= m_musicList.size()) {
        return;
    }

    m_currentIndex = index;
    m_currentMusicPath = m_musicList[index];

    ui->listMusic->setCurrentRow(index);

    m_player->setMedia(QUrl::fromLocalFile(m_currentMusicPath));

    // 加载当前歌曲歌词
    loadLyrics();

    // 切换歌曲后，更新收藏按钮状态
    updateFavoriteButton();

    // 开始播放新歌曲
    m_player->play();

    // 保存当前播放列表和歌曲位置
    savePlaylist();
}


//随机播放,使用洗牌算法
void  MainWindow::generateShuffleOrder()
{
    m_shuffleOrder.clear();

    for(int i = 0;i<m_musicList.size();i++){
        m_shuffleOrder.append(i);
    }

    static std::mt19937 generator(static_cast<unsigned int>(time(nullptr)));  //创建随机数生成器

    std::shuffle(m_shuffleOrder.begin(), m_shuffleOrder.end(), generator);  //洗牌

    m_shufflePos = 0;
}


//根据当前播放模式，决定下一首应该怎么播放。
void MainWindow::playNextByMode()
{
    if (m_musicList.isEmpty()) {
        return;
    }
    // 0.列表循环
    if (m_playMode == 0) {
        int nextIndex = m_currentIndex + 1;

        if (nextIndex >= m_musicList.size()) {
            nextIndex = 0;
        }

        playMusicByIndex(nextIndex);
    }

    // 1.单曲循环
    else if (m_playMode == 1) {
        playMusicByIndex(m_currentIndex);
    }

    // 2.顺序播放
    else if (m_playMode == 2) {
        int nextIndex = m_currentIndex + 1;

        if (nextIndex >= m_musicList.size()) {
            m_player->stop();
            return;
        }

        playMusicByIndex(nextIndex);
    }

    // 3.随机播放
    else if (m_playMode == 3) {

        // 随机顺序为空或者已经播放完，重新洗牌
        if(m_shuffleOrder.isEmpty() || m_shufflePos >= m_shuffleOrder.size())
        {
            generateShuffleOrder();
        }

        int randomIndex = m_shuffleOrder[m_shufflePos];

        m_shufflePos++;

        // 防止连续播放当前歌曲

        while(randomIndex == m_currentIndex && m_musicList.size() > 1)
        {
            // 当前随机顺序已经读取完，重新洗牌
            if(m_shufflePos >= m_shuffleOrder.size())
            {
                generateShuffleOrder();
            }

            randomIndex = m_shuffleOrder[m_shufflePos];

            m_shufflePos++;

        }

        playMusicByIndex(randomIndex);
    }
}

// 根据当前播放模式，决定上一首应该怎么播放
void MainWindow::playPrevByMode()
{
    if (m_musicList.isEmpty()) {
        return;
    }

    // 0. 列表循环
    if (m_playMode == 0) {
        int prevIndex = m_currentIndex - 1;

        if (prevIndex < 0) {
            prevIndex = m_musicList.size() - 1;
        }

        playMusicByIndex(prevIndex);
    }

    // 1. 单曲循环
    else if (m_playMode == 1) {
        playMusicByIndex(m_currentIndex);
    }

    // 2. 顺序播放
    else if (m_playMode == 2) {
        int prevIndex = m_currentIndex - 1;

        if (prevIndex < 0) {
            return;
        }

        playMusicByIndex(prevIndex);
    }

    // 3. 随机播放
    else if (m_playMode == 3) {
        playNextByMode();
    }

}

