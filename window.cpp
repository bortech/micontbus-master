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
#include <QTextEdit>
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
  , comboPort(new QComboBox())
  , comboSpeed(new QComboBox())
  , spinTimeout(new QSpinBox())
  , spinId(new QSpinBox())
  , comboCmd(new QComboBox())
  , lineAddr(new QLineEdit("0x0000"))
  , spinAddr(new QSpinBox())
  , spinSize(new QSpinBox())
  , comboType(new QComboBox)
  , pushQuery(new QPushButton(QIcon("icons/transaction.svg"), tr("Query")))
  , tableVariables(new QTableWidget())
  , tableTags(new QTableWidget())
  , textRaw(new QTextEdit())
  , treeMonitor(new QTreeWidget())
  , labelStatus(new QLabel(tr("Ready")))
{
    // fill port combo with available serial ports
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        comboPort->addItem(QString("%1 %2").arg(info.portName()).arg(info.description()), info.portName());
    comboPort->setCurrentIndex(comboPort->findData("COM4"));

    // fill speed combo with available baudrates
    foreach (qint32 baudrate, QSerialPortInfo::standardBaudRates()) {
        if (baudrate >= 9600) {
            comboSpeed->addItem(QString::number(baudrate), baudrate);
        }
    }
    comboSpeed->setCurrentIndex(comboSpeed->findText("115200"));

    // timeout range & default value
    spinTimeout->setRange(0, 10000);
    spinTimeout->setValue(1000);

    // id range & default value
    spinId->setRange(0, 255);
    spinId->setValue(2);

    // fill cmd combo
    comboCmd->addItem(cmdToString(MicontBusPacket::CMD_GETSIZE), MicontBusPacket::CMD_GETSIZE);
    comboCmd->addItem(cmdToString(MicontBusPacket::CMD_GETBUF_B), MicontBusPacket::CMD_GETBUF_B);
    comboCmd->addItem(cmdToString(MicontBusPacket::CMD_PUTBUF_B), MicontBusPacket::CMD_PUTBUF_B);
    connect(comboCmd, SIGNAL(currentIndexChanged(int)),
            this, SLOT(cmdChanged()));

    // addr range & default value
    spinAddr->setRange(0, 0xffff);
    spinAddr->setValue(0);
    connect(spinAddr, SIGNAL(valueChanged(int)),
            this, SLOT(addrChanged(int)));

    // addr hex line edit setup
    QRegExp rx("0x[0-9|a-f|A-F]{1,4}");
    QValidator *v = new QRegExpValidator(rx);
    lineAddr->setValidator(v);
    lineAddr->setFixedWidth(50);
    connect(lineAddr, SIGNAL(editingFinished()),
            this, SLOT(hexAddrChanged()));

    // fill data type combo
    comboType->addItem(tr("Variables"), DataVariables);
    comboType->addItem(tr("Tags"), DataTags);
    comboType->addItem(tr("Raw Bytes"), DataRawBytes);    
    connect(comboType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(typeChanged()));

    // size range & default value
    spinSize->setRange(0, 0xffff);
    spinSize->setValue(1);
    connect(spinSize, SIGNAL(valueChanged(int)),
            this, SLOT(countChanged()));

    // variables editor setup
    tableVariables->setSelectionMode(QAbstractItemView::NoSelection);
    tableVariables->verticalHeader()->setVisible(false);
    tableVariables->setColumnCount(2);
    tableVariables->setHorizontalHeaderLabels(QStringList() << tr("addr") << tr("data"));
    tableVariables->setItemDelegateForColumn(1, new Delegate);
    tableVariables->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableVariables, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(editorContextMenu(QPoint)));

    // tags editor setup
    tableTags->hide();
    tableTags->setSelectionMode(QAbstractItemView::NoSelection);
    tableTags->verticalHeader()->setVisible(false);
    tableTags->setColumnCount(4);
    tableTags->setHorizontalHeaderLabels(QStringList() << tr("Tag") << tr("Channel") << tr("Value1") << tr("Value2"));
//    tableTags->setItemDelegateForColumn(1, new Delegate);
//    tableTags->setContextMenuPolicy(Qt::CustomContextMenu);
//    connect(tableTags, SIGNAL(customContextMenuRequested(QPoint)),
//            this, SLOT(editorContextMenu(QPoint)));

    // raw bytes editor
    textRaw->hide();

    // monitor setup
    treeMonitor->setHeaderHidden(true);
    treeMonitor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeMonitor, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(monitorContextMenu(QPoint)));
    connect(treeMonitor, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(monitorItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    // settings group
    QGroupBox *group_settings = new QGroupBox(tr("Settings:"));
    QGridLayout *grid_settings = new QGridLayout;
    grid_settings->addWidget(new QLabel(tr("Port:")), 0, 0);
    grid_settings->addWidget(comboPort, 0, 1, 1, 3);
    grid_settings->addWidget(new QLabel(tr("Speed:")), 1, 0);
    grid_settings->addWidget(comboSpeed, 1, 1);
    grid_settings->addWidget(new QLabel(tr("Timeout, ms:")), 1, 2);
    grid_settings->addWidget(spinTimeout, 1, 3);
    group_settings->setLayout(grid_settings);

    // query group
    QGroupBox *group_query = new QGroupBox(tr("MicontBUS Query:"));
    QGridLayout *grid_query = new QGridLayout;
    grid_query->addWidget(new QLabel(tr("Id:")), 0, 0);
    grid_query->addWidget(spinId, 0, 1);
    grid_query->addWidget(new QLabel(tr("Cmd:")), 1, 0);
    grid_query->addWidget(comboCmd, 1, 1, 1, 2);
    grid_query->addWidget(new QLabel(tr("Addr:")), 2, 0);
    grid_query->addWidget(spinAddr, 2, 1);
    grid_query->addWidget(lineAddr, 2, 2);

    // data widgets block
    dataWidgets << new QLabel(tr("Data:"));
    grid_query->addWidget(dataWidgets.last(), 3, 0);
    dataWidgets << comboType;
    grid_query->addWidget(comboType, 3, 1, 1, 2);
    dataWidgets << new QLabel(tr("Count:"));
    grid_query->addWidget(dataWidgets.last(), 3, 3);
    dataWidgets << spinSize;
    grid_query->addWidget(spinSize, 3, 4);

    grid_query->setColumnStretch(5, 1);
    group_query->setLayout(grid_query);

    // transaction group
    QGroupBox *group_transaction = new QGroupBox(tr("Transaction:"));
    QGridLayout *grid_transaction = new QGridLayout;
    grid_transaction->addWidget(pushQuery, 0, 0);
    grid_transaction->setColumnStretch(1, 1);
    group_transaction->setLayout(grid_transaction);

    // data editor group
    QGroupBox *group_data = new QGroupBox(tr("Data viewer/editor:"));
    QGridLayout *grid_data = new QGridLayout;
    grid_data->addWidget(tableVariables, 0, 0);
    grid_data->addWidget(tableTags, 1, 0);
    grid_data->addWidget(textRaw, 2, 0);
    group_data->setLayout(grid_data);

    labelStatTxBytes = new QLabel;
    labelStatTxPackets = new QLabel;
    labelStatRxBytes = new QLabel;
    labelStatRxPackets = new QLabel;
    labelStatCrcErrors = new QLabel;
    labelStatTimeouts = new QLabel;

    // monitor group
    QGroupBox *group_monitor = new QGroupBox(tr("Monitor:"));
    QGridLayout *grid_monitor = new QGridLayout;
    grid_monitor->addWidget(treeMonitor, 0, 0, 1, 6);

    grid_monitor->addWidget(new QLabel(tr("Tx Bytes:")), 1, 0);
    grid_monitor->addWidget(labelStatTxBytes, 1, 1);
    grid_monitor->setColumnStretch(1, 1);

    grid_monitor->addWidget(new QLabel(tr("Tx Packets:")), 1, 2);
    grid_monitor->addWidget(labelStatTxPackets, 1, 3);
    grid_monitor->setColumnStretch(3, 1);

    grid_monitor->addWidget(new QLabel(tr("CRC Errors:")), 1, 4);
    grid_monitor->addWidget(labelStatCrcErrors, 1, 5);
    grid_monitor->setColumnStretch(5, 1);

    grid_monitor->addWidget(new QLabel(tr("Rx Bytes:")), 2, 0);
    grid_monitor->addWidget(labelStatRxBytes, 2, 1);

    grid_monitor->addWidget(new QLabel(tr("Rx Packets:")), 2, 2);
    grid_monitor->addWidget(labelStatRxPackets, 2, 3);

    grid_monitor->addWidget(new QLabel(tr("Timeouts:")), 2, 4);
    grid_monitor->addWidget(labelStatTimeouts, 2, 5);

    group_monitor->setLayout(grid_monitor);

    // main layout
    QGridLayout *grid_main = new QGridLayout;
    grid_main->addWidget(group_settings, 0, 0);
    grid_main->addWidget(group_query, 1, 0);
    grid_main->addWidget(group_transaction, 2, 0);
    grid_main->addWidget(group_data, 3, 0);
    grid_main->addWidget(group_monitor, 0, 1, 4, 1);
    grid_main->addWidget(labelStatus, 4, 0);
    grid_main->setColumnStretch(1, 1);

    QWidget *w = new QWidget(this);
    w->setLayout(grid_main);
    setCentralWidget(w);
    setWindowTitle(tr("MicontBUS RTU Master"));
    setWindowIcon(QIcon("icons/network.svg"));

    connect(pushQuery, SIGNAL(clicked()),
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
    labelStatus->setText(tr("Opening port %1...").arg(comboPort->currentData().toString()));

    MicontBusPacket packet;
    packet.setId(spinId->value());
    packet.setCmd(comboCmd->currentData().toInt());
    packet.setAddr(spinAddr->value());

    if (comboType->currentData().toInt() == DataVariables) {
        packet.setSize(spinSize->value() * 4);
    } else {
        packet.setSize(spinSize->value());
    }

    if (packet.cmd() == MicontBusPacket::CMD_PUTBUF_B) {
        QVector<tMicontVar> vars(tableVariables->rowCount());

        for (int i = 0; i < tableVariables->rowCount(); i++) {
            bool ok;
            vars[i].u = tableVariables->item(i, 1)->text().toUInt(&ok);
            if (!ok)
                vars[i].i = tableVariables->item(i, 1)->text().toInt(&ok);
            if (!ok)
                vars[i].f = tableVariables->item(i, 1)->text().toFloat(&ok);
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

    master.transaction(comboPort->currentData().toString(),
                       comboSpeed->currentData().toInt(),
                       spinTimeout->value(), packet.serialize());

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
        tableVariables->clearContents();

        if (comboType->currentData().toInt() == DataRawBytes) {
            tableVariables->setRowCount(1);
            tableVariables->setItem(0, 0, new QTableWidgetItem(QString("0x%1").arg(p.addr(), 4, 16, QLatin1Char('0'))));
            tableVariables->setItem(0, 1, new QTableWidgetItem(bufferToString(p.data())));
        } else if (comboType->currentData().toInt() == DataVariables) {
            QVector<tMicontVar> vars = p.variables();
            tableVariables->setRowCount(vars.count());
            for (int i = 0; i < vars.count(); i++) {
                QTableWidgetItem *item = new QTableWidgetItem;
                item->setData(Qt::UserRole, vars[i].u);
                item->setText(QString("%1").arg(vars[i].u));
                tableVariables->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(p.addr() + i, 4, 16, QLatin1Char('0'))));
                tableVariables->setItem(i, 1, item);
            }
        }
    }

    labelStatus->setText(tr("Ready"));
}

void Window::processError(const QString &s)
{
    setControlsEnabled(true);
    labelStatus->setText(tr("Error (%1)").arg(s));
    updateStatistics();
}

void Window::processTimeout(const QString &s)
{
    setControlsEnabled(true);
    labelStatus->setText(tr("Error (%1)").arg(s));
    updateStatistics();
}

void Window::addrChanged(int newAddr)
{
    lineAddr->setText(QString("0x%1").arg(newAddr, 4, 16, QLatin1Char('0')));
    if (comboCmd->currentData().toInt() == MicontBusPacket::CMD_PUTBUF_B) {
        fillDataEditor();
    }
}

void Window::countChanged()
{
    if (comboCmd->currentData().toInt() == MicontBusPacket::CMD_PUTBUF_B) {
        fillDataEditor();
    }
}

void Window::hexAddrChanged()
{
    spinAddr->setValue(lineAddr->text().toInt(0, 0));
}

void Window::cmdChanged()
{
    tableVariables->clearContents();
    tableVariables->setRowCount(0);
    comboType->removeItem(comboType->findData(DataTags));
    comboType->setCurrentIndex(0);

    switch (comboCmd->currentData().toInt()) {
        case MicontBusPacket::CMD_GETSIZE:
            spinSize->setValue(0);
            toggleWidgets(dataWidgets, false);
            break;
        case MicontBusPacket::CMD_GETBUF_B:
        case MicontBusPacket::CMD_PUTBUF_B:
            spinSize->setValue(1);
            toggleWidgets(dataWidgets, true);
            if (comboCmd->currentData().toInt() == MicontBusPacket::CMD_PUTBUF_B) {
                fillDataEditor();
                comboType->insertItem(1, "Tags", DataTags);
            }
            break;
    }
}

void Window::typeChanged()
{
    switch (comboType->currentData().toInt()) {
    case DataRawBytes:
        tableTags->hide();
        tableVariables->hide();
        textRaw->show();
        break;
    case DataVariables:
        textRaw->hide();
        tableTags->hide();
        tableVariables->show();
        break;
    case DataTags:
        tableVariables->hide();
        textRaw->hide();
        tableTags->show();
        break;
    }
}

void Window::fillDataEditor()
{
    tableVariables->setRowCount(spinSize->value());
    for (int i = 0; i < spinSize->value(); i++) {
        tableVariables->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(spinAddr->value() + i, 4, 16, QLatin1Char('0'))));
        if (!tableVariables->item(i, 1))
            tableVariables->setItem(i, 1, new QTableWidgetItem(QString("0")));
    }
}

void Window::monitorItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current) {
        if (current->type() == QTreeWidgetItem::UserType + 1) {
            QTreeWidgetItem *parentItem = current->parent();
            QLabel *l = qobject_cast<QLabel *>(treeMonitor->itemWidget(current->parent(), 0));

            QPoint p = current->data(0, Qt::UserRole).toPoint();
            l->setText(bufferToString(parentItem->data(0, Qt::UserRole).toByteArray(), p.x(), p.y()));
        }
    }

    if (previous) {
        if (previous->type() == QTreeWidgetItem::UserType + 1 && previous->parent() != current->parent()) {
            QLabel *l = qobject_cast<QLabel *>(treeMonitor->itemWidget(previous->parent(), 0));
            l->setText(bufferToString(previous->parent()->data(0, Qt::UserRole).toByteArray()));
        }
    }
}

void Window::monitorContextMenu(const QPoint &)
{
    QMenu *menu = new QMenu;
    menu->addAction(QIcon("icons/trash.svg"), tr("Clear"), this, SLOT(monitorClear()));
    menu->exec(QCursor::pos());
}

void Window::editorContextMenu(const QPoint &p)
{
    QTableWidgetItem *item = tableVariables->itemAt(p);
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
    treeMonitor->clear();

    if (comboCmd->currentData().toInt() != MicontBusPacket::CMD_PUTBUF_B) {
        tableVariables->clearContents();
        tableVariables->setRowCount(0);
    }
}

void Window::itemSwitchViewToUInt()
{
    QTableWidgetItem *item = tableVariables->currentItem();
    if (!item)
        return;
    item->setText(QString("%1").arg(item->data(Qt::UserRole).toUInt()));
}

void Window::itemSwitchViewToInt()
{
    QTableWidgetItem *item = tableVariables->currentItem();
    if (!item)
        return;
    item->setText(QString("%1").arg(item->data(Qt::UserRole).toInt()));
}

void Window::itemSwitchViewToFloat()
{
    QTableWidgetItem *item = tableVariables->currentItem();
    if (!item)
        return;
    tMicontVar var;
    var.u = item->data(Qt::UserRole).toUInt();
    item->setText(QString("%1").arg(var.f));
}

void Window::itemSwitchViewToHex()
{
    QTableWidgetItem *item = tableVariables->currentItem();
    if (!item)
        return;
    item->setText(QString("0x%1").arg(item->data(Qt::UserRole).toUInt(), 8, 16, QLatin1Char('0')));
}

void Window::itemSwitchViewToBit()
{
    QTableWidgetItem *item = tableVariables->currentItem();
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
    pushQuery->setEnabled(enable);
}

QString Window::bufferToString(const QByteArray &data, int start, int length)
{
    QString s;

    for (int i = 0; i < data.size(); i++) {
        if (length > 0 && i == start)
            s.append("<b><font color=black>");
        s.append(data.mid(i, 1).toHex());
        s.append(' ');
        if (length > 0 && i == start + length - 1)
            s.append("</font></b>");
    }

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

    treeMonitor->addTopLevelItem(item);
    treeMonitor->setItemWidget(item, 0, new QLabel(bufferToString(rawPacket)));
    treeMonitor->scrollToBottom();

    updateStatistics();
}

void Window::updateStatistics()
{
    labelStatRxBytes->setText(QString::number(master.statRxBytes()));
    labelStatTxBytes->setText(QString::number(master.statTxBytes()));
    labelStatRxPackets->setText(QString::number(master.statRxPackets()));
    labelStatTxPackets->setText(QString::number(master.statTxPackets()));
    QString color = (master.statCrcErrors() != 0) ? "red" : "black";
    labelStatCrcErrors->setText("<font color=" + color + ">" + QString::number(master.statCrcErrors()) + "</font>");
    labelStatTimeouts->setText(QString::number(master.statTimeouts()));
}
