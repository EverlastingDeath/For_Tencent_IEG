#ifndef PACKET_H
#define PACKET_H
#include <set>
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include "global.h"

using namespace std;

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

//引入ffmpeg头文件
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/adts_parser.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}


struct Server_des{
    char *IP;
    quint16 Port;
};

////std::queue<Data> q;
class NALU_Packet : public QObject
{
    Q_OBJECT
public:
    explicit NALU_Packet(QObject *parent = 0);
    ~NALU_Packet();

    Server_des *server;

    bool Init();
    void run();
    void InputPacket(Data packet);

private:
    Data FU_buffer;
    std::set<int> seq;
    QTcpSocket *client;

signals:
public slots:
};

class QFFmpeg : public QObject
{
    Q_OBJECT
public:
    explicit QFFmpeg(QObject *parent = 0);
    ~QFFmpeg();

    bool Init();
    void Play();

    int VideoWidth()const{return videoWidth;}
    int VideoHeight()const{return videoHeight;}

private:
    Data FU_buffer;
    AVFrame *pAVPicture;
    AVFormatContext *pAVFormatContext;
    AVCodecContext *pAVCodecContext;
    AVFrame *pAVFrame;
    SwsContext * pSwsContext;
    AVPacket pAVPacket;
    AVCodecParserContext* pAVCodecParserContext;
    QMutex mutex;

    int videoWidth;
    int videoHeight;
    int videoStreamIndex;
    int result = 0;

signals:
    void GetImage(const QImage &image);

public slots:

};

#endif // PACKET_H
