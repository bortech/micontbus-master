#ifndef MICONTBUSMASTER_H
#define MICONTBUSMASTER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "micontbuspacket.h"

class MicontBusPacket;

class MicontBusMaster : public QThread
{
    Q_OBJECT

public:
    MicontBusMaster(QObject *parent = 0);
    ~MicontBusMaster();

    void transaction(const QString &portName, int waitTimeout, const MicontBusPacket &packet);
    void run();

signals:
    void response(const QByteArray &packet);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    QString portName;
    MicontBusPacket packet;
    int waitTimeout;
    QMutex mutex;
    QWaitCondition cond;
    bool quit;

private:
    quint16 crc16(const QByteArray &array);
};

#endif // MICONTBUSMASTER_H
