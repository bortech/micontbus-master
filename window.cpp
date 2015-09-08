#include "window.h"
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
#include <QMenu>
#include <QVector>
#include <QItemDelegate>
#include <QMessageBox>

#include <QtSerialPort/QSerialPortInfo>

QT_USE_NAMESPACE

/* Delegate to add Data Editor input validation */
class Delegate : public QItemDelegate
{
public:
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem & option,
                      const QModelIndex & index) const
    {
        Q_UNUSED(option)
        Q_UNUSED(index)

        QLineEdit *lineEdit = new QLineEdit(parent);
        QLocale locale = QLocale::C;
        locale.setNumberOptions(QLocale::RejectGroupSeparator | QLocale::OmitGroupSeparator);
        QValidator *validator = new QDoubleValidator;
        validator->setLocale(locale);
        lineEdit->setValidator(validator);
        return lineEdit;
    }
};

Window::Window(QWidget *parent) : QMainWindow(parent)
  , combo_port(new QComboBox())
  , combo_speed(new QComboBox())
  , spin_timeout(new QSpinBox())
  , spin_id(new QSpinBox())
  , combo_cmd(new QComboBox())
  , line_addr(new QLineEdit("0x0000"))
  , spin_addr(new QSpinBox())
  , spin_size(new QSpinBox())
  , combo_type(new QComboBox)
  , push_query(new QPushButton(tr("Query")))
  , table_editor(new QTableWidget())
  , tree_monitor(new QTreeWidget())
  , label_status(new QLabel(tr("Ready")))
{
    // fill port combo with available serial ports
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        combo_port->addItem(QString("%1 %2").arg(info.portName()).arg(info.description()), info.portName());
    combo_port->setCurrentIndex(combo_port->findData("COM4"));

    // fill speed combo with available baudrates
    foreach (qint32 baudrate, QSerialPortInfo::standardBaudRates()) {
        if (baudrate >= 9600) {
            combo_speed->addItem(QString::number(baudrate), baudrate);
        }
    }
    combo_speed->setCurrentIndex(combo_speed->findText("115200"));

    // timeout range & default value
    spin_timeout->setRange(0, 10000);
    spin_timeout->setValue(1000);

    // id range & default value
    spin_id->setRange(0, 255);
    spin_id->setValue(2);

    // fill cmd combo
    combo_cmd->addItem(cmdToString(MicontBusPacket::CMD_GETSIZE), MicontBusPacket::CMD_GETSIZE);
    combo_cmd->addItem(cmdToString(MicontBusPacket::CMD_GETBUF_B), MicontBusPacket::CMD_GETBUF_B);
    combo_cmd->addItem(cmdToString(MicontBusPacket::CMD_PUTBUF_B), MicontBusPacket::CMD_PUTBUF_B);
    connect(combo_cmd, SIGNAL(currentIndexChanged(int)),
            this, SLOT(cmdChanged()));

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

    // fill data type combo
    combo_type->addItem(tr("Variables"), DataVariables);
    combo_type->addItem(tr("Raw Bytes"), DataRawBytes);
    combo_type->addItem(tr("Tags"), DataTags);

    // size range & default value
    spin_size->setRange(1, 0xffff);
    spin_size->setValue(1);
    connect(spin_size, SIGNAL(valueChanged(int)),
            this, SLOT(countChanged()));

    // editor setup
    table_editor->setSelectionMode(QAbstractItemView::NoSelection);
    table_editor->verticalHeader()->setVisible(false);
    table_editor->setColumnCount(2);
    table_editor->setHorizontalHeaderLabels(QStringList() << tr("addr") << tr("data"));
    table_editor->setItemDelegateForColumn(1, new Delegate);
    table_editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table_editor, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(editorContextMenu(QPoint)));

    // monitor setup
    tree_monitor->setHeaderHidden(true);
    tree_monitor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree_monitor, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(monitorContextMenu(QPoint)));
    connect(tree_monitor, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(monitorItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    // settings group
    QGroupBox *group_settings = new QGroupBox(tr("Settings:"));
    QGridLayout *grid_settings = new QGridLayout;
    grid_settings->addWidget(new QLabel(tr("Port:")), 0, 0);
    grid_settings->addWidget(combo_port, 0, 1, 1, 3);
    grid_settings->addWidget(new QLabel(tr("Speed:")), 1, 0);
    grid_settings->addWidget(combo_speed, 1, 1);
    grid_settings->addWidget(new QLabel(tr("Timeout, ms:")), 1, 2);
    grid_settings->addWidget(spin_timeout, 1, 3);
    group_settings->setLayout(grid_settings);

    // query group
    QGroupBox *group_query = new QGroupBox(tr("MicontBUS Query:"));
    QGridLayout *grid_query = new QGridLayout;
    grid_query->addWidget(new QLabel(tr("Id:")), 0, 0);
    grid_query->addWidget(spin_id, 0, 1);
    grid_query->addWidget(new QLabel(tr("Cmd:")), 1, 0);
    grid_query->addWidget(combo_cmd, 1, 1, 1, 2);
    grid_query->addWidget(new QLabel(tr("Addr:")), 2, 0);
    grid_query->addWidget(spin_addr, 2, 1);
    grid_query->addWidget(line_addr, 2, 2);

    // data widgets block
    dataWidgets << new QLabel(tr("Data:"));
    grid_query->addWidget(dataWidgets.last(), 3, 0);
    dataWidgets << combo_type;
    grid_query->addWidget(combo_type, 3, 1, 1, 2);
    dataWidgets << new QLabel(tr("Count:"));
    grid_query->addWidget(dataWidgets.last(), 3, 3);
    dataWidgets << spin_size;
    grid_query->addWidget(spin_size, 3, 4);

    grid_query->setColumnStretch(5, 1);
    group_query->setLayout(grid_query);

    // transaction group
    QGroupBox *group_transaction = new QGroupBox(tr("Transaction:"));
    QGridLayout *grid_transaction = new QGridLayout;
    grid_transaction->addWidget(push_query, 0, 0);
    grid_transaction->setColumnStretch(1, 1);
    group_transaction->setLayout(grid_transaction);

    // data editor group
    QGroupBox *group_data = new QGroupBox(tr("Data editor:"));
    QGridLayout *grid_data = new QGridLayout;
    grid_data->addWidget(table_editor, 0, 0);
    group_data->setLayout(grid_data);

    label_stat_tx_bytes = new QLabel;
    label_stat_tx_packets = new QLabel;
    label_stat_rx_bytes = new QLabel;
    label_stat_rx_packets = new QLabel;
    label_stat_crc_errors = new QLabel;
    label_stat_timeouts = new QLabel;

    // monitor group
    QGroupBox *group_monitor = new QGroupBox(tr("Monitor:"));
    QGridLayout *grid_monitor = new QGridLayout;
    grid_monitor->addWidget(tree_monitor, 0, 0, 1, 6);

    grid_monitor->addWidget(new QLabel(tr("Tx Bytes:")), 1, 0);
    grid_monitor->addWidget(label_stat_tx_bytes, 1, 1);
    grid_monitor->setColumnStretch(1, 1);

    grid_monitor->addWidget(new QLabel(tr("Tx Packets:")), 1, 2);
    grid_monitor->addWidget(label_stat_tx_packets, 1, 3);
    grid_monitor->setColumnStretch(3, 1);

    grid_monitor->addWidget(new QLabel(tr("CRC Errors:")), 1, 4);
    grid_monitor->addWidget(label_stat_crc_errors, 1, 5);
    grid_monitor->setColumnStretch(5, 1);

    grid_monitor->addWidget(new QLabel(tr("Rx Bytes:")), 2, 0);
    grid_monitor->addWidget(label_stat_rx_bytes, 2, 1);

    grid_monitor->addWidget(new QLabel(tr("Rx Packets:")), 2, 2);
    grid_monitor->addWidget(label_stat_rx_packets, 2, 3);

    grid_monitor->addWidget(new QLabel(tr("Timeouts:")), 2, 4);
    grid_monitor->addWidget(label_stat_timeouts, 2, 5);

    group_monitor->setLayout(grid_monitor);

    // main layout
    QGridLayout *grid_main = new QGridLayout;
    grid_main->addWidget(group_settings, 0, 0);
    grid_main->addWidget(group_query, 1, 0);
    grid_main->addWidget(group_transaction, 2, 0);
    grid_main->addWidget(group_data, 3, 0);
    grid_main->addWidget(group_monitor, 0, 1, 4, 1);
    grid_main->addWidget(label_status, 4, 0);
    grid_main->setColumnStretch(1, 1);

    QWidget *w = new QWidget(this);
    w->setLayout(grid_main);
    setCentralWidget(w);
    setWindowTitle(tr("MicontBUS RTU Master"));
    setWindowIcon(QIcon("icons/network.svg"));

    connect(push_query, SIGNAL(clicked()),
            this, SLOT(doTransaction()));
    connect(&master, SIGNAL(response(QByteArray)),
            this, SLOT(processResponse(QByteArray)));
    connect(&master, SIGNAL(error(QString)),
            this, SLOT(processError(QString)));
    connect(&master, SIGNAL(timeout(QString)),
            this, SLOT(processTimeout(QString)));

    cmdChanged();

    master.statClear();
    updateStatistics();
}

void Window::doTransaction()
{
    setControlsEnabled(false);
    label_status->setText(tr("Opening port %1...").arg(combo_port->currentData().toString()));

    MicontBusPacket packet;
    packet.setId(spin_id->value());
    packet.setCmd(combo_cmd->currentData().toInt());
    packet.setAddr(spin_addr->value());

    if (combo_type->currentData().toInt() == DataVariables) {
        packet.setSize(spin_size->value() * 4);
    } else {
        packet.setSize(spin_size->value());
    }

    if (packet.cmd() == MicontBusPacket::CMD_PUTBUF_B) {
        QVector<tMicontVar> vars(table_editor->rowCount());

        for (int i = 0; i < table_editor->rowCount(); i++) {
            bool ok;
            vars[i].u = table_editor->item(i, 1)->text().toUInt(&ok);
            if (!ok)
                vars[i].i = table_editor->item(i, 1)->text().toInt(&ok);
            if (!ok)
                vars[i].f = table_editor->item(i, 1)->text().toFloat(&ok);
            if (!ok) {
                // MAKE HORROR
                QMessageBox msgBox;
                msgBox.setText("Data conversion error");
                msgBox.exec();
            }
        }

        packet.setVariables(vars);
    }

#ifdef QT_DEBUG
    qDebug() << packet;
#endif

    master.transaction(combo_port->currentData().toString(),
                       combo_speed->currentData().toInt(),
                       spin_timeout->value(), packet.serialize());

    logPacket(packet);
}

void Window::processResponse(const QByteArray &rawPacket)
{
    setControlsEnabled(true);

    MicontBusPacket p;
    if (!p.parse(rawPacket)) {
        processError(tr("packet parse error"));
        return;
    }

    logPacket(p);

#ifdef QT_DEBUG
    qDebug() << p;
#endif

    if (!p.data().isEmpty()) {
        table_editor->clearContents();

        if (combo_type->currentData().toInt() == DataRawBytes) {
            table_editor->setRowCount(1);
            table_editor->setItem(0, 0, new QTableWidgetItem(QString("0x%1").arg(p.addr(), 4, 16, QLatin1Char('0'))));
            table_editor->setItem(0, 1, new QTableWidgetItem(bufferToString(p.data())));
        } else if (combo_type->currentData().toInt() == DataVariables) {
            QVector<tMicontVar> vars = p.variables();
            table_editor->setRowCount(vars.count());
            for (int i = 0; i < vars.count(); i++) {
                QTableWidgetItem *item = new QTableWidgetItem;
                item->setData(Qt::UserRole, vars[i].u);
                item->setText(QString("%1").arg(vars[i].u));
                table_editor->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(p.addr() + i, 4, 16, QLatin1Char('0'))));
                table_editor->setItem(i, 1, item);
            }
        }
    }

    label_status->setText(tr("Ready"));
}

void Window::processError(const QString &s)
{
    setControlsEnabled(true);
    label_status->setText(tr("Error (%1)").arg(s));
    updateStatistics();
}

void Window::processTimeout(const QString &s)
{
    setControlsEnabled(true);
    label_status->setText(tr("Error (%1)").arg(s));
    updateStatistics();
}

void Window::addrChanged(int newAddr)
{
    line_addr->setText(QString("0x%1").arg(newAddr, 4, 16, QLatin1Char('0')));
    if (combo_cmd->currentData().toInt() == MicontBusPacket::CMD_PUTBUF_B) {
        fillDataEditor();
    }
}

void Window::countChanged()
{
    if (combo_cmd->currentData().toInt() == MicontBusPacket::CMD_PUTBUF_B) {
        fillDataEditor();
    }
}

void Window::hexAddrChanged()
{
    spin_addr->setValue(line_addr->text().toInt(0, 0));
}

void Window::cmdChanged()
{
    table_editor->clearContents();
    table_editor->setRowCount(0);

    switch (combo_cmd->currentData().toInt()) {
        case MicontBusPacket::CMD_GETSIZE:
            spin_size->setValue(0);
            toggleWidgets(dataWidgets, false);
            break;
        case MicontBusPacket::CMD_GETBUF_B:
        case MicontBusPacket::CMD_PUTBUF_B:
            spin_size->setValue(1);
            toggleWidgets(dataWidgets, true);
            if (combo_cmd->currentData().toInt() == MicontBusPacket::CMD_PUTBUF_B)
                fillDataEditor();
            break;
    }
}

void Window::fillDataEditor()
{
    table_editor->setRowCount(spin_size->value());
    for (int i = 0; i < spin_size->value(); i++) {
        table_editor->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(spin_addr->value() + i, 4, 16, QLatin1Char('0'))));
        if (!table_editor->item(i, 1))
            table_editor->setItem(i, 1, new QTableWidgetItem(QString("0")));
    }
}

void Window::monitorItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current) {
        if (current->type() == QTreeWidgetItem::UserType + 1) {
            QTreeWidgetItem *parentItem = current->parent();
            QLabel *l = qobject_cast<QLabel *>(tree_monitor->itemWidget(current->parent(), 0));

            QPoint p = current->data(0, Qt::UserRole).toPoint();
            l->setText(bufferToString(parentItem->data(0, Qt::UserRole).toByteArray(), p.x(), p.y()));
        }
    }

    if (previous) {
        if (previous->type() == QTreeWidgetItem::UserType + 1 && previous->parent() != current->parent()) {
            QLabel *l = qobject_cast<QLabel *>(tree_monitor->itemWidget(previous->parent(), 0));
            l->setText(bufferToString(previous->parent()->data(0, Qt::UserRole).toByteArray()));
        }
    }
}

void Window::monitorContextMenu(const QPoint &)
{
    QMenu *menu = new QMenu;
    menu->addAction(tr("Clear"), this, SLOT(monitorClear()));
    menu->exec(QCursor::pos());
}

void Window::editorContextMenu(const QPoint &p)
{
    QTableWidgetItem *item = table_editor->itemAt(p);
    if (!item)
        return;

    if (item->column() != 1)
        return;

    QMenu *menu = new QMenu;
    menu->addAction(tr("Unsigned Int"), this, SLOT(itemSwitchViewToUInt()));
    menu->addAction(tr("Int"), this, SLOT(itemSwitchViewToInt()));
    menu->addAction(tr("Float"), this, SLOT(itemSwitchViewToFloat()));
    menu->addAction(tr("HEX"), this, SLOT(itemSwitchViewToHex()));
    menu->addAction(tr("Bit"), this, SLOT(itemSwitchViewToBit()));
    menu->exec(QCursor::pos());
}

void Window::monitorClear()
{
    master.statClear();
    updateStatistics();
    tree_monitor->clear();

    if (combo_cmd->currentData().toInt() != MicontBusPacket::CMD_PUTBUF_B) {
        table_editor->clearContents();
        table_editor->setRowCount(0);
    }
}

void Window::itemSwitchViewToUInt()
{
    QTableWidgetItem *item = table_editor->currentItem();
    if (!item)
        return;
    item->setText(QString("%1").arg(item->data(Qt::UserRole).toUInt()));
}

void Window::itemSwitchViewToInt()
{
    QTableWidgetItem *item = table_editor->currentItem();
    if (!item)
        return;
    item->setText(QString("%1").arg(item->data(Qt::UserRole).toInt()));
}

void Window::itemSwitchViewToFloat()
{
    QTableWidgetItem *item = table_editor->currentItem();
    if (!item)
        return;
    tMicontVar var;
    var.u = item->data(Qt::UserRole).toUInt();
    item->setText(QString("%1").arg(var.f));
}

void Window::itemSwitchViewToHex()
{
    QTableWidgetItem *item = table_editor->currentItem();
    if (!item)
        return;
    item->setText(QString("0x%1").arg(item->data(Qt::UserRole).toUInt(), 8, 16, QLatin1Char('0')));
}

void Window::itemSwitchViewToBit()
{
    QTableWidgetItem *item = table_editor->currentItem();
    if (!item)
        return;
    item->setText(QString("%1").arg(item->data(Qt::UserRole).toUInt(), 32, 2, QLatin1Char('0')));
}

void Window::toggleWidgets(const QList<QWidget *> &widgets, bool show)
{
    foreach (QWidget *w, widgets) {
        w->setVisible(show);
    }
}

void Window::setControlsEnabled(bool enable)
{
    push_query->setEnabled(enable);
}

QString Window::bufferToString(const QByteArray &data, int start, int length)
{
    QString s;

    s.append("<font color=#333333>");

    for (int i = 0; i < data.size(); i++) {
        if (length > 0 && i == start)
            s.append("<b><font color=black>");
        s.append(data.mid(i, 1).toHex());
        s.append(' ');
        if (length > 0 && i == start + length - 1)
            s.append("</font></b>");
    }

    s.append("</font>");

    return s;
}

QString Window::cmdToString(quint8 cmd)
{
    QString s;

    switch (cmd & 0x0f) {
    case MicontBusPacket::CMD_GETSIZE:
        s.append(tr("GETSIZE"));
        break;
    case MicontBusPacket::CMD_GETBUF_B:
        s.append(tr("GETBUF_B"));
        break;
    case MicontBusPacket::CMD_PUTBUF_B:
        s.append(tr("PUTBUF_B"));
        break;
    }

    switch (cmd & 0xf0) {
    case 0:
        break;
    case MicontBusPacket::CMD_RESULT_OK:
        s.append(tr(" + OK"));
        break;
    case MicontBusPacket::CMD_RESULT_ERRVAR:
        s.append(tr(" + ADDR ERROR"));
        break;
    case MicontBusPacket::CMD_RESULT_ERRBSIZE:
        s.append(tr(" + SIZE ERROR"));
        break;
    default:
        s.append(tr(" + ERROR"));
        break;
    }

    return s;
}

void Window::logPacket(const MicontBusPacket &packet)
{
    QTreeWidgetItem *item = new QTreeWidgetItem;
    QTreeWidgetItem *subitem;

    subitem = new QTreeWidgetItem(QTreeWidgetItem::UserType + 1);
    subitem->setData(0, Qt::UserRole, QPoint(0, 1));
    subitem->setText(0, QString("%1: %2").arg(tr("id")).arg(packet.id()));
    item->addChild(subitem);

    subitem = new QTreeWidgetItem(QTreeWidgetItem::UserType + 1);
    subitem->setData(0, Qt::UserRole, QPoint(1, 1));
    subitem->setText(0, QString("%1: %2").arg(tr("cmd")).arg(cmdToString(packet.cmd())));
    item->addChild(subitem);

    subitem = new QTreeWidgetItem(QTreeWidgetItem::UserType + 1);
    subitem->setData(0, Qt::UserRole, QPoint(2, 2));
    subitem->setText(0, QString("%1: %2 (0x%3)").arg(tr("addr")).arg(packet.addr()).arg(packet.addr(), 4, 16, QLatin1Char('0')));
    item->addChild(subitem);

    if (packet.size() != 0) {
        subitem = new QTreeWidgetItem(QTreeWidgetItem::UserType + 1);
        subitem->setData(0, Qt::UserRole, QPoint(4, 2));
        subitem->setText(0, QString("%1: %2").arg(tr("size")).arg(packet.size()));
        item->addChild(subitem);
    }

    if (!packet.data().isEmpty()) {
        subitem = new QTreeWidgetItem(QTreeWidgetItem::UserType + 1);
        subitem->setData(0, Qt::UserRole, QPoint((packet.size()) ? 6 : 4, packet.data().length()));
        subitem->setText(0, QString("%1: %2").arg(tr("data")).arg(bufferToString(packet.data())));
        item->addChild(subitem);
    }

    QByteArray rawPacket = packet.serialize();
    item->setData(0, Qt::UserRole, rawPacket);

    tree_monitor->addTopLevelItem(item);
    tree_monitor->setItemWidget(item, 0, new QLabel(bufferToString(rawPacket)));
    tree_monitor->scrollToBottom();

    updateStatistics();
}

void Window::updateStatistics()
{
    label_stat_rx_bytes->setText(QString::number(master.statRxBytes()));
    label_stat_tx_bytes->setText(QString::number(master.statTxBytes()));
    label_stat_rx_packets->setText(QString::number(master.statRxPackets()));
    label_stat_tx_packets->setText(QString::number(master.statTxPackets()));
    QString color = (master.statCrcErrors() != 0) ? "red" : "black";
    label_stat_crc_errors->setText("<font color=" + color + ">" + QString::number(master.statCrcErrors()) + "</font>");
    label_stat_timeouts->setText(QString::number(master.statTimeouts()));
}
