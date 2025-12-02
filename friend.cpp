#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include "server.h"
#include "friend.h"
#include "header.h"

QJsonObject sendMessage(const int &senderID, const int &receiverID, const QString &content, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "InsertMessageConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for inserting message:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "insert into Messages (SenderID, ReceiverID, Content) values (:SenderID, :ReceiverID, :Content);");
    query.bindValue(":SenderID", senderID);
    query.bindValue(":ReceiverID", receiverID);
    query.bindValue(":Content", content);

    if (!query.exec()) {
        qDebug() << "Inserting message failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("InsertMessageConnection");
        result["success"] = false;
        result["message"] = "Failed to insert message.";
        return result;
    }

    db.close();
    QSqlDatabase::removeDatabase("InsertMessageConnection");
    result["success"] = true;
    result["message"] = "Message inserted successfully.";
    return result;
}

QJsonObject handleSendMessage(const QJsonObject &request, SOCKET clientSocket)
{
    int senderID = request["senderID"].toInt();
    int receiverID = request["receiverID"].toInt();
    QString content = request["content"].toString();

    QJsonObject response = sendMessage(senderID, receiverID, content, DB_NAME);
    response["action"] = "sendMessage";
    sendJsonResponse(clientSocket, response);
    SOCKET targetSocket = Server::getInstance()->getUserSocket(receiverID);
    if (targetSocket != 0) {
        sendJsonResponse(targetSocket, request); // Forward the message to the receiver
    }
    qDebug() << "Sent insert message response to client.";
    return {};
}

QJsonObject getAllMessages(int userID, int friendID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ReturnMessagesConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for returning messages:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "select SenderID, ReceiverID, Content, SentAt from Messages "
        "where (SenderID = :UserID and ReceiverID = :FriendID) "
        "or (SenderID = :FriendID and ReceiverID = :UserID) "
        "order by SentAt ASC;");
    query.bindValue(":UserID", userID);
    query.bindValue(":FriendID", friendID);

    QJsonArray messagesArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject messageObj;
            messageObj["senderID"] = query.value(0).toInt();
            messageObj["receiverID"] = query.value(1).toInt();
            messageObj["content"] = query.value(2).toString();
            messageObj["sentAt"] = query.value(3).toString();
            messagesArray.append(messageObj);
        }
        result["success"] = true;
        result["messages"] = messagesArray;
    } else {
        qDebug() << "Returning messages failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve messages.";
    }

    db.close();
    QSqlDatabase::removeDatabase("ReturnMessagesConnection");
    return result;
}

QJsonObject handleGetAllMessages(const QJsonObject &request, SOCKET clientSocket)
{
    int userID = request["userID"].toInt();
    int friendID = request["friendID"].toInt();

    QJsonObject response = getAllMessages(userID, friendID, DB_NAME);
    response["action"] = "getAllMessages";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent return all messages response to client.";
    return response;
}

QJsonObject getAllUsers(const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ReturnUsersConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for returning users:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("select UserID, Username, Status from Users;");

    QJsonArray usersArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject userObj;
            userObj["userID"] = query.value(0).toInt();
            userObj["username"] = query.value(1).toString();
            userObj["status"] = query.value(2).toInt();
            usersArray.append(userObj);
        }
        result["success"] = true;
        result["users"] = usersArray;
    } else {
        qDebug() << "Returning users failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve users.";
    }

    db.close();
    QSqlDatabase::removeDatabase("ReturnUsersConnection");
    return result;
}

QJsonObject handleGetAllUsers(const QJsonObject &request, SOCKET clientSocket)
{
    QJsonObject response = getAllUsers(DB_NAME);
    response["action"] = "getAllUsers";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent return all users response to client.";
    return response;
}

QJsonObject getNonFriendUsers(int userID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetNonFriendUsersConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for getting non-friend users:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    // Select users who are NOT the current user AND NOT in the Friendships table with the current user
    query.prepare(
        "SELECT UserID, Username, Status FROM Users "
        "WHERE UserID != :UserID "
        "AND UserID NOT IN ("
        "   SELECT UserID2 FROM Friendships WHERE UserID1 = :UserID "
        "   UNION "
        "   SELECT UserID1 FROM Friendships WHERE UserID2 = :UserID"
        ");");
    query.bindValue(":UserID", userID);

    QJsonArray usersArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject userObj;
            userObj["userID"] = query.value(0).toInt();
            userObj["username"] = query.value(1).toString();
            userObj["status"] = query.value(2).toInt();
            usersArray.append(userObj);
        }
        result["success"] = true;
        result["users"] = usersArray;
    } else {
        qDebug() << "Getting non-friend users failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve non-friend users.";
    }

    db.close();
    QSqlDatabase::removeDatabase("GetNonFriendUsersConnection");
    return result;
}

QJsonObject handleGetNonFriendUsers(const QJsonObject &request, SOCKET clientSocket)
{
    int userID = request["userID"].toInt();

    QJsonObject response = getNonFriendUsers(userID, DB_NAME);
    response["action"] = "getNonFriendUsers";
    int sent = sendJsonResponse(clientSocket, response);
    if (sent != SOCKET_ERROR) {
         qDebug() << "Sent get non-friend users response to client.";
    } 
    return response;
}

void initFriendHandlers(std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers)
{
    handlers["sendMessage"] = handleSendMessage;
    handlers["getAllMessages"] = handleGetAllMessages;
    handlers["getAllUsers"] = handleGetAllUsers;
    handlers["getNonFriendUsers"] = handleGetNonFriendUsers;
}
