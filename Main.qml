import QtQuick
import QtQuick.Controls
import Server

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Chat Server")

    Server {
        id: server
        Component.onCompleted: server.startServer()
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Text {
            text: "Server IP: " + server.serverIp
            font.pixelSize: 20
        }

        Text {
            text: "Server Port: " + server.serverPort
            font.pixelSize: 20
        }
    }
}
