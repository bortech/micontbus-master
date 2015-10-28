#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QList>

#include "micontbusmaster.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QComboBox;
class QTreeWidget;
class QTableWidget;
class QTableWidgetItem;
class QTreeWidgetItem;
class QTextEdit;
QT_END_NAMESPACE

class MicontBusPacket;

class Window : public QMainWindow
{
    enum DataType {
        DataVariables,
        DataRawBytes,
        DataTags
    };

    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);

private slots:
    void doTransaction();
    void processResponse(const QByteArray &rawPacket);
    void processError(const QString &s);
    void processTimeout(const QString &s);
    void addrChanged(int newAddr);
    void countChanged();
    void hexAddrChanged();
    void cmdChanged();
    void typeChanged();
    void fillDataEditor();
    void monitorItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void monitorContextMenu(const QPoint &);
    void editorContextMenu(const QPoint &p);
    void monitorClear();
    void itemSwitchViewToUInt();
    void itemSwitchViewToInt();
    void itemSwitchViewToFloat();
    void itemSwitchViewToHex();
    void itemSwitchViewToBit();

private:
    void toggleWidgets(const QList<QWidget *> &widgets, bool show);
    void setControlsEnabled(bool enable);
    QString bufferToString(const QByteArray &data, int start = 0, int length = 0);
    QString cmdToString(quint8 cmd);
    void logPacket(const MicontBusPacket &packet);
    void updateStatistics(void);

private:
    // settings group
    QComboBox *comboPort;
    QComboBox *comboSpeed;
    QSpinBox *spinTimeout;

    // micontbus query group
    QSpinBox *spinId;
    QComboBox *comboCmd;
    QLineEdit *lineAddr;
    QSpinBox *spinAddr;
    QSpinBox *spinSize;
    QComboBox *comboType;
    QPushButton *pushQuery;
    QList<QWidget *> dataWidgets;

    // Variables editor
    QTableWidget *tableVariables;
    // Tags editor
    QTableWidget *tableTags;
    // Raw bytes editor
    QTextEdit *textRaw;

    // monitor group
    QTreeWidget *treeMonitor;

    // status label
    QLabel *labelStatus;

    // statistics
    QLabel *labelStatRxBytes;
    QLabel *labelStatTxBytes;
    QLabel *labelStatRxPackets;
    QLabel *labelStatTxPackets;
    QLabel *labelStatCrcErrors;
    QLabel *labelStatTimeouts;

    MicontBusMaster master;
};

#endif // WINDOW_H
