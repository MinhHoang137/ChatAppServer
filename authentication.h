#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <string>
#include <QString>
#include <QJsonObject>
#include <map>
#include <functional>

#include <winsock2.h>

typedef struct{
    bool result;
    std::string message;
    int userId = -1;
} AuthResult;

AuthResult registerUser(const QString &username, const QString &password, const std::string &dbName);
QJsonObject handleRegistration(const QJsonObject &request, SOCKET clientSocket);

AuthResult loginUser(const QString &username, const QString &password, const std::string &dbName);
QJsonObject handleLogin(const QJsonObject &request, SOCKET clientSocket);

bool logoutUser(const QString &username, const std::string &dbName);
QJsonObject handleLogout(const QJsonObject &request, SOCKET clientSocket);
void initAuthenticationHandlers(std::map<QString, std::function<QJsonObject(const QJsonObject&, SOCKET)>> &handlers);

#endif // AUTHENTICATION_H
