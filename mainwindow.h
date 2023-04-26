#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QWidget>
#include <QtWidgets>
#include <QtNetwork>
#include <QTcpServer>
#include <QTcpSocket>

#define TIMEMS qPrintable(QTime::currentTime().toString("HH:mm:ss zzz"))

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QTcpServer  *tcpServer;
    QTcpSocket  *tcpSocket;
    QTimer      *timer;

    QByteArray  messageSend;
    QByteArray  messageRecv;

    QTabWidget  *tabMain;
    QTabWidget  *tabP500;

private slots:

    void newConnection_Slot();

    void readyRead_Slot();

    void on_btnListen_clicked();

    void on_btnSearch_clicked();

    void on_btnRun_clicked();

private:
    void        DataProess(int Function);
    void        readData(QByteArray data);
    uchar       Xor(uchar *buf, int len);
    static int  strHexToDecimal(const QString &strHex);
    void loadIP(QComboBox *cbox);

    bool isOk;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
