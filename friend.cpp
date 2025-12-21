#include "friend.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include "header.h"
#include "server.h"

QJsonObject sendMessage(const int &senderID,
                        const int &receiverID,
                        const QString &content,
                        const std::string &dbName)
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
    query.prepare("insert into Messages (SenderID, ReceiverID, Content) values (:SenderID, "
                  ":ReceiverID, :Content);");
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
        QJsonObject forwardMessage = request;
        forwardMessage["action"] = "receiveMessage";
        sendJsonResponse(targetSocket, forwardMessage); // Forward the message to the receiver
    }
    qDebug() << "Sent insert message response to client.";
    return {};
}

// Lấy toàn bộ tin nhắn giữa hai người dùng
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
    query.prepare("select SenderID, ReceiverID, Content, SentAt from Messages "
                  "where (SenderID = :UserID and ReceiverID = :FriendID) "
                  "or (SenderID = :FriendID and ReceiverID = :UserID) "
                  "order by SentAt ASC LIMIT 20;");
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
        qDebug() << "Failed to open database for getting non-friend users:"
                 << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    // Select users who are NOT the current user AND NOT in the Friendships table with Status = 1 (Friends)
    // This includes Strangers and Pending Requests (Incoming/Outgoing)
    query.prepare("SELECT u.UserID, u.Username, u.Status "
                  "FROM Users u "
                  "WHERE u.UserID != :UserID "
                  "AND NOT EXISTS ("
                  "   SELECT 1 FROM Friendships f "
                  "   WHERE ((f.UserID1 = u.UserID AND f.UserID2 = :UserID) "
                  "      OR (f.UserID1 = :UserID AND f.UserID2 = u.UserID)) "
                  "   AND f.Status = 1"
                  ");");
    query.bindValue(":UserID", userID);

    QJsonArray usersArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject userObj;
            userObj["userID"] = query.value(0).toInt();
            userObj["username"] = query.value(1).toString();
            userObj["status"] = query.value(2).toInt();
            // userObj["sentRequest"] is no longer needed as we filter them out
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

QJsonObject getFriendRequests(int userID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetFriendRequestsConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for getting friend requests:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    // Select users who sent me a friend request (Incoming Pending)
    query.prepare("SELECT u.UserID, u.Username, u.Status "
                  "FROM Users u "
                  "JOIN Friendships f ON u.UserID = f.UserID1 "
                  "WHERE f.UserID2 = :UserID AND f.Status = 0;");
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
        result["requests"] = usersArray;
    } else {
        qDebug() << "Getting friend requests failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve friend requests.";
    }

    db.close();
    QSqlDatabase::removeDatabase("GetFriendRequestsConnection");
    return result;
}

QJsonObject handleGetFriendRequests(const QJsonObject &request, SOCKET clientSocket)
{
    int userID = request["userID"].toInt();

    QJsonObject response = getFriendRequests(userID, DB_NAME);
    response["action"] = "getFriendRequests";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent get friend requests response to client.";
    return response;
}

QJsonObject friendRequest(const int &fromUserID, const int &toUserID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "FriendRequestConnection");
    db.setDatabaseName(QString::fromStdString(dbName));
    if (!db.open()) {
        qDebug() << "Failed to open database for friend request:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "insert into Friendships (UserID1, UserID2, Status) values (:UserID1, :UserID2, 0);");
    query.bindValue(":UserID1", fromUserID);
    query.bindValue(":UserID2", toUserID);

    if (query.exec()) {
        result["success"] = true;
        result["message"] = "Friend request sent successfully.";
    } else {
        qDebug() << "Friend request failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to send friend request.";
    }

    db.close();
    QSqlDatabase::removeDatabase("FriendRequestConnection");
    return result;
}

QJsonObject handleFriendRequest(const QJsonObject &request, SOCKET clientSocket)
{
    int fromUserID = request["fromUserID"].toInt();
    int toUserID = request["toUserID"].toInt();

    QJsonObject response = friendRequest(fromUserID, toUserID, DB_NAME);
    response["action"] = "friendRequest";
    // sendJsonResponse(clientSocket, response);
    // người dùng tải lại danh sách bạn bè là thấy kết quả
    qDebug() << "Sent friend request response to client.";
    return response;
}

QJsonObject acceptFriendRequest(const int &fromUserID,
                                const int &toUserID,
                                const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "AcceptFriendRequestConnection");
    db.setDatabaseName(QString::fromStdString(dbName));
    if (!db.open()) {
        qDebug() << "Failed to open database for accepting friend request:"
                 << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("update Friendships set Status = 1 "
                  "where (UserID1 = :UserID1 and UserID2 = :UserID2) "
                  "or (UserID1 = :UserID2 and UserID2 = :UserID1);");
    query.bindValue(":UserID1", fromUserID);
    query.bindValue(":UserID2", toUserID);

    if (query.exec()) {
        result["success"] = true;
        result["message"] = "Friend request accepted successfully.";
    } else {
        qDebug() << "Accepting friend request failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to accept friend request.";
    }

    db.close();
    QSqlDatabase::removeDatabase("AcceptFriendRequestConnection");
    return result;
}

QJsonObject queryFriendStatus(const int &fromUserID, const int &toUserID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "QueryFriendStatusConnection");
    db.setDatabaseName(QString::fromStdString(dbName));
    if (!db.open()) {
        qDebug() << "Failed to open database for querying friend status:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("select Status, UserID1 from Friendships "
                  "where (UserID1 = :UserID1 and UserID2 = :UserID2) "
                  "or (UserID1 = :UserID2 and UserID2 = :UserID1);");
    query.bindValue(":UserID1", fromUserID);
    query.bindValue(":UserID2", toUserID);

    if (query.exec() && query.next()) {
        result["success"] = true;
        int status = query.value(0).toInt();
        int sender = query.value(1).toInt();

        if (status == 0) {
            // If pending, check who sent it
            if (sender == fromUserID) {
                result["status"] = 0; // I sent it
            } else {
                result["status"] = 2; // They sent it (Incoming request)
            }
        } else {
            result["status"] = 1; // Friends
        }
    } else {
        result["success"] = true;
        result["status"] = -1; // Not friends
    }

    db.close();
    QSqlDatabase::removeDatabase("QueryFriendStatusConnection");
    return result;
}

QJsonObject handleQueryFriendStatus(const QJsonObject &request, SOCKET clientSocket)
{
    int fromUserID = request["fromUserID"].toInt();
    int toUserID = request["toUserID"].toInt();

    QJsonObject response = queryFriendStatus(fromUserID, toUserID, DB_NAME);
    response["action"] = "queryFriendStatus";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent query friend status response to client.";
    return response;
}

QJsonObject handleAcceptFriendRequest(const QJsonObject &request, SOCKET clientSocket)
{
    int fromUserID = request["fromUserID"].toInt();
    int toUserID = request["toUserID"].toInt();

    QJsonObject response = acceptFriendRequest(fromUserID, toUserID, DB_NAME);
    response["action"] = "acceptFriendRequest";
    // sendJsonResponse(clientSocket, response);
    // khi người dùng chấp nhận thì render mới luôn
    qDebug() << "Sent accept friend request response to client.";
    return response;
}

QJsonObject getFriendsList(const int &userID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetFriendsListConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for getting friends list:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("SELECT u.UserID, u.Username, u.Status "
                  "FROM Users u "
                  "JOIN Friendships f ON (u.UserID = f.UserID1 OR u.UserID = f.UserID2) "
                  "WHERE (f.UserID1 = :UserID OR f.UserID2 = :UserID) AND f.Status = 1 AND "
                  "u.UserID != :UserID;");
    query.bindValue(":UserID", userID);

    QJsonArray friendsArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject friendObj;
            friendObj["userID"] = query.value(0).toInt();
            friendObj["username"] = query.value(1).toString();
            friendObj["status"] = query.value(2).toInt();
            friendsArray.append(friendObj);
        }
        result["success"] = true;
        result["friends"] = friendsArray;
    } else {
        qDebug() << "Getting friends list failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve friends list.";
    }

    db.close();
    QSqlDatabase::removeDatabase("GetFriendsListConnection");
    return result;
}

QJsonObject handleGetFriendsList(const QJsonObject &request, SOCKET clientSocket)
{
    int userID = request["userID"].toInt();

    QJsonObject response = getFriendsList(userID, DB_NAME);
    response["action"] = "getFriendsList";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent get friends list response to client.";
    return response;
}

QJsonObject unfriend(const int &userID1, const int &userID2, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "UnfriendConnection");
    db.setDatabaseName(QString::fromStdString(dbName));
    if (!db.open()) {
        qDebug() << "Failed to open database for unfriending:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("delete from Friendships "
                  "where (UserID1 = :UserID1 and UserID2 = :UserID2) "
                  "or (UserID1 = :UserID2 and UserID2 = :UserID1);");
    query.bindValue(":UserID1", userID1);
    query.bindValue(":UserID2", userID2);

    if (query.exec()) {
        result["success"] = true;
        result["message"] = "Unfriended successfully.";
    } else {
        qDebug() << "Unfriending failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to unfriend.";
    }

    db.close();
    QSqlDatabase::removeDatabase("UnfriendConnection");
    return result;
}

QJsonObject handleUnfriend(const QJsonObject &request, SOCKET clientSocket)
{
    int userID1 = request["fromUserID"].toInt();
    int userID2 = request["toUserID"].toInt();

    QJsonObject response = unfriend(userID1, userID2, DB_NAME);
    response["action"] = "unfriend";
    // người dùng tải lại danh sách bạn bè là thấy kết quả
    qDebug() << "Sent unfriend response to client.";
    return response;
}

void initFriendHandlers(
    std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers)
{
    handlers["sendMessage"] = handleSendMessage;
    handlers["getAllMessages"] = handleGetAllMessages;
    handlers["getAllUsers"] = handleGetAllUsers;
    handlers["getNonFriendUsers"] = handleGetNonFriendUsers;
    handlers["getFriendRequests"] = handleGetFriendRequests;
    handlers["friendRequest"] = handleFriendRequest;
    handlers["acceptFriendRequest"] = handleAcceptFriendRequest;
    handlers["queryFriendStatus"] = handleQueryFriendStatus;
    handlers["getFriendsList"] = handleGetFriendsList;
    handlers["unfriend"] = handleUnfriend;
}
