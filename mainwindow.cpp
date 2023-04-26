#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    isOk = false;

    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection_Slot()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_btnSearch_clicked()));

    loadIP(ui->cboxIP);
}

MainWindow::~MainWindow()
{
    tcpServer->close();
    delete ui;
}

void MainWindow::loadIP(QComboBox *cbox)
{
    //获取本机所有IP
    static QStringList ips;
    if (ips.count() == 0) {
#ifdef Q_OS_WASM
        ips << "127.0.0.1";
#else
        QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
        foreach (const QNetworkInterface  &netInterface, netInterfaces) {
            //移除虚拟机和抓包工具的虚拟网卡
            QString humanReadableName = netInterface.humanReadableName().toLower();
            if (humanReadableName.startsWith("vmware network adapter") || humanReadableName.startsWith("npcap loopback adapter")) {
                continue;
            }

            //过滤当前网络接口
            bool flag = (netInterface.flags() == (QNetworkInterface::IsUp | QNetworkInterface::IsRunning | QNetworkInterface::CanBroadcast | QNetworkInterface::CanMulticast));
            if (flag) {
                QList<QNetworkAddressEntry> addrs = netInterface.addressEntries();
                foreach (QNetworkAddressEntry addr, addrs) {
                    //只取出IPV4的地址
                    if (addr.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        QString ip4 = addr.ip().toString();
                        if (ip4 != "127.0.0.1") {
                            ips << ip4;
                        }
                    }
                }
            }
        }
#endif
    }

    cbox->clear();
    cbox->addItems(ips);
    if (!ips.contains("127.0.0.1")) {
        cbox->addItem("127.0.0.1");
    }
}


/// @brief 按照用户点击的按钮来处理即将发送到P500+的数据包
/// @param[in]  Function    用户选择的功能
/// @param[out] messageSend 要发送给P500+的数据包
void MainWindow::processData(int Function)
{
    messageSend = 0;
    switch (Function) {
    case 1:{
        uchar buf[] = {0xff, 0x11, 0x05, 0x01, 0xea};
        for (uint i=0;i < sizeof (buf);i++) {
            messageSend.append(buf[i]);
        }
    }break;
    case 2:{
        uchar buf[12] = {0xff, 0x11, 0x0d, 0x02};
        uint FlowRate = ui->txtFlowRate->text().toInt() *10;
        buf[4] = (FlowRate >> 8) & 0xff;
        buf[5] = (FlowRate >> 0) & 0xff;
        uint MaxPress = ui->txtMaxPress->text().toInt() *10;
        buf[6] = (MaxPress >> 8) & 0xff;
        buf[7] = (MaxPress >> 0) & 0xff;
        uint MinPress = ui->txtMinPress->text().toInt() *10;
        buf[8] = (MinPress >> 8) & 0xff;
        buf[9] = (MinPress >> 0) & 0xff;
        buf[10] = 0x01;
        buf[11] = 0x00;
        for (uint i=0;i < sizeof (buf);i++) {
            messageSend.append(buf[i]);
        }
        messageSend.append(QUIHelperData::getOrCode(messageSend));
    }break;
    case 3:{
        uchar buf[12] = {0xff, 0x11, 0x0d, 0x02};
        uint FlowRate = ui->txtFlowRate->text().toInt() *10;
        buf[4] = (FlowRate >> 8) & 0xff;
        buf[5] = (FlowRate >> 0) & 0xff;
        uint MaxPress = ui->txtMaxPress->text().toInt() *10;
        buf[6] = (MaxPress >> 8) & 0xff;
        buf[7] = (MaxPress >> 0) & 0xff;
        uint MinPress = ui->txtMinPress->text().toInt() *10;
        buf[8] = (MinPress >> 8) & 0xff;
        buf[9] = (MinPress >> 0) & 0xff;
        buf[10] = 0x00;
        buf[11] = 0x00;
        for (uint i=0;i < sizeof (buf);i++) {
            messageSend.append(buf[i]);
        }
        messageSend.append(QUIHelperData::getOrCode(messageSend));
    }break;
    default:break;
    }
}

void MainWindow::readData(QByteArray data)
{
    QByteArray temp;
    QString strHex;
    data.resize(15);
    if(data.at(0) == '\xff' && data.at(1) == '\x01')
    {
        if(data.at(4) == '\x00')
            ui->labStatus->setText("停止");
        else
            ui->labStatus->setText("运行");
        uint Pressure = QUIHelperData::strHexToDecimal(data.toHex().left(10+4).right(4)) / 10;
        uint Flowrate = QUIHelperData::strHexToDecimal(data.toHex().left(10+4+4).right(4)) / 10;
        uint MaxPressure = QUIHelperData::strHexToDecimal(data.toHex().left(10+4+4+4).right(4)) / 10;
        uint MinPressure = QUIHelperData::strHexToDecimal(data.toHex().left(10+4+4+4+4).right(4)) / 10;
        ui->labPress->setText(QString::number(Pressure) + " MPa");
        ui->labFlowrate->setText(QString::number(Flowrate) + " ml/min");
        ui->labMaxpress->setText(QString::number(MaxPressure) + " MPa");
        ui->labMinpress->setText(QString::number(MinPressure) + " MPa");
    }
}

void MainWindow::newConnection_Slot()
{
    tcpSocket = tcpServer->nextPendingConnection();

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead_Slot()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected_Slot()));

    QString client = tcpSocket->peerAddress().toString() + ":" + QString::number(tcpSocket->peerPort());
    clients.append(client);
    qDebug() << clients;
}

void MainWindow::disconnected_Slot()
{
    tcpSocket = (QTcpSocket *)this->sender();
    for(int i=0;i<clients.length();i++)
    {
        QString client = tcpSocket->peerAddress().toString() + ":" + QString::number(tcpSocket->peerPort());
        if (clients.at(i) == client)
            clients.removeAt(i);
    }
    qDebug() << clients;
}

void MainWindow::readyRead_Slot()
{
    tcpSocket = (QTcpSocket *)this->sender();
    QString client = tcpSocket->peerAddress().toString() + ":" + QString::number(tcpSocket->peerPort());
    if(client == ui->cboxP500->currentText()) {
        p500Recv = tcpSocket->readAll();
        qDebug() << "P500 IP" << client << "recv:" << p500Recv;
    }
    else if (client == ui->cboxMCU->currentText()) {
        mcuRecv = tcpSocket->readAll();
        qDebug() << "MCU IP" << client << "recv:" << mcuRecv;
    }
}

void MainWindow::on_btnListen_clicked()
{
    if (ui->btnListen->text() == "监听") {
        isOk = tcpServer->listen(QHostAddress(ui->cboxIP->currentText()), ui->txtPort->text().toUInt());
        if (isOk) {
            ui->btnListen->setText("关闭");
            qDebug()<<"监听成功";
        } else {
            qDebug()<<"监听失败";
        }
    } else {
        isOk = false;
        tcpServer->close();
        ui->btnListen->setText("监听");
    }
}

void MainWindow::on_btnFresh_clicked()
{
    ui->cboxP500->clear();
    ui->cboxMCU->clear();
    ui->cboxP500->addItem(" ");
    ui->cboxMCU->addItem(" ");
    ui->cboxP500->addItems(clients);
    ui->cboxMCU->addItems(clients);
    ui->cboxP500->setCurrentIndex(0);
    ui->cboxMCU->setCurrentIndex(0);
}

void MainWindow::on_btnSearch_clicked()
{
    readData(p500Recv);
    processData(1);
    QStringList list = ui->cboxP500->currentText().split(":");
    tcpSocket->connectToHost(list.at(0), list.at(1).toUInt());
    tcpSocket->write(messageSend);
    timer->start(1000);
    ui->btnSearch->setText("查询中…");
}

void MainWindow::on_btnRun_clicked()
{
    QStringList list = ui->cboxP500->currentText().split(":");
    tcpSocket->connectToHost(list.at(0), list.at(1).toUInt());
    if (ui->btnRun->text() == "运行")
    {
        processData(2);
        tcpSocket->write(messageSend);
        ui->btnRun->setText("停止");
    }
    else
    {
        processData(3);
        tcpSocket->write(messageSend);
        ui->btnRun->setText("运行");
    }
}


