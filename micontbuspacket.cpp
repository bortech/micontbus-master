#include "micontbuspacket.h"

#include <QDataStream>

MicontBusPacket::MicontBusPacket()
{
}

quint8 MicontBusPacket::id()
{
    return m_id;
}

quint8 MicontBusPacket::cmd()
{
    return m_cmd;
}

quint16 MicontBusPacket::addr()
{
    return m_addr;
}

quint16 MicontBusPacket::size()
{
    return m_size;
}

QByteArray MicontBusPacket::data()
{
    return m_data;
}

void MicontBusPacket::setId(quint8 id)
{
    m_id = id;
}

void MicontBusPacket::setCmd(quint8 cmd)
{
    m_cmd = cmd;
}

void MicontBusPacket::setAddr(quint16 addr)
{
    m_addr = addr;
}

void MicontBusPacket::setSize(quint16 size)
{
    m_size = size;
}

void MicontBusPacket::setData(const QByteArray &data)
{
    m_data = data;
}

void MicontBusPacket::setData(qint32 data)
{
    QDataStream s(&m_data, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);

    s << data;
}

void MicontBusPacket::setData(float data)
{
    QDataStream s(&m_data, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);

    s << data;
}

bool MicontBusPacket::parse(const QByteArray &rawPacket)
{
    quint32 size = rawPacket.size();

    if (size < 4)
        return false;

    QDataStream s(rawPacket);
    s >> m_id >> m_cmd >> m_addr;
    size -= 4;

    switch (m_cmd & 0xf) {
        case CMD_GETSIZE:
            if ((m_cmd & 0xf0) != CMD_RESULT_OK)
                break;

            if (size != 4)
                return false;

            m_data = rawPacket.right(4);
            size -= 4;
            break;
        case CMD_PUTBUF_B:
        case CMD_GETBUF_B:
            if (size < 2)
                return false;

            s >> m_size;
            size -= 2;

            if ((m_cmd & 0xf) == CMD_PUTBUF_B)
                break;

            if ((m_cmd & 0xf0) != CMD_RESULT_OK)
                break;

            if (m_size != size)
                return false;

            m_data = rawPacket.right(m_size);
            size -= m_size;
            break;
        default:
            return false;
    }

    if (size > 0)
        return false;

    return true;
}

QByteArray MicontBusPacket::serialize() const
{
    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);

    s << m_id;
    s << m_cmd;
    s << m_addr;

    if (m_cmd == CMD_GETBUF_B || m_cmd == CMD_PUTBUF_B)
        s << m_size;

    if (m_cmd == CMD_PUTBUF_B)
        packet.append(m_data);

    return packet;
}
