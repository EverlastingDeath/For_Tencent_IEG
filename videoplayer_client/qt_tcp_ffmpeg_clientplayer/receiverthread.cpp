#include "rtpthread.h"

ReceiverThread::ReceiverThread(QObject *parent) :
    QThread(parent)
{
}

void  ReceiverThread::run()
{
    serv->run();
}
