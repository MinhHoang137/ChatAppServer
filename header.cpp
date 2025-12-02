#include "header.h"
#include <QJsonDocument>
#include <QByteArray>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <direct.h> // For _mkdir

static std::ofstream logFile;

void initLog() {
    _mkdir("logs");

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << "logs/server_log_" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".txt";
    std::string filename = oss.str();

    logFile.open(filename, std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        logMessage("Server started.");
    } else {
        std::cerr << "Failed to create log file: " << filename << std::endl;
    }
}

void logMessage(const std::string &message) {
    if (logFile.is_open()) {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        logFile << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << message << "\n";
        logFile.flush();
    }
}

int sendJsonResponse(SOCKET clientSocket, const QJsonObject &response) {
    char sendbuf[4096];
    QJsonDocument doc(response);
    QByteArray byteArray = doc.toJson(QJsonDocument::Compact);
    
    // Ensure we don't overflow the buffer
    if (byteArray.size() >= 4096) {
        std::cerr << "Error: JSON response too large for buffer" << std::endl;
        return SOCKET_ERROR;
    }

    memcpy(sendbuf, byteArray.constData(), byteArray.size());
    sendbuf[byteArray.size()] = '\0'; // Null-terminate

    int iSendResult = send(clientSocket, sendbuf, byteArray.size(), 0);
    if (iSendResult == SOCKET_ERROR) {
        std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
    }
    return iSendResult;
}
