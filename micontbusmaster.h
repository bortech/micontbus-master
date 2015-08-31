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

    void transaction(const QString &portName, int waitTimeout, const QByteArray &packet);
    void run();

signals:
    void response(const QByteArray &packet);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    QString portName;
    QByteArray packet;
    int waitTimeout;
    QMutex mutex;
    QWaitCondition cond;
    bool quit;

private:
    quint16 crc16(const QByteArray &array);
};

#endif // MICONTBUSMASTER_H
