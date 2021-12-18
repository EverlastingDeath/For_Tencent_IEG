#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <string>
#include "rtpthread.h"
#include <QString>
#include <iostream>
#include <unistd.h>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Split_url(char* url, Server_des *serv)
{
    const char separator[] = ":/";
    char *save, *tmp_str;
    tmp_str = strtok_r(url, separator, &save);
    tmp_str = NULL;
    tmp_str = strtok_r(NULL, separator, &save);
    if (tmp_str == nullptr)
    {
        qDebug()<<"No IP is detected, please check!!!";
    }
    serv->IP = tmp_str;
    QString str(save);
    serv->Port = str.toUShort();
}


void MainWindow::setImage(const QImage &image)
{
    if (image.height() > 0) {
        QPixmap pix = QPixmap::fromImage(image.scaled(ui->label_2->width(), ui->label_2->height()));
        ui->label_2->setPixmap(pix);
    }
}


void MainWindow::on_pushButton_clicked()
{
    if (ui->lineEdit->text() == 0)
    {
        QMessageBox::warning(this,"Warning","Please input address",QMessageBox::Ok);
    }
    else
    {
        NALU_Packet *f = new NALU_Packet(this);
        QFFmpeg *p = new QFFmpeg(this);

        string str = ui->lineEdit->text().toStdString();
        char *ch = const_cast<char*>(str.c_str());

        Split_url(ch, f->server);
        if (f->Init()) {
            ReceiverThread *R = new ReceiverThread(this);
            R->setserver(f);
            R->start();
            int count = 0;
            while (count < 2) {
                QMessageBox::warning(this,"Warning","Waiting for NALU packets",QMessageBox::Ok);
                count++;
                sleep(2);
                mutex_of_picture.lock();
                if (!h264_picture.empty()) {
                    mutex_of_picture.unlock();
                    break;
                }
                mutex_of_picture.unlock();
            }

            qDebug()<< count;
            if (count < 2) {
                if(p->Init())
                {
                    connect(p,SIGNAL(GetImage(QImage)),this,SLOT(setImage(QImage)));
                    PlayerThread *P = new PlayerThread(this);
                    P->set_player(p);
                    P->start();
                }
                else
                {
                    QMessageBox::warning(this,"Warning","Open rtsp failed",QMessageBox::Ok);
                }
            }
            else {
                QMessageBox::warning(this,"Warning","Fail to receive packets",QMessageBox::Ok);
            }
        }


    }
}
