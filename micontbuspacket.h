#ifndef MICONTBUSPACKET_H
#define MICONTBUSPACKET_H

#include <QDebug>
#include <QMetaType>
#include <QByteArray>

class MicontBusPacket
{    
public:

    enum CmdCode {
        CMD_GETSIZE =   0x01,
        CMD_GETBUF_B =  0x02,
        CMD_GETBUF =    0x03,
        CMD_PUTBUF_B =  0x04,
        CMD_PUTBUF =    0x05,
    };

    enum ResultCode {
        CMD_RESULT_OK =         0x10,
        CMD_RESULT_WAIT =       0x20,
        CMD_RESULT_BUSY =       0x30,
        CMD_RESULT_UCMD =       0x40,
        CMD_RESULT_ERRVAR =     0x50,
        CMD_RESULT_ERRCMD =     0x60,
        CMD_RESULT_ERRARG =     0x70,
        CMD_RESULT_ERRBSIZE =   0x80,
        CMD_RESULT_ERRBADDR =   0x90,
        CMD_RESULT_ERRPWD =     0xa0,
        CMD_RESULT_ERRLFT =     0xb0,
        CMD_RESULT_ERRDONE =    0xc0,
    };

    MicontBusPacket();
    MicontBusPacket(const MicontBusPacket &other);
    ~MicontBusPacket();

    // getters
    quint8 id() const;
    quint8 cmd() const;
    quint16 addr() const;
    quint16 size() const;
    QByteArray data() const;

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

Q_DECLARE_METATYPE(MicontBusPacket)

QDebug operator<<(QDebug dbg, const MicontBusPacket &packet);

#endif // MICONTBUSPACKET_H
