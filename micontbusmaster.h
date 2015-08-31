#ifndef MICONTBUSMASTER_H
#define MICONTBUSMASTER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QByteArray>

class MicontBusMaster : public QThread
{
    Q_OBJECT

public:
    MicontBusMaster(QObject *parent = 0);
    ~MicontBusMaster();

    void transaction(const QString &portName, qint32 baudRate, qint32 waitTimeout, const QByteArray &packet);
    void run();

signals:
    void response(const QByteArray &packet);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    QString portName;
    qint32  baudRate;
    qint32  waitTimeout;
    QByteArray packet;

    QMutex mutex;
    QWaitCondition cond;
    bool quit;

private:
    quint16 crc16(const QByteArray &array);
};

#endif // MICONTBUSMASTER_H
