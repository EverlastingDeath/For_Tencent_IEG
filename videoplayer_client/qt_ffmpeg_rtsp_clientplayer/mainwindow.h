#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qffmpeg.h"
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    QFFmpeg *f;
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void setImage(const QImage &image);
    void on_pushButton_clicked();
};
#endif // MAINWINDOW_H
