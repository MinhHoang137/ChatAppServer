#include "authentication.h"
#include <QDebug>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
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
        "insert into Users (Username, PasswordHash, Status) values (:Username, :PasswordHash, 1);");
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
    char sendbuf[4096];
    QJsonDocument doc(response);
    QByteArray byteArray = doc.toJson(QJsonDocument::Compact);
    memcpy(sendbuf, byteArray.constData(), byteArray.size());
    sendbuf[byteArray.size()] = '\0';
    int iSendResult = send(clientSocket, sendbuf, static_cast<int>(byteArray.size()), 0);
    if (iSendResult == SOCKET_ERROR) {
        qDebug() << "send failed with error:" << WSAGetLastError();
    } else {
        qDebug() << "Sent registration response to client: " << response["userId"].toInt();
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
        result.message = loginSuccess ? "Login successful." : "Incorrect username or password.";

        return result;
    } else {
        qDebug() << "Username not found during login.";
        db.close();
        QSqlDatabase::removeDatabase("LoginConnection");
        result.result = false;
        result.message = "Incorrect username or password.";
        return result;
    }
    return result;
}

QJsonObject handleLogin(const QJsonObject &request, SOCKET clientSocket)
{
    QString username = request["username"].toString();
    QString password = request["password"].toString();

    AuthResult result = loginUser(username, password, DB_NAME);
    return {{"success", result.result},
            {"message", QString::fromStdString(result.message)},
            {"userId", result.userId}};
}

bool logoutUser(const int & userID, const std::string &dbName)
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

// QJsonObject handleLogout(const QJsonObject &request, SOCKET clientSocket)
// {
//     QString username = request["username"].toString();
//     bool success = logoutUser(username, DB_NAME);
//     return {{"success", success}, {"message", success ? "Logout successful" : "Logout failed"}};
// }

void initAuthenticationHandlers(
    std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers)
{
    handlers["register"] = handleRegistration;
    handlers["login"] = handleLogin;
    // handlers["logout"] = handleLogout;
}
