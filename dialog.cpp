#include "dialog.h"
#include "micontbuspacket.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>

#include <QtSerialPort/QSerialPortInfo>

QT_USE_NAMESPACE

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , combo_port(new QComboBox())
    , combo_speed(new QComboBox())
    , spin_timeout(new QSpinBox())
    , spin_id(new QSpinBox())
    , combo_cmd(new QComboBox())
    , line_addr(new QLineEdit("0x0000"))
    , spin_addr(new QSpinBox())
    , spin_size(new QSpinBox())
    , push_query(new QPushButton(tr("Query")))
    , table_editor(new QTableWidget())
    , tree_monitor(new QTreeWidget())
    , label_status(new QLabel(tr("Ready")))    
{
    // fill port combo with available serial ports
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        combo_port->addItem(QString("%1 %2").arg(info.portName()).arg(info.description()), info.portName());

    // fill speed combo with available baudrates
    foreach (qint32 baudrate, QSerialPortInfo::standardBaudRates()) {
        if (baudrate >= 9600)
            combo_speed->addItem(QString::number(baudrate));
    }
    combo_speed->setCurrentIndex(combo_speed->findText("115200"));

    // timeout range & default value
    spin_timeout->setRange(0, 10000);
    spin_timeout->setValue(1000);

    // id range & default value
    spin_id->setRange(0, 255);
    spin_id->setValue(33);

    // fill cmd combo
    combo_cmd->addItem("0x01 GETSIZE", MicontBusPacket::CMD_GETSIZE);
    combo_cmd->addItem("0x02 GETBUF_B", MicontBusPacket::CMD_GETBUF_B);
    combo_cmd->addItem("0x04 PUTBUF_B", MicontBusPacket::CMD_PUTBUF_B);

    // addr range & default value
    spin_addr->setRange(0, 0xffff);
    spin_addr->setValue(0);
    connect(spin_addr, SIGNAL(valueChanged(int)),
            this, SLOT(addrChanged(int)));

    // addr hex line edit setup
    QRegExp rx("0x[0-9|a-f|A-F]{1,4}");
    QValidator *v = new QRegExpValidator(rx);
    line_addr->setValidator(v);
    line_addr->setFixedWidth(50);
    connect(line_addr, SIGNAL(editingFinished()),
            this, SLOT(hexAddrChanged()));

    // size range & default value
    spin_size->setRange(0, 0xffff);
    spin_size->setValue(4);   

    // editor setup
    table_editor->setSelectionMode(QAbstractItemView::NoSelection);
    table_editor->verticalHeader()->setVisible(false);
    table_editor->setColumnCount(2);
    table_editor->setHorizontalHeaderLabels(QStringList() << tr("addr") << tr("value"));

    // monitor setup
    tree_monitor->setHeaderHidden(true);

    // settings group
    QGroupBox *group_settings = new QGroupBox(tr("Settings:"));
    QGridLayout *grid_settings = new QGridLayout;
    grid_settings->addWidget(new QLabel(tr("Port:")), 0, 0);
    grid_settings->addWidget(combo_port, 0, 1);
    grid_settings->addWidget(new QLabel(tr("Speed:")), 0, 2);
    grid_settings->addWidget(combo_speed, 0, 3);
    grid_settings->addWidget(new QLabel(tr("Timeout, ms:")), 1, 2);
    grid_settings->addWidget(spin_timeout, 1, 3);
    group_settings->setLayout(grid_settings);

    // query group
    QGroupBox *group_query = new QGroupBox(tr("MicontBUS Query:"));
    QGridLayout *grid_query = new QGridLayout;
    grid_query->addWidget(new QLabel(tr("Id:")), 0, 0);
    grid_query->addWidget(spin_id, 0, 1);
    grid_query->addWidget(new QLabel(tr("Cmd:")), 0, 3);
    grid_query->addWidget(combo_cmd, 0, 4);
    grid_query->addWidget(new QLabel(tr("Addr:")), 1, 0);
    grid_query->addWidget(spin_addr, 1, 1);
    grid_query->addWidget(line_addr, 1, 2);
    grid_query->addWidget(new QLabel(tr("Size:")), 1, 3);
    grid_query->addWidget(spin_size, 1, 4);
    grid_query->addWidget(push_query, 0, 5, 2, 1);
    group_query->setLayout(grid_query);

    // data editor group
    QGroupBox *group_data = new QGroupBox(tr("Data editor:"));
    QGridLayout *grid_data = new QGridLayout;
    grid_data->addWidget(table_editor, 0, 0);
    group_data->setLayout(grid_data);

    // monitor group
    QGroupBox *group_monitor = new QGroupBox(tr("Monitor:"));
    QGridLayout *grid_monitor = new QGridLayout;
    grid_monitor->addWidget(tree_monitor, 0, 0);
    group_monitor->setLayout(grid_monitor);

    // main layout
    QGridLayout *grid_main = new QGridLayout;
    grid_main->addWidget(group_settings, 0, 0);
    grid_main->addWidget(group_query, 1, 0);
    grid_main->addWidget(group_data, 2, 0);
    grid_main->addWidget(group_monitor, 0, 1, 3, 1);
    grid_main->addWidget(label_status, 3, 0);
    setLayout(grid_main);

    setWindowTitle(tr("MicontBUS RTU Master"));

    connect(push_query, SIGNAL(clicked()),
            this, SLOT(doTransaction()));
    connect(&thread, SIGNAL(response(QByteArray)),
            this, SLOT(processResponse(QByteArray)));
    connect(&thread, SIGNAL(error(QString)),
            this, SLOT(processError(QString)));
    connect(&thread, SIGNAL(timeout(QString)),
            this, SLOT(processTimeout(QString)));
}

void Dialog::doTransaction()
{
    setControlsEnabled(false);
    label_status->setText(tr("Opening port %1...").arg(combo_port->currentData().toString()));

    MicontBusPacket packet;
    packet.setId(spin_id->value());
    packet.setCmd(combo_cmd->currentData().toInt());
    packet.setAddr(spin_addr->value());
    packet.setSize(spin_size->value());
    thread.transaction(combo_port->currentData().toString(), spin_timeout->value(), packet);
}

void Dialog::processResponse(const QByteArray &rawPacket)
{
    setControlsEnabled(true);

    label_status->setText(tr("Ready"));
    logPacket(rawPacket);

    MicontBusPacket p;
    if (!p.parse(rawPacket)) {
        processError(tr("packet parse error"));
        return;
    }

    table_editor->setRowCount(1);
    table_editor->setItem(0, 0, new QTableWidgetItem(QString("0x%1").arg(p.addr(), 4, 16, QLatin1Char('0'))));
    table_editor->setItem(0, 1, new QTableWidgetItem(QString(p.data().toHex())));
}

void Dialog::processError(const QString &s)
{
    setControlsEnabled(true);
    label_status->setText(tr("Error (%1)").arg(s));
}

void Dialog::processTimeout(const QString &s)
{
    setControlsEnabled(true);
    label_status->setText(tr("Error (%1)").arg(s));
}

void Dialog::addrChanged(int newAddr)
{
    line_addr->setText(QString("0x%1").arg(newAddr, 4, 16, QLatin1Char('0')));
}

void Dialog::hexAddrChanged()
{
    spin_addr->setValue(line_addr->text().toInt(0, 0));
}

void Dialog::setControlsEnabled(bool enable)
{
    push_query->setEnabled(enable);
}

void Dialog::logPacket(const QByteArray &packet)
{
    QTreeWidgetItem *item = new QTreeWidgetItem;
    QString s;

    for (int i = 0; i < packet.size(); i++) {
        s.append(packet.mid(i, 1).toHex());
        s.append(' ');
    }

    item->setText(0, s);
    tree_monitor->addTopLevelItem(item);
    tree_monitor->scrollToBottom();
}
