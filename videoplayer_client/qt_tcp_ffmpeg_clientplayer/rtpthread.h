#ifndef RTPTHREAD_H
#define RTPTHREAD_H

#include <QThread>
#include <packet.h>

class ReceiverThread : public QThread
{
    Q_OBJECT
public:
    explicit ReceiverThread(QObject *parent = 0);

    void run();
    void setserver(NALU_Packet *f){serv = f;}

private:
    NALU_Packet *serv;
signals:

public slots:

};

class PlayerThread : public QThread
{
    Q_OBJECT
public:
    explicit PlayerThread(QObject *parent = 0);

    void run();
    void set_player(QFFmpeg *f){qffmpeg = f;}

private:
    QFFmpeg *qffmpeg;
signals:

public slots:

};

#endif // RTPTHREAD_H
