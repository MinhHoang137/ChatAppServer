#ifndef HEADER_H
#define HEADER_H

#include <QJsonObject>
#include <string>
#include <winsock2.h>

int sendJsonResponse(SOCKET clientSocket, const QJsonObject &response);
void initLog();
void logMessage(const std::string &message);

#endif // HEADER_H
