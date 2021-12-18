/*
** Copyright (C) 2018 Wang Yaofu
** All rights reserved.
**
**Author:Wang Yaofu voipman@qq.com
**Description: The source file of Main process.
*/

#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "stream-push/StreamPush.h"
#include "common/singleton.h"
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{

#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL2/SDL.h>
#include <libavutil/imgutils.h>
}

using namespace cv;

const static int kFrameWidth = 1918;
const static int kFrameHeight = 888;
const static int kFrameChannel = 3;
const static int kFrameFPS = 25;

#define STREAM_PUSH_INS Singleton<StreamPush>::Instance()
//*****************inint*************
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVDictionary* options = NULL;
	AVInputFormat *ifmt;
	AVFrame *m_pYUVFrame;       
	AVPacket *packet;
	 AVFrame *ptmpFrame; // 将截屏图像解码出来的临时帧

//************************************
Mat avframe2cvmat(AVFrame *avframe ,int w,int h) {
 
	if (w <= 0) w = avframe->width;
	if (h <= 0) h = avframe->height;
	struct SwsContext *sws_ctx = NULL;
	sws_ctx = sws_getContext(avframe->width, avframe->height, (enum AVPixelFormat)avframe->format,
		w, h, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
 
	cv::Mat mat;
	mat.create(cv::Size(w, h), CV_8UC3);
	AVFrame *bgr24frame = av_frame_alloc();
	bgr24frame->data[0] = (uint8_t *)mat.data;
	avpicture_fill((AVPicture *)bgr24frame, bgr24frame->data[0], AV_PIX_FMT_BGR24, w, h);
	sws_scale(sws_ctx,
		(const uint8_t* const*)avframe->data, avframe->linesize,
		0, avframe->height, // from cols=0,all rows trans
		bgr24frame->data, bgr24frame->linesize);
 
	av_free(bgr24frame);
	sws_freeContext(sws_ctx);
	return mat;
}
int initPush(int port) {
    EndPointMgr *endPointMgr = new EndPointMgr();
    endPointMgr->addEndPoint(port);
	
    RtpStream* rtpStream = new RtpStream();

    rtpStream->SetEndPointMgr(endPointMgr);
    STREAM_PUSH_INS->init(kFrameWidth, kFrameHeight, kFrameChannel, kFrameFPS);
    STREAM_PUSH_INS->setRtpStream(rtpStream);
    STREAM_PUSH_INS->setRtpEnable(true);
    return 0;
}
int opendevice(){


	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	avdevice_register_all();


	av_dict_set(&options,"framerate","25",0);            // 抓取帧速率(25帧/sec)
    av_dict_set(&options,"follow_mouse","centered",0);   // 使抓取区域跟随鼠标
    //av_dict_set(&options,"video_size","1920x1080",0);    // 抓屏的大小
	ifmt=av_find_input_format("x11grab");
	//Grab at position 10,20
	if(avformat_open_input(&pFormatCtx,":0",ifmt,&options)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
	if(videoindex==-1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	
	
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    ///****************************



	return 0;

}
int handleVideo() {
    int ret, got_picture;
    opendevice();
    Mat frame;
	    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width,
                             pCodecCtx->height,
                             pCodecCtx->pix_fmt,
                             pCodecCtx->width,
                             pCodecCtx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BICUBIC, NULL, NULL, NULL);
	
	int nDataLen = pCodecCtx->width * pCodecCtx->height * 3;      // 假设以RGB格式储存的一帧数据大小
    uint8_t *yuv_buf = new uint8_t[nDataLen / 2];  // 一帧yuv数据的大小
	m_pYUVFrame = av_frame_alloc();
	av_image_fill_arrays(m_pYUVFrame->data, m_pYUVFrame->linesize, yuv_buf, AV_PIX_FMT_YUV420P, \
                         pCodecCtx->width, pCodecCtx->height, 1);
	m_pYUVFrame->width = pCodecCtx->width;
    m_pYUVFrame->height = pCodecCtx->height;
    m_pYUVFrame->format = AV_PIX_FMT_YUV420P;
 		
	                      
     ptmpFrame = av_frame_alloc();
     uint8_t *tmp_buf = new uint8_t;   
     av_image_fill_arrays(ptmpFrame->data, ptmpFrame->linesize, tmp_buf, pCodecCtx->pix_fmt, \
                          pCodecCtx->width, pCodecCtx->height, 1);   // 给ptmpFrame挂载内存




	while (true) {
        if(av_read_frame(pFormatCtx, packet)>=0){
				if(packet->stream_index==videoindex){
					if (avcodec_send_packet(pCodecCtx, packet)<0){
						printf("send packet error!");
					}
					got_picture = avcodec_receive_frame(pCodecCtx, ptmpFrame);
					//printf("inesize is %d   inesize is %d height is %d  \n",ptmpFrame->linesize[0],ptmpFrame->width,ptmpFrame->height);
					//printf("inesize is %d   height is %d  \n",pCodecCtx->width,pCodecCtx->pix_fmt);
					if(got_picture < 0){
						printf("Decode Error.\n");
						return -1;
					}
					if(0==got_picture){
                        sws_scale(img_convert_ctx, ptmpFrame->data, ptmpFrame->linesize, 0,
                               pCodecCtx->height,m_pYUVFrame->data, m_pYUVFrame->linesize);
                        frame=avframe2cvmat(m_pYUVFrame,0,0);
						//printf("%d\n",m_pYUVFrame->linesize[0]);
                        STREAM_PUSH_INS->push(&frame);
                        //usleep(20000);
                    }
                }
			av_free_packet(packet);
        }

    }
	sws_freeContext(img_convert_ctx);
	
}

int main(int argc, char* argv[]) {
    if (argc == 2 ) {
        initPush(atoi(argv[1]));
        handleVideo();
		av_free(m_pYUVFrame);
		av_free(ptmpFrame);
		avcodec_close(pCodecCtx);
		avformat_close_input(&pFormatCtx);
    } else {
        printf("Usage: port \n");
    }
    return 0;
}
