#include "server.h"
#include "authentication.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>

Server* Server::m_instance = nullptr;

Server::Server(QObject *parent) : QObject(parent), serverSocket(INVALID_SOCKET), m_serverPort(8080) {
    if (m_instance) {
        qFatal("Server instance already exists! Only one instance is allowed.");
    }
    m_instance = this;
    qDebug() << "Current Working Directory:" << QDir::currentPath();
    
    // Find local IP address
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
             m_serverIp = address.toString();
             break;
        }
    }
    if (m_serverIp.isEmpty()) {
        m_serverIp = "127.0.0.1";
    }

    // Initialize handlers
    initAuthenticationHandlers(handlers);

    // Initialize Database
    initDatabase();
}

Server::~Server() {
    if (m_instance == this) {
        m_instance = nullptr;
    }
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }
    WSACleanup();
}

Server* Server::getInstance() {
    return m_instance;
}

QString Server::serverIp() const {
    return m_serverIp;
}

int Server::serverPort() const {
    return m_serverPort;
}

void Server::startServer() {
    // Chạy server trên một luồng riêng biệt để không chặn giao diện người dùng (nếu có)
    std::thread(&Server::runServer, this).detach();
}

void Server::initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "InitConnection");
    db.setDatabaseName(QString::fromLatin1(DB_NAME));

    if (!db.open()) {
        qFatal("Failed to open database: %s", qPrintable(db.lastError().text()));
    }

    QStringList tables;
    tables << "create table if not exists Users ("
              "UserID INTEGER PRIMARY KEY AUTOINCREMENT,"
              "Username TEXT not null unique,"
              "PasswordHash TEXT not null,"
              "Status INTEGER default 0,"
              "CreatedAt DATETIME default CURRENT_TIMESTAMP"
              ");"
           << "create table if not exists Messages ("
              "MessageID INTEGER PRIMARY KEY AUTOINCREMENT,"
              "SenderID INTEGER not null,"
              "ReceiverID INTEGER not null,"
              "Content TEXT not null,"
              "SentAt DATETIME default CURRENT_TIMESTAMP,"
              "foreign key (SenderID) references Users(UserID),"
              "foreign key (ReceiverID) references Users(UserID)"
              ");"
           << "create table if not exists Friendships ("
              "UserID1 INTEGER not null,"
              "UserID2 INTEGER not null,"
              "Status INTEGER not null,"
              "CreatedAt DATETIME default CURRENT_TIMESTAMP,"
              "primary key (UserID1, UserID2),"
              "foreign key (UserID1) references Users(UserID),"
              "foreign key (UserID2) references Users(UserID)"
              ");"
           << "create table if not exists Groups ("
              "GroupID INTEGER PRIMARY KEY AUTOINCREMENT,"
              "GroupName TEXT not null,"
              "CreatedAt DATETIME default CURRENT_TIMESTAMP"
              ");"
           << "create table if not exists GroupMembers ("
              "GroupID INTEGER not null,"
              "UserID INTEGER not null,"
              "JoinedAt DATETIME default CURRENT_TIMESTAMP,"
              "primary key (GroupID, UserID),"
              "foreign key (GroupID) references Groups(GroupID),"
              "foreign key (UserID) references Users(UserID)"
              ");"
           << "create table if not exists GroupMessages ("
              "GroupMessageID INTEGER PRIMARY KEY AUTOINCREMENT,"
              "GroupID INTEGER not null,"
              "SenderID INTEGER not null,"
              "Content TEXT not null,"
              "SentAt DATETIME default CURRENT_TIMESTAMP,"
              "foreign key (GroupID) references Groups(GroupID),"
              "foreign key (SenderID) references Users(UserID)"
              ");";

    QSqlQuery query(db);
    for (const QString &table : tables) {
        if (!query.exec(table)) {
            qDebug() << "Failed to create table:" << query.lastError().text();
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("InitConnection");
    qDebug() << "Database initialized successfully.";
}

void Server::runServer() {
    WSADATA wsaData;
    int iResult;

    // Khởi tạo Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        qDebug() << "WSAStartup failed with error: " << iResult;
        return;
    }

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream
    hints.ai_protocol = IPPROTO_TCP; // TCP protocol
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, "8080", &hints, &result);
    if ( iResult != 0 ) {
        qDebug() << "getaddrinfo failed with error: " << iResult;
        WSACleanup();
        return;
    }

    // 1. Giai đoạn tạo socket (Create a SOCKET for connecting to server)
    serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (serverSocket == INVALID_SOCKET) {
        qDebug() << "socket failed with error: " << WSAGetLastError();
        freeaddrinfo(result);
        WSACleanup();
        return;
    }

    // 2. Giai đoạn bind (Setup the TCP listening socket)
    iResult = bind( serverSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        qDebug() << "bind failed with error: " << WSAGetLastError();
        freeaddrinfo(result);
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(result);

    // 3. Giai đoạn listen
    iResult = listen(serverSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        qDebug() << "listen failed with error: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    qDebug() << "Server listening on port 8080...";

    SOCKET clientSocket = INVALID_SOCKET;

    // 4. Giai đoạn accept (Accept a client socket)
    // Vòng lặp để chấp nhận nhiều kết nối (tuần tự trong ví dụ đơn giản này)
    while(true) {
        clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            int error = WSAGetLastError();
            if (error == WSAEINTR || error == WSAENOTSOCK) {
                qDebug() << "Server stopped listening.";
                return;
            }
            qDebug() << "accept failed with error: " << error;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }
        clientSocketsMutex.lock();
        clientSockets.push_back(clientSocket);
        clientSocketsMutex.unlock();
        qDebug() << "Client connected!";

        // Handle client in a separate thread
        std::thread(&Server::handleClient, this, clientSocket).detach();
    }
}

void Server::handleClient(SOCKET clientSocket) {
    int iResult;
    char recvbuf[4096]; // Buffer lớn hơn để chứa JSON
    int recvbuflen = 4096;

    // Nhận dữ liệu từ client
    do {
        iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            qDebug() << "Bytes received: " << iResult;

            // Chuyển dữ liệu nhận được thành chuỗi (giả sử là JSON string)
            std::string receivedData(recvbuf, iResult);

            // ---------------------------------------------------------
            // [CHO TRONG DE XU LY THONG TIN DUOC GUI DEN]
            // Tại đây bạn có thể parse chuỗi JSON 'receivedData'
            // Ví dụ: auto jsonDoc = QJsonDocument::fromJson(QByteArray::fromStdString(receivedData));

            clientSocketsMutex.lock();
            // Gửi lại dữ liệu cho tất cả các client đã kết nối (broadcast)
            for (SOCKET sock : clientSockets) {
                if (sock != clientSocket) { // Không gửi lại cho chính client gửi
                    int sendResult = send(sock, recvbuf, iResult, 0);
                    if (sendResult == SOCKET_ERROR) {
                        qDebug() << "send failed with error: " << WSAGetLastError();
                    }
                }
            }
            clientSocketsMutex.unlock();
            

            // ---------------------------------------------------------

        }
        else if (iResult == 0)
            qDebug() << "Connection closing...";
        else  {
            qDebug() << "recv failed with error: " << WSAGetLastError();
            closesocket(clientSocket);
            return;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        qDebug() << "shutdown failed with error: " << WSAGetLastError();
        closesocket(clientSocket);
        return;
    }

    closesocket(clientSocket);
}
