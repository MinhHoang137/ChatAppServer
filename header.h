#ifndef HEADER_H
#define HEADER_H

#include <winsock2.h>
#include <QJsonObject>
#include <string>

int sendJsonResponse(SOCKET clientSocket, const QJsonObject &response);
void initLog();
void logMessage(const std::string &message);

#endif // HEADER_H
