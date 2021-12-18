#ifndef GLOBAL_H
#define GLOBAL_H
#include <QSemaphore>
#include <queue>
#include <QMutex>
using namespace std;

struct Data{
    unsigned char* data;
    int size;
};

extern queue<Data> h264_picture;
extern QMutex mutex_of_picture;
extern QSemaphore picture_ok;

#endif // GLOBAL_H
