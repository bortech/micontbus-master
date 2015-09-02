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
class QTreeWidgetItem;
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
    void hexAddrChanged();
    void cmdChanged();
    void monitorItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void monitorContextMenu(const QPoint &);
    void monitorClear();

private:
    void toggleWidgets(const QList<QWidget *> &widgets, bool show);
    void setControlsEnabled(bool enable);
    QString bufferToString(const QByteArray &data, int start = 0, int length = 0);
    QString cmdToString(quint8 cmd);
    void logPacket(const MicontBusPacket &packet);

private:
    // settings group
    QComboBox *combo_port;
    QComboBox *combo_speed;
    QSpinBox *spin_timeout;

    // micontbus query group
    QSpinBox *spin_id;
    QComboBox *combo_cmd;
    QLineEdit *line_addr;
    QSpinBox *spin_addr;
    QSpinBox *spin_size;
    QComboBox *combo_type;
    QPushButton *push_query;
    QList<QWidget *> dataWidgets;

    // query data editor & result display
    QTableWidget *table_editor;

    // monitor group
    QTreeWidget *tree_monitor;

    // status label
    QLabel *label_status;

    MicontBusMaster master;
};

#endif // WINDOW_H
