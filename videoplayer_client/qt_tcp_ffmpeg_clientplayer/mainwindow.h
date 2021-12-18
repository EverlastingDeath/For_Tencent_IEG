#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "packet.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void Split_url(char *url, Server_des *serv);

private:
    Ui::MainWindow *ui;
    bool clicked;

private slots:
    void setImage(const QImage &image);
    void on_pushButton_clicked();
};
#endif // MAINWINDOW_H
