#include "global.h"

queue<Data> h264_picture;
QMutex mutex_of_picture;
QSemaphore picture_ok(0);
