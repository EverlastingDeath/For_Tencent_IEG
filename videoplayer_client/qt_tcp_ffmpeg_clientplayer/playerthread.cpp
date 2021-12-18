#include "rtpthread.h"


PlayerThread::PlayerThread(QObject *parent) :
    QThread(parent)
{
}

void  PlayerThread::run()
{

    qffmpeg->Play();
}
