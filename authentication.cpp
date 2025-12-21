#include "authentication.h"
#include <QDebug>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include "header.h"
#include "server.h"

AuthResult registerUser(const QString &username, const QString &password, const std::string &dbName)
{
    AuthResult result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "RegisterConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for registration:" << db.lastError().text();
        result.result = false;
        result.message = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "insert into Users (Username, PasswordHash, Status) values (:Username, :PasswordHash, 0);");
    query.bindValue(":Username", username);
    query.bindValue(":PasswordHash", password); // In production, hash the password!

    if (!query.exec()) {
        qDebug() << "Registration failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("RegisterConnection");
        result.result = false;
        result.message = "Registration failed. Username may already exist.";
        return result;
    }

    int newUserId = query.lastInsertId().toInt();

    db.close();
    QSqlDatabase::removeDatabase("RegisterConnection");
    result.result = true;
    result.message = "Registration successful.";
    result.userId = newUserId;
    return result;
}

QJsonObject handleRegistration(const QJsonObject &request, SOCKET clientSocket)
{
    QString username = request["username"].toString();
    QString password = request["password"].toString();
    QJsonObject response;
    AuthResult result = registerUser(username, password, DB_NAME);
    response = {{"action", "registerResponse"},
                {"success", result.result},
                {"message", QString::fromStdString(result.message)},
                {"userId", result.userId}};
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent registration response to client: " << response["userId"].toInt();
    if (result.result) {
        // If registration successful, add user to server's userSockets map
        Server::getInstance()->addUserToMap(result.userId, clientSocket);
    }
    return response;
}

AuthResult loginUser(const QString &username, const QString &password, const std::string &dbName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "LoginConnection");
    db.setDatabaseName(QString::fromStdString(dbName));
    AuthResult result;
    if (!db.open()) {
        qDebug() << "Failed to open database for login:" << db.lastError().text();
        result.result = false;
        result.message = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("select UserID, PasswordHash from Users where Username = :Username;");
    query.bindValue(":Username", username);

    if (!query.exec()) {
        qDebug() << "Login query failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("LoginConnection");
        result.result = false;
        result.message = "Login failed due to query error.";
        return result;
    }

    if (query.next()) {
        int userId = query.value(0).toInt();
        QString storedPasswordHash = query.value(1).toString();
        bool loginSuccess = storedPasswordHash == password;
        if (loginSuccess) {
            // udate user status to online
            QSqlQuery updateQuery(db);
            updateQuery.prepare("update Users set Status = 1 where Username = :Username;");
            updateQuery.bindValue(":Username", username);
            if (!updateQuery.exec()) {
                qDebug() << "Failed to update user status:" << updateQuery.lastError().text();
            }
            qDebug() << "User logged in successfully:" << username;
            result.userId = userId;

        } else {
            qDebug() << "Incorrect password for user:" << username;
        }
        db.close();
        QSqlDatabase::removeDatabase("LoginConnection");
        result.result = loginSuccess;
        result.message = loginSuccess ? "Đăng nhập thành công."
                                      : "Tên đăng nhập hoặc mật khẩu không đúng.";
        return result;
    } else {
        qDebug() << "Username not found during login.";
        db.close();
        QSqlDatabase::removeDatabase("LoginConnection");
        result.result = false;
        result.message = "Tên đăng nhập có thể không tồn tại.";
        return result;
    }
    return result;
}

QJsonObject handleLogin(const QJsonObject &request, SOCKET clientSocket)
{
    QString username = request["username"].toString();
    QString password = request["password"].toString();

    AuthResult result = loginUser(username, password, DB_NAME);
    QJsonObject response = {{"action", "loginResponse"},
                            {"success", result.result},
                            {"message", QString::fromStdString(result.message)},
                            {"userId", result.userId}};

    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent login response to client: " << response["userId"].toInt();
    if (result.result) {
        // If login successful, add user to server's userSockets map
        Server::getInstance()->addUserToMap(result.userId, clientSocket);
    }
    return response;
}

bool logoutUser(const int &userID, const std::string &dbName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "LogoutConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for logout:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare("update Users set Status = 0 where UserID = :UserID;");
    query.bindValue(":UserID", userID);

    if (!query.exec()) {
        qDebug() << "Logout failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("LogoutConnection");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("LogoutConnection");
    qDebug() << "User logged out successfully:" << userID;
    return true;
}

QJsonObject handleLogout(const QJsonObject &request, SOCKET clientSocket)
{
    int userId = request["userId"].toInt();
    bool success = logoutUser(userId, DB_NAME);
    QJsonObject response = {{"action", "logoutResponse"},
                            {"success", success},
                            {"message", success ? "Logout successful" : "Logout failed"}};
    sendJsonResponse(clientSocket, response);
    return response;
}

void initAuthenticationHandlers(
    std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers)
{
    handlers["register"] = handleRegistration;
    handlers["login"] = handleLogin;
    handlers["logout"] = handleLogout;
}
