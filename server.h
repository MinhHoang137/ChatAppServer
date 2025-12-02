#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QQmlEngine>
#include "authentication.h"
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <QJsonObject>
#include <QJsonDocument>

#define DB_NAME "ChatApp.db"

#pragma comment(lib, "Ws2_32.lib")

// Helper function to send JSON response


class Server : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString serverIp READ serverIp NOTIFY serverIpChanged)
    Q_PROPERTY(int serverPort READ serverPort NOTIFY serverPortChanged)

public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

    Q_INVOKABLE void startServer();

    static Server *getInstance();

    QString serverIp() const;
    int serverPort() const;

    void addUserToMap(int userId, SOCKET clientSock);
    SOCKET getUserSocket(int userId);
signals:
    void serverIpChanged();
    void serverPortChanged();

private:
    void runServer();
    void handleClient(SOCKET clientSocket);
    void initDatabase();

    SOCKET serverSocket;
    QString m_serverIp;
    int m_serverPort;
    std::vector<SOCKET> clientSockets;
    std::mutex clientSocketsMutex;
    static Server *m_instance;
    std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> handlers;
    std::map<int, SOCKET> userSockets;
    std::mutex userSocketsMutex;
};

#endif // SERVER_H
