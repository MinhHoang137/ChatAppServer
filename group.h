#ifndef GROUP_H
#define GROUP_H

#include <QJsonObject>
#include <QString>
#include <functional>
#include <map>
#include <winsock2.h>

QJsonObject handleCreateGroup(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleAddUserToGroup(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleRemoveUserFromGroup(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleLeaveGroup(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleSendGroupMessage(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleGetGroupMessages(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleGetUserGroups(const QJsonObject &request, SOCKET clientSocket);
QJsonObject handleGetGroupMembers(const QJsonObject &request, SOCKET clientSocket);

void initGroupHandlers(
    std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers);

#endif // GROUP_H