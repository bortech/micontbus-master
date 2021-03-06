#include "micontbuspacket.h"

#include <QDataStream>
#include <QVector>

MicontBusPacket::MicontBusPacket() : m_id(0), m_cmd(0), m_addr(0), m_size(0)
{    
}

MicontBusPacket::MicontBusPacket(const MicontBusPacket &other)
{
    m_id = other.m_id;
    m_cmd = other.m_cmd;
    m_addr = other.m_addr;
    m_size = other.m_size;
    m_data = other.m_data;
}

MicontBusPacket::~MicontBusPacket()
{
}

quint8 MicontBusPacket::id() const
{
    return m_id;
}

quint8 MicontBusPacket::cmd() const
{
    return m_cmd;
}

quint16 MicontBusPacket::addr() const
{
    return m_addr;
}

quint16 MicontBusPacket::size() const
{
    return m_size;
}

QByteArray MicontBusPacket::data() const
{
    return m_data;
}

QVector<tMicontVar> MicontBusPacket::variables() const
{
    if (m_data.size() % sizeof(quint32) != 0)
        return QVector<tMicontVar>(0);

    int count = m_data.size() / sizeof(quint32);
    QVector<tMicontVar> v(count);

    QDataStream s(m_data);
    s.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < count; i++) {
        quint32 var;
        s >> var;
        v[i].u = var;
    }

    return v;
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

void MicontBusPacket::setVariable(qint32 data)
{
    QDataStream s(&m_data, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);

    s << data;
}

void MicontBusPacket::setVariable(float data)
{
    QDataStream s(&m_data, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);

    s << data;
}

void MicontBusPacket::setVariables(const QVector<tMicontVar> &vars)
{
    QDataStream s(&m_data, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);

    foreach (tMicontVar var, vars) {
        s << var.u;
    }
}

bool MicontBusPacket::parse(const QByteArray &rawPacket)
{
    quint32 size = rawPacket.size();

    if (size < 4)
        return false;

    QDataStream s(rawPacket);
    s.setByteOrder(QDataStream::LittleEndian);
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

    if ((m_cmd & 0x0f) == CMD_GETBUF_B || (m_cmd & 0x0f) == CMD_PUTBUF_B)
        s << m_size;

    if (m_cmd == CMD_PUTBUF_B ||
        m_cmd == (CMD_GETBUF_B | CMD_RESULT_OK) ||
        m_cmd == (CMD_GETSIZE | CMD_RESULT_OK))
        packet.append(m_data);

    return packet;
}

QDebug operator<<(QDebug dbg, const MicontBusPacket &packet)
{
    dbg.nospace() << "MicontBusPacket(id: " << packet.id()
                  << ", cmd: " << QString("0x%1").arg(packet.cmd(), 2, 16, QLatin1Char('0'))
                  << ", addr: " << packet.addr() << " [" << QString("0x%1").arg(packet.addr(), 2, 16, QLatin1Char('0')) << "]"
                  << ", size: " << packet.size()
                  << ", data: " << packet.data().toHex()
                  << ")";

    return dbg.maybeSpace();
}
