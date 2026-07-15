#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMediaMetaData>
#include <QPixmap>
#include <QStringList>
// 更新当前歌曲的元数据信息
void MainWindow::updateMusicMetaData()
{

    QString title = m_player->metaData(QMediaMetaData::Title).toString();

    QString artist = m_player->metaData(QMediaMetaData::AlbumArtist).toString();

    // 如果专辑艺术家为空，尝试读取歌曲艺术家
    if(artist.isEmpty())
    {
        QStringList artists = m_player->metaData(QMediaMetaData::ContributingArtist).toStringList();  //参与当前媒体的艺术家列表

        if(!artists.isEmpty())
        {
            artist = artists.join(" / ");
        }
    }

    QString album = m_player->metaData(QMediaMetaData::AlbumTitle).toString();

    if(title.isEmpty())
    {
        QFileInfo fileInfo(m_currentMusicPath);

        title = fileInfo.completeBaseName();
    }
    if(artist.isEmpty())
    {
        artist = "未知";
    }
    if(album.isEmpty())
    {
        album = "未知";
    }

    ui->labelTitle->setText("标题：" + title);
    ui->labelArtist->setText("歌手：" + artist);
    ui->labelAlbum->setText("专辑：" + album);

    updateAlbumCover();

}

// 更新当前歌曲的专辑封面
void MainWindow::updateAlbumCover()
{
    QImage coverImage =m_player->metaData(QMediaMetaData::CoverArtImage).value<QImage>();  //读取内嵌封面

    // CoverArtImage 读取失败时，尝试读取缩略图
    if(coverImage.isNull())
    {
        coverImage =
               m_player->metaData(QMediaMetaData::ThumbnailImage).value<QImage>();
    }

    //如果没有封面，尝试在歌曲所在目录里面找
    if(coverImage.isNull())
    {
        QFileInfo musicFileInfo(m_currentMusicPath);  //获取当前歌曲的信息

        QDir musicDir = musicFileInfo.dir();  //获取歌曲所在目录

        QStringList coverNames;  //保存常见封面文件名

        coverNames << "folder.jpg"
                   << "cover.jpg"
                   << "front.jpg"
                   << "album.jpg"
                   << "folder.png"
                   << "cover.png"
                   << "front.png"
                   << "album.png";

        for(const QString &coverName:coverNames)
        {
            QString coverPath = musicDir.absoluteFilePath(coverName);  //拼出完整路径

            if(QFileInfo::exists(coverPath))
            {
                if(coverImage.load(coverPath))
                {
                    break;
                }
            }
        }
    }

    //如果还是没找到，显示默认封面
    if(coverImage.isNull())
    {
        QPixmap defaultCover(":/new/prefix1/default_cover.png");

        defaultCover = defaultCover.scaled(
                    ui->labelCover->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation

                    );
        ui->labelCover->clear();

        ui->labelCover->setPixmap(defaultCover);

        return ;

    }

    QPixmap coverPixmap = QPixmap::fromImage(coverImage);   //QImage更偏向图片数据、像素处理
                                                            //QPixmap更适合界面图片显示
    coverPixmap = coverPixmap.scaled(
                ui->labelCover->size(),    //获取封面 QLabel 当前大小
                Qt::KeepAspectRatio,       //保持图片原来的宽高比例
                Qt::SmoothTransformation   //缩放图片时使用较平滑的图像转换
                );

    ui->labelCover->setPixmap(coverPixmap);  //显示封面
}


// 清空当前歌曲信息
void MainWindow::clearMusicInfo()
{

    ui->labelTitle->setText("标题：无");

    ui->labelArtist->setText("歌手：无");

    ui->labelAlbum->setText("专辑：无");

    ui->sliderProgress->setValue(0);

    ui->labelTime->setText("00:00 / 00:00");

    // 显示默认封面
    QPixmap defaultCover(":/image/default_cover.png");

    defaultCover = defaultCover.scaled(
                ui->labelCover->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                );

    ui->labelCover->clear();

    ui->labelCover->setPixmap(defaultCover);
}
