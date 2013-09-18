import QtQuick 2.0

Item{
    id: navNoRoute
    anchors.fill: parent

    Image {
        anchors.fill: parent
        source: "../res/nav/map.png"
    }

    // Zoom "+" button
    Image{
        id: zoomIn
        source: "../res/buttons/zoom.png"
        anchors.top: parent.top
        anchors.topMargin:  1/5 * contentLoader.height
        anchors.left: parent.left
        anchors.leftMargin: 1/100 * contentLoader.width

        Text{
            id: zoomInBtn
            anchors.centerIn: parent
            text: "+"
            color: "white"
            font.pixelSize: 30
        }

        MouseArea{
            anchors.fill: parent
            onPressed: {
                parent.source = "../res/buttons/zoom_pressed.png"
                zoomInBtn.color = "black"
            }
            onReleased:  {
                parent.source = "../res/buttons/zoom.png"
                zoomInBtn.color = "white"
            }

            onClicked: {
                //Some behavior
            }
        }
    }

    // Zoom "-" button
    Image{
        id: zoomOut
        anchors.top: parent.top
        anchors.topMargin:  2/5 * contentLoader.height
        anchors.left: parent.left
        anchors.leftMargin: 1/100 * contentLoader.width
        source: "../res/buttons/zoom.png"

        Text{
            id: zoomOutBtn
            anchors.centerIn: parent
            text: "-"
            color: "white"
            font.pixelSize: 30
        }

        MouseArea{
            anchors.fill: parent
            onPressed: {
                parent.source = "../res/buttons/zoom_pressed.png"
                zoomOutBtn.color = "black"
            }
            onReleased:  {
                parent.source = "../res/buttons/zoom.png"
                zoomOutBtn.color = "white"
            }

            onClicked: {
                //Some behavior
            }
        }
    }

    Image{
        id: compas
        anchors.horizontalCenter: zoomOut.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1/5 * contentLoader.height
        source: "../res/nav/compass.png"
    }

    // Options button
    Image{
        id: navOptions
        anchors.top: parent.top
        anchors.right: parent.right
        source: "../res/nav/options.png"

        MouseArea{
            anchors.fill: parent
            onPressed: {
                parent.source = "../res/nav/options_pressed.png"
            }
            onReleased:  {
                parent.source = "../res/nav/options.png"
            }
            onClicked: {
            //Options screen
            }
        }
    }

    // Destination search button
    Image{
        id: destSearch
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        source: "../res/buttons/longest_oval_btn.png"

        Image{
            id: destIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 15
            source: "../res/nav/dest_icon.png"
        }

        Text{
            id: destText
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: destIcon.right
            anchors.leftMargin: 5
            text: "Destination"
            color: "#1d81d5"
            font.pixelSize: 20
        }

        MouseArea {
            anchors.fill: parent

            onPressed: {
                destSearch.source = "../res/buttons/longest_oval_btn_pressed.png"
                destIcon.source = "../res/nav/dest_icon_black.png"
                destText.color = "black"
            }

            onReleased: {
                destSearch.source = "../res/buttons/longest_oval_btn.png"
                destIcon.source = "../res/nav/dest_icon.png"
                destText.color = "#1d81d5"
            }

            onClicked: {
                contentLoader.go("./views/NavigationMenuGridView.qml")
            }
        }
    }


}
