#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "micontbusmaster.h"

QT_BEGIN_NAMESPACE

class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QComboBox;
class QTreeWidget;
class QTableWidget;

QT_END_NAMESPACE

class MicontBusPacket;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);

private slots:
    void doTransaction();
    void processResponse(const QByteArray &rawPacket);
    void processError(const QString &s);
    void processTimeout(const QString &s);
    void addrChanged(int newAddr);
    void hexAddrChanged();
    void cmdChanged();

private:
    void setControlsEnabled(bool enable);
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
    QPushButton *push_query;

    // query data editor & result display
    QTableWidget *table_editor;

    // monitor group
    QTreeWidget *tree_monitor;

    // status label
    QLabel *label_status;

    MicontBusMaster master;
};

#endif // DIALOG_H
