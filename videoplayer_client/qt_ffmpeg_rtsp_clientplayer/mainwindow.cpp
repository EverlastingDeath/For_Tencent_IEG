#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rtspthread.h"
#include <QMessageBox>

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
        QFFmpeg *f = new QFFmpeg(this);
        f->SetUrl(ui->lineEdit->text());
        if(f->Init())
        {
            connect(f,SIGNAL(GetImage(QImage)),this,SLOT(setImage(QImage)));
            RtspThread *t = new RtspThread(this);
            t->setffmpeg(f);
            t->start();
        }
        else
        {
            QMessageBox::warning(this,"Warning","Open rtsp failed",QMessageBox::Ok);
        }

    }

}
