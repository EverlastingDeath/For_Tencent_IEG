#include "packet.h"
#include <QDateTime>
#include <qdebug.h>

QFFmpeg::QFFmpeg(QObject *parent) :
    QObject(parent)
{
    avformat_network_init();//初始化网络流格式
    pAVFormatContext = avformat_alloc_context();//AVFormatContext初始化
    pAVFrame=av_frame_alloc();
    pAVPicture=av_frame_alloc();


}

QFFmpeg::~QFFmpeg()
{
    avformat_free_context(pAVFormatContext);
    av_frame_free(&pAVFrame);
    av_frame_free(&pAVPicture);
    sws_freeContext(pSwsContext);
}

bool QFFmpeg::Init()
{
    //获取视频流的分辨率大小
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    pAVCodecContext = avcodec_alloc_context3(codec);
    pAVCodecContext->width = 1280;
    pAVCodecContext->height = 720;
    videoWidth = pAVCodecContext->width;
    videoHeight = pAVCodecContext->height;
    pAVCodecParserContext = av_parser_init(AV_CODEC_ID_H264);

    av_image_alloc(pAVPicture->data,
        pAVPicture->linesize,
        videoWidth,
        videoHeight,
        AV_PIX_FMT_BGR24,
        1);
    av_image_alloc(pAVFrame->data,
        pAVFrame->linesize,
        videoWidth,
        videoHeight,
        AV_PIX_FMT_YUV420P,
        1);

    //获取视频流解码器
    pSwsContext = sws_getContext(videoWidth,videoHeight,AV_PIX_FMT_YUV420P,videoWidth,videoHeight,AV_PIX_FMT_BGR24,SWS_BICUBIC,0,0,0);

    //打开对应解码器
    int result = avcodec_open2(pAVCodecContext,codec,NULL);
    if (result<0){
        qDebug()<<"打开解码器失败";
        return false;
    }

    qDebug()<<"初始化视频流成功";
    return true;
}

void QFFmpeg::Play()
{
    qDebug()<<"Begin to play"<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    while (1){
        picture_ok.acquire();
        if(result == 0){
            result = 1;
            //av_init_packet(&pAVPacket);
            pAVPacket.pts                  = AV_NOPTS_VALUE;
            pAVPacket.dts                  = AV_NOPTS_VALUE;
            pAVPacket.pos                  = -1;
            pAVPacket.duration             = 0;
            pAVPacket.flags                = 0;
            pAVPacket.stream_index         = 0;
            pAVPacket.buf                  = NULL;
            pAVPacket.side_data            = NULL;
            pAVPacket.side_data_elems      = 0;
            pAVPacket.stream_index = 0;
        }

        mutex_of_picture.lock();
        Data NALU = h264_picture.front();
        h264_picture.pop();
        mutex_of_picture.unlock();

        result = av_parser_parse2(pAVCodecParserContext, pAVCodecContext,
                                  &pAVPacket.data,& pAVPacket.size,(uint8_t *)NALU.data,NALU.size,
                                  AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

        avcodec_send_packet(pAVCodecContext, &pAVPacket);
        avcodec_receive_frame(pAVCodecContext, pAVFrame);

        //qDebug()<<"开始解码"<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        int ret = avcodec_send_packet(pAVCodecContext, &pAVPacket);
        if (ret != 0) {
            av_packet_unref(&pAVPacket);//释放资源
            qDebug()<<"数据包发送失败";
        }

        while(avcodec_receive_frame(pAVCodecContext, pAVFrame) == 0) {
            sws_scale(pSwsContext,(const uint8_t* const *)pAVFrame->data, pAVFrame->linesize,0,videoHeight, pAVPicture->data, pAVPicture->linesize);
            //发送获取一帧图像信号
            mutex.lock();
            QImage image(pAVPicture->data[0],videoWidth,videoHeight, QImage::Format_RGB888);
            emit GetImage(image);
            mutex.unlock();
        }
        av_packet_unref(&pAVPacket);//释放资源
    }

}
