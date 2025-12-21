#include "group.h"
#include <QDebug>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "header.h"
#include "server.h"
#include "friend.h"
#include "authentication.h"
#include "server.h"

QJsonObject createGroup(const QString &groupName, int ownerID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "CreateGroupConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for creating group:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    // Create group
    QSqlQuery query(db);
    query.prepare("insert into Groups (GroupName) values (:GroupName);");
    query.bindValue(":GroupName", groupName);

    if (!query.exec()) {
        qDebug() << "Creating group failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("CreateGroupConnection");
        result["success"] = false;
        result["message"] = "Failed to create group.";
        return result;
    }

    int groupID = query.lastInsertId().toInt();

    // Add owner to group
    QSqlQuery addMember(db);
    addMember.prepare("insert into GroupMembers (GroupID, UserID) values (:GroupID, :UserID);");
    addMember.bindValue(":GroupID", groupID);
    addMember.bindValue(":UserID", ownerID);
    if (!addMember.exec()) {
        qDebug() << "Adding owner to group failed:" << addMember.lastError().text();
    }

    db.close();
    QSqlDatabase::removeDatabase("CreateGroupConnection");
    result["success"] = true;
    result["groupID"] = groupID;
    result["groupName"] = groupName;
    return result;
}

QJsonObject handleCreateGroup(const QJsonObject &request, SOCKET clientSocket)
{
    QString groupName = request["groupName"].toString();
    int ownerID = request["ownerID"].toInt();
    QJsonObject response = createGroup(groupName, ownerID, DB_NAME);
    response["action"] = "createGroup";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent create group response to client.";
    return response;
}

QJsonObject addUserToGroup(int groupID, int userID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "AddUserToGroupConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for adding user to group:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("insert into GroupMembers (GroupID, UserID) values (:GroupID, :UserID);");
    query.bindValue(":GroupID", groupID);
    query.bindValue(":UserID", userID);

    if (query.exec()) {
        result["success"] = true;
    } else {
        qDebug() << "Adding user to group failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to add user to group.";
    }

    db.close();
    QSqlDatabase::removeDatabase("AddUserToGroupConnection");
    return result;
}

QJsonObject handleAddUserToGroup(const QJsonObject &request, SOCKET clientSocket)
{
    int groupID = request["groupID"].toInt();
    int userID = request["userID"].toInt();
    QJsonObject response = addUserToGroup(groupID, userID, DB_NAME);
    response["action"] = "addUserToGroup";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent add user to group response to client.";
    return response;
}

QJsonObject removeUserFromGroup(int groupID, int userID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "RemoveUserFromGroupConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for removing user from group:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("delete from GroupMembers where GroupID = :GroupID and UserID = :UserID;");
    query.bindValue(":GroupID", groupID);
    query.bindValue(":UserID", userID);

    if (query.exec()) {
        result["success"] = true;
    } else {
        qDebug() << "Removing user from group failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to remove user from group.";
    }

    db.close();
    QSqlDatabase::removeDatabase("RemoveUserFromGroupConnection");
    return result;
}

QJsonObject handleRemoveUserFromGroup(const QJsonObject &request, SOCKET clientSocket)
{
    int groupID = request["groupID"].toInt();
    int userID = request["userID"].toInt();
    QJsonObject response = removeUserFromGroup(groupID, userID, DB_NAME);
    response["action"] = "removeUserFromGroup";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent remove user from group response to client.";
    return response;
}

QJsonObject leaveGroup(int groupID, int userID, const std::string &dbName)
{
    // Same as removing self from group
    return removeUserFromGroup(groupID, userID, dbName);
}

QJsonObject handleLeaveGroup(const QJsonObject &request, SOCKET clientSocket)
{
    int groupID = request["groupID"].toInt();
    int userID = request["userID"].toInt();
    QJsonObject response = leaveGroup(groupID, userID, DB_NAME);
    response["action"] = "leaveGroup";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent leave group response to client.";
    return response;
}

QJsonObject sendGroupMessage(int groupID, int senderID, const QString &content, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "SendGroupMessageConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for sending group message:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("insert into GroupMessages (GroupID, SenderID, Content) values (:GroupID, :SenderID, :Content);");
    query.bindValue(":GroupID", groupID);
    query.bindValue(":SenderID", senderID);
    query.bindValue(":Content", content);

    if (!query.exec()) {
        qDebug() << "Sending group message failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("SendGroupMessageConnection");
        result["success"] = false;
        result["message"] = "Failed to send group message.";
        return result;
    }

    db.close();
    QSqlDatabase::removeDatabase("SendGroupMessageConnection");
    result["success"] = true;
    return result;
}

QJsonObject handleSendGroupMessage(const QJsonObject &request, SOCKET clientSocket)
{
    int groupID = request["groupID"].toInt();
    int senderID = request["senderID"].toInt();
    QString content = request["content"].toString();
    
    qDebug() << "[GroupMessage] Received request - groupID:" << groupID << "senderID:" << senderID << "content:" << content;

    QJsonObject response = sendGroupMessage(groupID, senderID, content, DB_NAME);
    response["action"] = "sendGroupMessage";
    response["groupID"] = groupID;
    response["senderID"] = senderID;
    response["content"] = content;
    
    qDebug() << "[GroupMessage] DB result - success:" << response["success"].toBool();
    
    // Get sender name
    QString senderName;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetSenderNameConnection");
        db.setDatabaseName(QString::fromLatin1(DB_NAME));
        if (db.open()) {
            QSqlQuery q(db);
            q.prepare("SELECT Username FROM Users WHERE UserID = :UserID;");
            q.bindValue(":UserID", senderID);
            if (q.exec() && q.next()) {
                senderName = q.value(0).toString();
            }
            db.close();
        }
        QSqlDatabase::removeDatabase("GetSenderNameConnection");
    }
    response["senderName"] = senderName;
    
    qDebug() << "[GroupMessage] Sending response to sender, senderName:" << senderName;
    sendJsonResponse(clientSocket, response);

    // Forward to all online group members
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ForwardGroupMessageConnection");
    db.setDatabaseName(QString::fromLatin1(DB_NAME));
    if (db.open()) {
        QSqlQuery membersQuery(db);
        membersQuery.prepare("select UserID from GroupMembers where GroupID = :GroupID;");
        membersQuery.bindValue(":GroupID", groupID);
        if (membersQuery.exec()) {
            while (membersQuery.next()) {
                int memberID = membersQuery.value(0).toInt();
                SOCKET targetSocket = Server::getInstance()->getUserSocket(memberID);
                qDebug() << "[GroupMessage] Forwarding to memberID:" << memberID << "socket:" << targetSocket;
                if (targetSocket != 0) {
                    QJsonObject forward;
                    forward["action"] = "receiveGroupMessage";
                    forward["groupID"] = groupID;
                    forward["senderID"] = senderID;
                    forward["senderName"] = senderName;
                    forward["content"] = content;
                    sendJsonResponse(targetSocket, forward);
                }
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("ForwardGroupMessageConnection");

    qDebug() << "[GroupMessage] Sent group message response to client and forwarded to members.";
    return response;
}

QJsonObject getGroupMessages(int groupID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetGroupMessagesConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for getting group messages:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("SELECT gm.SenderID, u.Username, gm.Content, gm.SentAt "
                  "FROM GroupMessages gm "
                  "JOIN Users u ON gm.SenderID = u.UserID "
                  "WHERE gm.GroupID = :GroupID ORDER BY gm.SentAt ASC LIMIT 50;");
    query.bindValue(":GroupID", groupID);

    QJsonArray messagesArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject messageObj;
            messageObj["senderID"] = query.value(0).toInt();
            messageObj["senderName"] = query.value(1).toString();
            messageObj["content"] = query.value(2).toString();
            messageObj["sentAt"] = query.value(3).toString();
            messagesArray.append(messageObj);
        }
        result["success"] = true;
        result["messages"] = messagesArray;
    } else {
        qDebug() << "Getting group messages failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve group messages.";
    }

    db.close();
    QSqlDatabase::removeDatabase("GetGroupMessagesConnection");
    return result;
}

QJsonObject handleGetGroupMessages(const QJsonObject &request, SOCKET clientSocket)
{
    int groupID = request["groupID"].toInt();
    QJsonObject response = getGroupMessages(groupID, DB_NAME);
    response["action"] = "getGroupMessages";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent get group messages response to client.";
    return response;
}

QJsonObject getUserGroups(int userID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetUserGroupsConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for getting user groups:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("select g.GroupID, g.GroupName from Groups g join GroupMembers gm on g.GroupID = gm.GroupID where gm.UserID = :UserID;");
    query.bindValue(":UserID", userID);

    QJsonArray groupsArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject groupObj;
            groupObj["groupID"] = query.value(0).toInt();
            groupObj["groupName"] = query.value(1).toString();
            groupsArray.append(groupObj);
        }
        result["success"] = true;
        result["groups"] = groupsArray;
    } else {
        qDebug() << "Getting user groups failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve user groups.";
    }

    db.close();
    QSqlDatabase::removeDatabase("GetUserGroupsConnection");
    return result;
}

QJsonObject handleGetUserGroups(const QJsonObject &request, SOCKET clientSocket)
{
    int userID = request["userID"].toInt();
    QJsonObject response = getUserGroups(userID, DB_NAME);
    response["action"] = "getUserGroups";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent get user groups response to client.";
    return response;
}

QJsonObject getGroupMembers(int groupID, const std::string &dbName)
{
    QJsonObject result;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "GetGroupMembersConnection");
    db.setDatabaseName(QString::fromStdString(dbName));

    if (!db.open()) {
        qDebug() << "Failed to open database for getting group members:" << db.lastError().text();
        result["success"] = false;
        result["message"] = "Database connection error.";
        return result;
    }

    QSqlQuery query(db);
    query.prepare("select u.UserID, u.Username, u.Status, gm.JoinedAt "
                  "from GroupMembers gm join Users u on gm.UserID = u.UserID "
                  "where gm.GroupID = :GroupID order by u.Username ASC;");
    query.bindValue(":GroupID", groupID);

    QJsonArray membersArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject m;
            m["userID"] = query.value(0).toInt();
            m["username"] = query.value(1).toString();
            m["status"] = query.value(2).toInt();
            m["joinedAt"] = query.value(3).toString();
            membersArray.append(m);
        }
        result["success"] = true;
        result["members"] = membersArray;
    } else {
        qDebug() << "Getting group members failed:" << query.lastError().text();
        result["success"] = false;
        result["message"] = "Failed to retrieve group members.";
    }

    db.close();
    QSqlDatabase::removeDatabase("GetGroupMembersConnection");
    return result;
}

QJsonObject handleGetGroupMembers(const QJsonObject &request, SOCKET clientSocket)
{
    int groupID = request["groupID"].toInt();
    QJsonObject response = getGroupMembers(groupID, DB_NAME);
    response["action"] = "getGroupMembers";
    sendJsonResponse(clientSocket, response);
    qDebug() << "Sent get group members response to client.";
    return response;
}

void initGroupHandlers(std::map<QString, std::function<QJsonObject(const QJsonObject &, SOCKET)>> &handlers)
{
    handlers["createGroup"] = handleCreateGroup;
    handlers["addUserToGroup"] = handleAddUserToGroup;
    handlers["removeUserFromGroup"] = handleRemoveUserFromGroup;
    handlers["leaveGroup"] = handleLeaveGroup;
    handlers["sendGroupMessage"] = handleSendGroupMessage;
    handlers["getGroupMessages"] = handleGetGroupMessages;
    handlers["getUserGroups"] = handleGetUserGroups;
    handlers["getGroupMembers"] = handleGetGroupMembers;
}
