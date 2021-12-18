#include "qffmpeg.h"
#include <QDateTime>
#include <qdebug.h>

QFFmpeg::QFFmpeg(QObject *parent) :
    QObject(parent)
{
    videoStreamIndex=-1;
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
    AVDictionary* options = NULL;
    av_dict_set(&options, "buffer_size", "102400", 0); //缓存大小
    //av_dict_set(&options, "rtsp_transport", "tcp", 0); //tcp打开
    av_dict_set(&options, "stimeout", "2000000", 0); //超时断开连接时间(us)
    av_dict_set(&options, "max_delay", "500000", 0); //最大时延

    //打开视频流
    int result=avformat_open_input(&pAVFormatContext, url.toStdString().c_str(),NULL,&options);
    if (result<0){
        qDebug()<<url<<endl;
        qDebug()<<"打开视频流失败";
        return false;
    }

    //获取视频流信息
    result=avformat_find_stream_info(pAVFormatContext,NULL);
    if (result<0){
        qDebug()<<"获取视频流信息失败";
        return false;
    }

    //获取视频流索引
    videoStreamIndex = -1;
    for (uint i = 0; i < pAVFormatContext->nb_streams; i++) {
        if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {//codecpar
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex==-1){
        qDebug()<<"获取视频流索引失败";
        return false;
    }

    //获取视频流的分辨率大小
    AVCodec *codec = avcodec_find_decoder(pAVFormatContext->streams[videoStreamIndex]->codecpar->codec_id);//AV_CODEC_ID_H264);//
    pAVCodecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(pAVCodecContext, pAVFormatContext->streams[videoStreamIndex]->codecpar);
    videoWidth = pAVCodecContext->width;
    videoHeight = pAVCodecContext->height;

    //av_image_fill_arrays(pAVPicture->data, pAVPicture->linesize, NULL, AV_PIX_FMT_BGR24, videoWidth, videoHeight, 0);
    //av_image_fill_arrays(pAVFrame->data, pAVFrame->linesize, NULL, AV_PIX_FMT_BGR24, videoWidth, videoHeight, 0);
    av_image_alloc(pAVPicture->data,
        pAVPicture->linesize,
        videoWidth,
        videoHeight,
       AV_PIX_FMT_RGBA,
        1);
    av_image_alloc(pAVFrame->data,
        pAVFrame->linesize,
        videoWidth,
        videoHeight,
        AV_PIX_FMT_YUVJ420P,
        1);

    AVCodec *pAVCodec;

    //获取视频流解码器
    pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
    pSwsContext = sws_getContext(videoWidth,videoHeight,AV_PIX_FMT_YUVJ420P,videoWidth,videoHeight,AV_PIX_FMT_RGBA,SWS_BICUBIC,0,0,0);

    //打开对应解码器
    result = avcodec_open2(pAVCodecContext,pAVCodec,NULL);
    if (result<0){
        qDebug()<<"打开解码器失败";
        return false;
    }

    qDebug()<<"初始化视频流成功";
    return true;
}

void QFFmpeg::Play()
{
    //一帧一帧读取
    while (true){
        if (av_read_frame(pAVFormatContext, &pAVPacket) >= 0){
            if(pAVPacket.stream_index == videoStreamIndex){
                qDebug()<<"开始解码"<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
                int ret = avcodec_send_packet(pAVCodecContext, &pAVPacket);
                if (ret != 0) {
                    av_packet_unref(&pAVPacket);//释放资源
                    qDebug()<<"数据包发送失败";
                }
                while(avcodec_receive_frame(pAVCodecContext, pAVFrame) == 0) {
                    sws_scale(pSwsContext,(const uint8_t* const *)pAVFrame->data, pAVFrame->linesize,0,videoHeight, pAVPicture->data, pAVPicture->linesize);
                    //发送获取一帧图像信号
                    mutex.lock();
                    QImage image(pAVPicture->data[0],videoWidth,videoHeight, QImage::Format_RGBA8888);
                    emit GetImage(image);
                    mutex.unlock();
                }
            }
        }
        av_packet_unref(&pAVPacket);//释放资源
    }
}


