#include "packet.h"
#include <libavutil/imgutils.h>
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <iostream>
#include <QDateTime>
#include <unistd.h>

#define Tranverse16(X) ((((short)(X) & 0xff00) >> 8) |(((short)(X) & 0x00ff) << 8))
#define SECTION  count>0
const int inteval = 0x01000000;
struct RTP_header
{
    char V_P_X_CC;//协议版本号V，填充标志P，扩展标志X
    char MP_T;//标记和有效载荷类型
    short sequence_number;//序列号
    int time_stamp;//时间戳
    int SSRC;
};



NALU_Packet::NALU_Packet(QObject *parent) :
    QObject(parent)
{
    client = new QTcpSocket(this);
}

NALU_Packet::~NALU_Packet()
{
    client->close();
}

bool NALU_Packet::Init()
{

    QString adr = QString::fromStdString(server->IP);
    client->connectToHost(adr, server->Port);
    if (!client->waitForConnected((3000)))
    {
        qDebug()<<"No port is detected, please check!!!";
        return false;
    }
    qDebug() << "Client initiated!";

    return true;
}

void NALU_Packet::InputPacket(Data packet)
{
    unsigned char* buffer = packet.data;

    int pos = 0;
    static int count = 0;
    //read rtp header
    RTP_header* header = new RTP_header;
    memcpy(header,buffer,sizeof(RTP_header));
    pos+=12;
    //cout<<counter<<endl;
    //short V = (header->V_P_X_CC & 0xc0)>>6;
    //short P = (header->V_P_X_CC &0x20)>>5;
    short X = (header->V_P_X_CC & 0x10)>>4;
    //short CC = (header->V_P_X_CC & 0x0e);
    short seq_num = Tranverse16(header->sequence_number);
    if(seq.count(seq_num)) return;
    else{
        seq.insert(seq_num);
    }

    if(X){
        //profile extension
        //short define;
        short length;
        length =  buffer[pos + 3];//suppose not so long extension
        pos+=4;
        pos += (length * 4);
        //cout<<"extension length: "<<length<<endl;
        //cout<<pos<<endl;
    }
    char payload_header = buffer[pos];
    short type = payload_header & 0x1f;//后五位
    pos++;
    if(type == 24){
        //cout<<"STAP-A\n";
        while(pos<packet.size)
        {
            unsigned short NALU_size;
            memcpy(&NALU_size,buffer + pos, 2);
            NALU_size = Tranverse16(NALU_size);
            pos+=2;
            char NAL_header = buffer[pos];
            short NAL_type = NAL_header & 0x1f;
            //cout<<NAL_type;
            if(NAL_type == 7){
                count++;
                //cout<<"SPS, sequence number: "<<seq_num<<endl;
            }
            else if(NAL_type == 8){
                //cout<<"PPS, sequence number: "<<seq_num<<endl;
            }
            else if(NAL_type == 10){
                //cout<<"end of sequence, sequence number: "<<seq_num<<endl;
            }
            //cout<<pos<<endl;
            if(SECTION){
                Data d;
                d.data = new unsigned char[NALU_size+4];
                d.size = NALU_size+4;
                memcpy(d.data, &inteval, 4);
                memcpy(d.data+4, &buffer[pos], NALU_size);
                mutex_of_picture.lock();
                h264_picture.push(d);
                mutex_of_picture.unlock();
                picture_ok.release();
           }
            pos += NALU_size;
        }
    }

}

void NALU_Packet::run()
{
    QString stp = "SETUP rtsp://127.0.0.1:8554/live/track0 RTSP/1.0\r\nCSeq: 4\r\nTransport: RTP/AVP;unicast;client_port=" + QString::number(client->localPort()) + "-" + QString::number(client->localPort() + 1)+ "\r\n\r\n";
    client->write(stp.toStdString().c_str());
    client->flush();
    QByteArray datagram;
    while (client->bytesAvailable() < 123)
    datagram = client->readAll();
    qDebug()<< datagram.size();
    QString ply = "PLAY rtsp://127.0.0.1:8554/live RTSP/1.0\r\nCSeq: 5\r\nSession: 66334873\r\nRange: npt=0.000-\r\n\r\n";
    client->write(ply.toStdString().c_str());
    while (client->bytesAvailable() < 123)
    datagram = client->readAll();
    qDebug()<<"Begin to reiceve packet"<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    while (true) {
        datagram = client->readAll();
        //qDebug()<< datagram;
        Data temp;
        temp.size = datagram.size();
        temp.data = new unsigned char[temp.size];
        memcpy(temp.data, datagram.data(), temp.size);
        InputPacket(temp);
        delete [] temp.data;
    }

}

