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

    void statClear(void);
    quint32 statTxBytes();
    quint32 statRxBytes();
    quint32 statTxPackets();
    quint32 statRxPackets();
    quint32 statCrcErrors();
    quint32 statTimeouts();

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

// statistics
    quint32 m_statTxBytes;
    quint32 m_statRxBytes;
    quint32 m_statTxPackets;
    quint32 m_statRxPackets;
    quint32 m_statCrcErrors;
    quint32 m_statTimeouts;

private:
    quint16 crc16(const QByteArray &array);
};

#endif // MICONTBUSMASTER_H
