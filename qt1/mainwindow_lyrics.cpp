#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractItemView>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QListWidgetItem>

#include <algorithm>

//清除旧歌词
void MainWindow::clearLyrics()
{
    m_lyrics.clear();

    m_currentLyricIndex = -1;

    ui->listLyrics->clear();

    ui->listLyrics->addItem("暂无歌词");
}





// 加载当前歌曲的歌词
void MainWindow::loadLyrics()
{
    m_lyrics.clear();

    ui->listLyrics->clear();

    m_currentLyricIndex = -1;   //清空旧数据

    if(m_currentMusicPath.isEmpty())
    {
        return;
    }

    QFileInfo musicFileInfo(m_currentMusicPath);

    QDir musicDir = musicFileInfo.dir();

    QString lyricFileName =
            musicFileInfo.completeBaseName() + ".lrc";  //获取不带扩展名的歌曲名

    QString lyricPath =
            musicDir.absoluteFilePath(lyricFileName);

    QFile lyricFile(lyricPath);  //创建歌词文件对象

    if(!lyricFile.exists())
    {
        ui->listLyrics->addItem("暂无歌词");

        return;
    }

    if(!lyricFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        ui->listLyrics->addItem("歌词文件读取失败");

        return;
    }

    QTextStream stream(&lyricFile);

    stream.setCodec("UTF-8");  //设置 UTF-8

    while(!stream.atEnd())  //一行一行读取
    {
        QString line = stream.readLine();

        QRegularExpression regex(
                    "^\\[(\\d+):(\\d+)(?:\\.(\\d+))?\\](.*)$"
                    );
        QRegularExpressionMatch match = regex.match(line);

        //如果这一行不符合歌词格式，直接跳过
        if(!match.hasMatch())
        {
            continue;
        }

        //提取并计算绝对毫秒时间
        int minutes = match.captured(1).toInt();

        int seconds = match.captured(2).toInt();

        QString fractionText = match.captured(3);

        int milliseconds = 0;

        if(fractionText.length() == 1)
            {
                milliseconds = fractionText.toInt() * 100;  //例如 [01:23.4]，.4 代表 400 毫秒，所以转成整数后乘以100
            }
            else if(fractionText.length() == 2)
            {
                milliseconds = fractionText.toInt() * 10;  //例如 [01:23.45]，.45 代表 450 毫秒，所以乘以10
            }
            else if(fractionText.length() >= 3)  //例如 [01:23.456]，直接通过 .left(3) 截取前3位，转为整数作为毫秒
            {
                milliseconds = fractionText.left(3).toInt();
            }

            qint64 time =
                    (minutes * 60 + seconds) * 1000
                    + milliseconds;

            QString text = match.captured(4).trimmed();

            LyricLine lyricLine;

            lyricLine.time = time;

            lyricLine.text = text;

            m_lyrics.append(lyricLine);
    }

    std::sort(
                m_lyrics.begin(),
                m_lyrics.end(),
                [](const LyricLine &a, const LyricLine &b)  //确保整首歌的歌词是按照从前到后的时间顺序进行排列
    {
        return a.time < b.time;
    });

    for(const LyricLine &lyricLine : m_lyrics)
    {
        QListWidgetItem *item =
                new QListWidgetItem(lyricLine.text, ui->listLyrics);

        item->setTextAlignment(Qt::AlignCenter);
    }

    lyricFile.close();
}

// 根据当前播放时间更新歌词
void MainWindow::updateLyric(qint64 position)
{
    if(m_lyrics.isEmpty())
    {
        return;
    }

    int lyricIndex = -1;

    // 寻找当前时间对应的歌词
    for(int i = 0; i < m_lyrics.size(); i++)
    {
        if(position >= m_lyrics[i].time)
        {
            lyricIndex = i;
        }
        else
        {
            break;
        }
    }

    // 还没有播放到第一句歌词
    if(lyricIndex == -1)
    {
        m_currentLyricIndex = -1;
        ui->listLyrics->setCurrentRow(-1);
        ui->listLyrics->scrollToTop();
        return;
    }

    // 当前歌词没有变化，不重复刷新
    if(lyricIndex == m_currentLyricIndex)
    {
        return;
    }

    m_currentLyricIndex = lyricIndex;

    // 高亮当前歌词
    ui->listLyrics->setCurrentRow(lyricIndex);

    // 把当前歌词滚动到列表中间
    ui->listLyrics->scrollToItem(
                ui->listLyrics->item(lyricIndex),
                QAbstractItemView::PositionAtCenter
                );
}
