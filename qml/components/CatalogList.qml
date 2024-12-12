import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
//import QtGraphicalEffects 1.12

import FontAwesome 1.0
import "." as laiinComponents

ListView {
    id: catalogList
    width: 910; height: 3000//width: catalogList.boxWidth; height: (catalogList.boxHeight * count) + (spacing * (count - 1)) //anchors.fill: parent//width: contentItem.childrenRect.width; height: contentItem.childrenRect.height // The height is the height of all the models in the listview combined
    spacing: 5
    property real boxWidth: 910
    property real boxHeight: 300
    Component.onCompleted: {
        width = catalogList.boxWidth
        height = (catalogList.boxHeight * count) + (spacing * (count - 1))// + footerItem.height + mainWindow.footer.height
        // size and children size should match
        /*console.log("catalogList size", width, height)
        console.log("catalogList children size", childrenRect.width, childrenRect.height)*/
    }
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
    }    
    model: Backend.getListings()//10
    delegate: Rectangle {
        id: productBox
        width: catalogList.boxWidth; height: catalogList.boxHeight // The height of each individual model item/ list element
        color: (laiinComponents.Style.darkTheme) ? (laiinComponents.Style.themeName == "PurpleDust" ? "#17171c" : "#181a1b") : "#c9c9cd"//"#f0f0f0"
        border.color: (laiinComponents.Style.darkTheme) ? "#404040" : "#4d4d4d"////(laiinComponents.Style.darkTheme) ? "#ffffff" : "#000000"
        border.width: 0
        radius: 5
        // Hide radius at right border
        Rectangle {
            width: productImageRect.width / 2; height: productImageRect.height - (parent.border.width * 2)
            anchors.top: parent.top; anchors.topMargin: parent.border.width
            anchors.bottom: parent.bottom; anchors.bottomMargin: parent.border.width
            anchors.right: productImageRect.right; anchors.rightMargin: -2
            color: productImageRect.color//"blue"
            border.width: parent.border.width; border.color: productImageRect.color
            radius: 0
        }

        Rectangle {
            id: productImageRect
            anchors.top: parent.top
            anchors.left: parent.left // so that margins will also apply to left and right sides
            anchors.bottom: parent.bottom
            anchors.margins: parent.border.width
            width: (parent.width / 3); height: parent.height
            color: "#ffffff"//"transparent"
            radius: parent.radius
                             
            Image {
                id: productImage
                source: "image://listing?id=%1&image_id=%2".arg(modelData.key).arg(modelData.product_images[0].name)//"file:///" + modelData.product_image_file//"qrc:/assets/images/image_gallery.png"
                anchors.centerIn: parent
                width: 192; height: 192
                fillMode: Image.PreserveAspectFit
                mipmap: true
                asynchronous: true
                onStatusChanged: {
                    if (productImage.status === Image.Error) {
                        // Handle the error by displaying a fallback or placeholder image
                        source = "image://listing?id=%1&image_id=%2".arg(modelData.key).arg("thumbnail.jpg")
                    }
                }
                    
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    onEntered: {
                        productBox.border.width = 1
                    }
                    onExited: {
                        productBox.border.width = 0
                    }
                    onClicked: { 
                        navBar.uncheckAllButtons() // Uncheck all navigational buttons
                        console.log("Loading product page ...");
                        pageStack.pushPageWithProperties("qrc:/qml/pages/ProductPage.qml", { "model": modelData })////, { "listingId": modelData.listing_uuid })
                    }
                    cursorShape: Qt.PointingHandCursor
                }                    
            }
        } // ProductImageRect
    }     
}
