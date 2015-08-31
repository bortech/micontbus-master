#ifndef MICONTBUSPACKET_H
#define MICONTBUSPACKET_H

#include <QObject>

class MicontBusPacket
{    
public:

    enum CmdCode {
        CMD_GETSIZE = 0x01,
        CMD_GETBUF_B = 0x02,
        CMD_PUTBUF_B = 0x04
    };

    enum ResultCode {
        CMD_RESULT_OK = 0x10,
    };

    MicontBusPacket();
//    MicontBusPacket(const MicontBusPacket &other);
//    ~MicontBusPacket();

    // getters
    quint8 id();
    quint8 cmd();
    quint16 addr();
    quint16 size();
    QByteArray data();

    // setters
    void setId(quint8 id);
    void setCmd(quint8 cmd);
    void setAddr(quint16 addr);
    void setSize(quint16 size);
    void setData(const QByteArray &data);
    void setData(qint32 data);
    void setData(float data);

    bool parse(const QByteArray &rawPacket);
    QByteArray serialize() const;
private:
    quint8 m_id;
    quint8 m_cmd;
    quint16 m_addr;
    quint16 m_size;
    QByteArray m_data;
};

#endif // MICONTBUSPACKET_H
