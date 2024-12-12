#pragma once

#ifndef DAEMON_MANAGER_HPP_laiin
#define DAEMON_MANAGER_HPP_laiin

#include <mutex>

#include <QApplication>
#include <QProcess>

namespace laiin {

class DaemonManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool daemonRunning READ isDaemonRunning NOTIFY daemonRunningChanged)
    Q_PROPERTY(bool daemonConnected READ isDaemonConnected NOTIFY daemonConnectedChanged)
    Q_PROPERTY(double daemonProgress READ getDaemonProgress NOTIFY daemonProgressChanged)
    Q_PROPERTY(QString daemonStatusText READ getDaemonStatusText NOTIFY daemonStatusTextChanged)
public:
    explicit DaemonManager(QObject *parent = nullptr);
    ~DaemonManager();
    
    void terminateDaemonProcess();
    
    void connect();
    void disconnect();
    
    double getDaemonProgress() const;
    QString getDaemonStatusText() const;

    void setDaemonRunning(bool running);
    void setDaemonConnected(bool connected);

    Q_INVOKABLE bool isDaemonRunning() const;
    static bool isDaemonRunningAlready(); // checks if daemon is already running in background
    static bool isDaemonServerBound();
    Q_INVOKABLE bool isDaemonConnected() const; // when GUI client has connected to daemon server
    static bool isDaemonDHTServerBound();

public slots:
    void startDaemonProcess(/*const QString &daemonPath*/);
    void startDaemonProcessDetached(/*const QString &daemonPath*/);
signals:
    void daemonRunningChanged(bool running);
    void daemonConnectedChanged();
    void daemonProgressChanged();
    void daemonStatusTextChanged();
private:
    QProcess daemonProcess;
    bool m_daemonRunning;
    bool m_daemonConnected;
    static qint64 pid;
    std::mutex clientMutex;
    void onConnectionSuccess();
    void onConnectionFailure();    
};

}
#endif
