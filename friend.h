#ifndef FRIEND_H
#define FRIEND_H

#include <map>
#include <functional>
#include <QString>
#include <QJsonObject>
#include <winsock2.h>

QJsonObject handleGetFriendRequests(const QJsonObject &request, SOCKET clientSocket);

void initFriendHandlers(std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers);

#endif // FRIEND_H
