#ifndef MICONTBUSPACKET_H
#define MICONTBUSPACKET_H

#include <QDebug>
#include <QMetaType>
#include <QByteArray>

typedef union {
    quint32 u;
    qint32 i;
    float f;
} tMicontVar;

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

    quint8 id() const;
    void setId(quint8 id);

    quint8 cmd() const;
    void setCmd(quint8 cmd);

    quint16 addr() const;
    void setAddr(quint16 addr);

    quint16 size() const;
    void setSize(quint16 size);

    QByteArray data() const;
    void setData(const QByteArray &data);

    QVector<tMicontVar> variables() const;
    void setVariable(qint32 data);
    void setVariable(float data);
    void setVariables(const QVector<tMicontVar> &vars);

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
