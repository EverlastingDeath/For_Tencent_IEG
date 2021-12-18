/*
** Copyright (C) 2018 Wang Yaofu
** All rights reserved.
**
**Author:Wang Yaofu voipman@qq.com
**Description: The source file of Main process.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include<netdb.h>
#include "libavcodec/avcodec.h"
#include "SDL2/SDL.h"

extern AVCodec ff_h264_decoder;
extern AVCodecParser ff_h264_parser;

#define MAX_PACK_SIZE 6220800
struct ImagePacket {
    unsigned char *buf_;
    int len_;
    int offset_;
    int end_;
    unsigned int frame_index_;
};

struct ImagePacket *gImagePacket = NULL;

void initPacket() {
    gImagePacket = (struct ImagePacket *) malloc(sizeof(struct ImagePacket));
    gImagePacket->buf_ = (unsigned char *) malloc(MAX_PACK_SIZE);
    gImagePacket->frame_index_ = 0;
}

int resetPacket(struct ImagePacket *packetPtr) {
    memset(packetPtr->buf_, MAX_PACK_SIZE, 0);
    packetPtr->len_ = 0;
    packetPtr->offset_ = 0;
    packetPtr->end_ = 0;
    return 0;
}


const static int kRtpOffset = 14;

int parseRTP(const char *inBuff, int len, struct ImagePacket *packetPtr) {
    int naluType = 0;
    int nalunri = 0;
    int naluf = 0;

    printf("parseRTP BEGIN\n");
    fflush(stdout);
    int isEnd = 0;
    if ((int) (inBuff[1] >> 7 & 0x01) == 1) {
        isEnd = 0;
    }

    char naluHeader = inBuff[12];
    naluType = naluHeader & 0x1f;
    nalunri = naluHeader >> 5 & 0x03;
    naluf = naluHeader >> 7 & 0x01;

    char fu_h_byte[5] = {0};
    fu_h_byte[0] = 0;
    fu_h_byte[1] = 0;
    fu_h_byte[2] = 0;
    fu_h_byte[3] = 1;

    if (28 == naluType) {
        // H264 Begin
        int fu_type = 0;
        int fu_r = 0;
        int fu_e = 0;
        int fu_s = 0;

        char fu_header = inBuff[13];
        fu_type = fu_header & 0x1f;
        fu_r = fu_header >> 5 & 0x01;
        fu_e = fu_header >> 6 & 0x01;
        fu_s = fu_header >> 7 & 0x01;

        if (fu_s == 1) {
            //buffStream.doSwitch();
            // start of fu-a
            fu_h_byte[4] = ((fu_type & 0x1f) | (nalunri << 5 & 0x60) | (naluf << 7 & 0x80));
            //buffStream.getOutputStream().write(fu_h_byte, 0, 5);
            //buffStream.getOutputStream().write(inBuff, 14, len - 14);
            memcpy(packetPtr->buf_ + packetPtr->offset_, inBuff + kRtpOffset, len - kRtpOffset);
            packetPtr->len_ += len - kRtpOffset;
            packetPtr->offset_ += len - kRtpOffset;
        } else {
            // end of fu-a
            //buffStream.getOutputStream().write(inBuff, 14, len - 14);
            memcpy(packetPtr->buf_ + packetPtr->offset_, inBuff + kRtpOffset, len - kRtpOffset);
            packetPtr->len_ += len - kRtpOffset;
            packetPtr->offset_ += len - kRtpOffset;
            isEnd = 1;
        }

    } else {
        fu_h_byte[4] = ((naluType & 0x1f) | (nalunri << 5 & 0x60) | (naluf << 7 & 0x80));
        //buffStream.getOutputStream().write(fu_h_byte, 0, 5);
        //buffStream.getOutputStream().write(inBuff, 13, len - 13);
        memcpy(packetPtr->buf_ + packetPtr->offset_, inBuff + kRtpOffset, len - kRtpOffset);
        packetPtr->len_ += len - kRtpOffset;
        packetPtr->offset_ += len - kRtpOffset;
    }
    if (isEnd) {
        packetPtr->end_ = 1;
    }
    //printf("parseRTP END\n");
    //fflush(stdout);
    return 0;
}

AVCodec *gCodec = NULL;
AVCodecContext *gCodec_ctx = NULL;
AVCodecParserContext *gParser = NULL;
AVFrame *gAVFrame = NULL;

void doAVCodecInit() {
    avcodec_register(&ff_h264_decoder);
    av_register_codec_parser(&ff_h264_parser);

    gCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!gCodec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    gCodec_ctx = avcodec_alloc_context3(gCodec);
    if (!gCodec_ctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    if (avcodec_open2(gCodec_ctx, gCodec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    gParser = av_parser_init(AV_CODEC_ID_H264);
    if (!gParser) {
        fprintf(stderr, "Could not create H264 parser\n");
        exit(1);
    }

    gAVFrame = av_frame_alloc();
    if (!gAVFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    initPacket();
}


#define LOAD_YUV420P 0

#define HAS_BORDER     1

const int bpp = 12;

//const int screen_w = 1536, screen_h = 864;
const int screen_w = 1280, screen_h = 720;
const int pixel_w = 1918, pixel_h = 888;

//const int screen_w=1920,screen_h=1080;
//const int pixel_w=1920,pixel_h=1080;

SDL_Window *gScreen = NULL;
SDL_Renderer *gSdlRenderer = NULL;
SDL_Texture *gSdlTexture = NULL;
SDL_Rect sdlRect;

//Refresh Event
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
int thread_exit = 0;

void sig_action() {
    printf("process exit!!!");
    fflush(stdout);
    _exit(0);
}

int refresh_video(void *opaque) {
    printf("thread woeking! ****************\n");
    thread_exit == 0;
    while (thread_exit == 0) {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(10);
    }
    return 0;
}

int doSDLInit() {
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    //SDL 2.0 Support for multiple windows
    gScreen = SDL_CreateWindow("AI-Detect-Four-View", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!gScreen) {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }
    gSdlRenderer = SDL_CreateRenderer(gScreen, -1, 0);
    int pixformat = SDL_PIXELFORMAT_IYUV;

    gSdlTexture = SDL_CreateTexture(gSdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

    int border = 0;


    sdlRect.x = 0 + border;
    sdlRect.y = 0 + border;
    sdlRect.w = screen_w - border * 2;
    sdlRect.h = screen_h - border * 2;

    SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
}

#define UDP_PACK_SIZE 1500


static void yuv_show(unsigned char *buf[], int wrap[], int xsize, int ysize) {
    int i;
    for (i = 0; i < ysize; i++) {
        memcpy(gImagePacket->buf_ + gImagePacket->offset_, buf[0] + i * wrap[0], xsize);
        gImagePacket->len_ += xsize;
        gImagePacket->offset_ += xsize;
    }
    for (i = 0; i < ysize / 2; i++) {
        //fwrite(buf[1] + i * wrap[1], 1, xsize/2, f);
        memcpy(gImagePacket->buf_ + gImagePacket->offset_, buf[1] + i * wrap[1], xsize / 2);
        gImagePacket->len_ += xsize / 2;
        gImagePacket->offset_ += xsize / 2;
    }
    for (i = 0; i < ysize / 2; i++) {
        //fwrite(buf[2] + i * wrap[2], 1, xsize/2, f);
        memcpy(gImagePacket->buf_ + gImagePacket->offset_, buf[2] + i * wrap[2], xsize / 2);
        gImagePacket->len_ += xsize / 2;
        gImagePacket->offset_ += xsize / 2;
    }

    printf("yuv_show\n");
    fflush(stdout);
    SDL_Event event;
    SDL_WaitEvent(&event);
    printf("yuv_show\n");
    printf("%d************************************************************\n",event.type);
    if (event.type == REFRESH_EVENT) {
    printf("yuv_show\n");  
        SDL_UpdateTexture(gSdlTexture, NULL, gImagePacket->buf_, pixel_w);
        SDL_RenderClear(gSdlRenderer);
        SDL_RenderCopy(gSdlRenderer, gSdlTexture, NULL, &sdlRect);
        SDL_RenderPresent(gSdlRenderer);
        printf("sdl showing ");
        //Delay 40ms
        SDL_Delay(1);
    } else if (event.type == SDL_QUIT) {
        _exit(0);
    }

    //cv::Mat fourViewImage(ysize*1.5, xsize, CV_8UC1, (unsigned char *) gImagePacket->buf_);
    //cv::imshow("AI-Detect-Four-View", fourViewImage);
    //cv::waitKey(1);
}

static int doDecodeFrame(AVPacket *pkt, unsigned int frame_index) {
    int got_frame = 0;
    do {
        int len = avcodec_decode_video2(gCodec_ctx, gAVFrame, &got_frame, pkt);
        if (len < 0) {
            fprintf(stderr, "Error while decoding frame %d\n", frame_index);
            return len;
        }
        if (got_frame) {
            printf("Got frame %d\n", frame_index);
            fflush(stdout);

            yuv_show(gAVFrame->data, gAVFrame->linesize, gAVFrame->width, gAVFrame->height);
        }
    } while (0);
    return 0;
}

int doPackDecode(struct ImagePacket *packetPtr) {
    uint8_t *data = NULL;
    int size = 0;
    int bytes_used = av_parser_parse2(gParser, gCodec_ctx, &data, &size, packetPtr->buf_, packetPtr->len_, 0, 0,
                                      AV_NOPTS_VALUE);
                    printf("parse %d \n",bytes_used);
                    printf("packet.size %d \n",size);
    if (size == 0) {
        return -1;
    }

    // We have data of one packet, decode it; or decode whatever when ending
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = data;
    packet.size = size;
    int ret = doDecodeFrame(&packet, packetPtr->frame_index_);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

void recv_frame(int sockfd, struct sockaddr *servaddr ,socklen_t clilen) {
    int n;
    
    char mesg[UDP_PACK_SIZE + 1] = {0};


    struct ImagePacket *imagePacket = (struct ImagePacket *) malloc(sizeof(struct ImagePacket));
    imagePacket->buf_ = (unsigned char *) malloc(MAX_PACK_SIZE);
    resetPacket(imagePacket);

    for (;;) {
        
        /* waiting for receive data */
        n = recvfrom(sockfd, mesg, UDP_PACK_SIZE, 0, servaddr , &clilen);
        if (n > 0) {
            //printf("recv len:%d\n", n);
            parseRTP(mesg, n, imagePacket);
            if (imagePacket->end_) {
                doPackDecode(imagePacket);
                resetPacket(imagePacket);
                imagePacket->frame_index_++;

                resetPacket(gImagePacket);
            }
        }
        if(n<=0){printf("no recv \n");}
    }
}

int udp_server(int port,char * ip) {
    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */
/* init servaddr */
    //struct hostent* h;
    //if ( (h = gethostbyname(ip)) == 0 )   // 指定服务端的ip地址。
    //{ printf("gethostbyname failed.\n"); close(sockfd); return -1; }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //memcpy(&servaddr.sin_addr,h->h_addr,h->h_length);
    servaddr.sin_addr.s_addr =inet_addr(ip);
    servaddr.sin_port = htons(port);
/* bind address and port to socket */

char sendline[20];

int send_length=0;
sprintf(sendline,"hello server!");
send_length=sendto(sockfd,sendline,sizeof(sendline),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
if(send_length<0){
perror("sendto()error");
exit(1);}
printf("send_length =%d\n",send_length); 


    recv_frame(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr));
    return 0;
}

int main(int argc, char *argv[]) {

    signal(SIGINT, sig_action);
    signal(SIGTERM, sig_action);
    if (argc == 3) {
        // h264_video_decode(argv[1], argv[2]);
        doAVCodecInit();
        doSDLInit();
        int ii=atoi(argv[1]);
        udp_server(ii,argv[2]);
    } else {
        printf("Usage: port\n", argv[0]);
    }
    return 0;
}
