#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include "devicehandler.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanClicked();
    void onDeviceSelected(QListWidgetItem *item);
    void onDataReceived(const QByteArray &data);
    void appendStatus(const QString &msg);
    void onBluetoothPermissionGranted();

    // 服务和特征树点击
    void onServiceTreeItemClicked(QTreeWidgetItem *item, int column);

private:
    void requestAndroidPermissions();
    void buildServiceTree();

    DeviceHandler *m_deviceHandler;
    QListWidget  *m_deviceList;
    QPushButton  *m_scanButton;
    QTextEdit    *m_dataDisplay;
    QTreeWidget  *m_serviceTree;

    QList<QBluetoothDeviceInfo> m_discoveredDevices;
};

#endif // MAINWINDOW_H