import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
//import QtGraphicalEffects 1.12// replaced with "import Qt5Compat.GraphicalEffects 1.15" in Qt6// ColorOverlay

import "." as laiinComponents // Hint

RowLayout {
    id: navBar
    readonly property string defaultButtonColor: "#6b5b95"
    property bool useDefaultButtonColor: false
    property alias messageCounterText: messagesButton.count//messageCounter.text
    
    function getCheckedButton() {
        return navBarButtonGroup.checkedButton;
    }
    function uncheckAllButtons() {
        navBarButtonGroup.checkState = Qt.Unchecked;
    }
    function checkButtonByIndex(buttonIndex) {
        if(buttonIndex == 0) walletButton.checked = true
        if(buttonIndex == 1) shopButton.checked = true
        if(buttonIndex == 2) messagesButton.checked = true
        if(buttonIndex == 3) ordersButton.checked = true
        if(buttonIndex == 4) accountButton.checked = true
    }
        
    ButtonGroup {
        id: navBarButtonGroup
        exclusive: true
        //checkState: conditionParentBox.checkState
        onClicked: {
            console.log("Selected", button.text, "button")
            if(button.text == walletButton.text) {
                if(!Wallet.isConnectedToDaemon()) {
                    button.checked = false
                    messageBox.text = qsTr("wallet must be connected to a daemon")
                    messageBox.open()
                    return
                }
                button.checked = true
                searchBar.children[0].text = ""
                pageStack.pushPage("qrc:/qml/pages/subpages/WalletPage.qml", StackView.Immediate)//_stackview.currentIndex = 0
            }
            if(button.text == shopButton.text) {
                button.checked = true
                searchBar.children[0].text = ""
                pageStack.pushPage("qrc:/qml/pages/subpages/StorePage.qml", StackView.Immediate)
            }
            if(button.text == messagesButton.text) {
                button.checked = true
                searchBar.children[0].text = ""
                pageStack.pushPage("qrc:/qml/pages/subpages/MessagesPage.qml", StackView.Immediate)
            }
            if(button.text == ordersButton.text) {
                button.checked = true
                searchBar.children[0].text = ""
                pageStack.pushPage("qrc:/qml/pages/subpages/OrdersPage.qml", StackView.Immediate)
            }
            if(button.text == accountButton.text) {
                button.checked = true
                searchBar.children[0].text = ""
                pageStack.pushPage("qrc:/qml/pages/subpages/AccountPage.qml", StackView.Immediate)
            }                                                        
        }
    }
        
    Button {
        id: walletButton
        text: qsTr("Funds")
        ButtonGroup.group: navBarButtonGroup // attaches a button to a button group
        display: AbstractButton.IconOnly
        //checkable: true
        hoverEnabled: true
        
        icon.source: "qrc:/assets/images/wallet.png"
        icon.color: (!checked && this.hovered) ? laiinComponents.Style.moneroOrangeColor : "#ffffff"
        //property string reservedColor: laiinComponents.Style.moneroOrangeColor
                        
        background: Rectangle {
            color: (parent.checked) ? laiinComponents.Style.moneroOrangeColor : "transparent"
            border.color: laiinComponents.Style.moneroOrangeColor
            border.width: (!parent.checked && parent.hovered) ? 1 : 0
            radius: 5
        }
        
        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = false
            cursorShape: !parent.checked ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
           
        laiinComponents.Hint {
            id: walletButtonHint
            visible: parent.hovered
            text: parent.text
            pointer.visible: false
            delay: 0 // Show immediately on hovering over button
            //textObject.font.bold: true
        }
    }
                            
    Button {
        id: shopButton
        text: qsTr("Store")
        ButtonGroup.group: navBarButtonGroup
        display: AbstractButton.IconOnly//AbstractButton.TextBesideIcon
        hoverEnabled: true
        
        icon.source: "qrc:/assets/images/shop.png"
        icon.color: (!checked && this.hovered) ? reservedColor : "#ffffff"
        property string reservedColor: (useDefaultButtonColor) ? defaultButtonColor : "royalblue"
                        
        background: Rectangle {
            color: (parent.checked) ? parent.reservedColor : "transparent"
            border.color: parent.reservedColor
            border.width: (!parent.checked && parent.hovered) ? 1 : 0
            radius: 5
        }
        
        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = false
            cursorShape: !parent.checked ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
           
        laiinComponents.Hint {
            visible: parent.hovered
            text: parent.text
            pointer.visible: false
            delay: 0 // Show immediately on hovering over button
            //textObject.font.bold: true
        }
    }
    
    Button {
        id: messagesButton
        text: (messagesButton.count > 0) ? qsTr("Messages (%1)").arg(messagesButton.count.toString()) : qsTr("Messages")
        ButtonGroup.group: navBarButtonGroup
        display: AbstractButton.IconOnly//AbstractButton.TextBesideIcon
        hoverEnabled: true
        property int count: 0
        
        icon.source: "qrc:/assets/images/mailbox.png"
        icon.color: (!checked && this.hovered) ? reservedColor : "#ffffff"
        property string reservedColor: (useDefaultButtonColor) ? defaultButtonColor : "#524656"
                        
        background: Rectangle {
            color: (parent.checked) ? parent.reservedColor : "transparent"
            border.color: parent.reservedColor
            border.width: (!parent.checked && parent.hovered) ? 1 : 0
            radius: 5
        }
        
        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = false
            cursorShape: !parent.checked ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
           
        laiinComponents.Hint {
            visible: parent.hovered
            text: parent.text
            pointer.visible: false
            delay: 0
            //textObject.font.bold: true
        }
        
        /*Rectangle {
            visible: (Number(messageCounter.text) > 0) && !messagesButton.checked
            anchors.top: parent.top
            anchors.topMargin: -5
            anchors.right: parent.right
            anchors.rightMargin: -5
            z: 1
            width: children[0].contentWidth + 10; height: 20
            radius: 5
            color: "firebrick"
            Text {
                id: messageCounter
                text: "0"
                anchors.centerIn: parent
                color: "#ffffff"
                font.pointSize: 8
            }
        }*/
    }
    
    Button {
        id: ordersButton
        text: qsTr("Orders")
        ButtonGroup.group: navBarButtonGroup
        display: AbstractButton.IconOnly//AbstractButton.TextBesideIcon
        hoverEnabled: true
        
        icon.source: "qrc:/assets/images/order.png"
        icon.color: (!checked && this.hovered) ? reservedColor : "#ffffff"
        property string reservedColor: (useDefaultButtonColor) ? defaultButtonColor : "#607848"
                        
        background: Rectangle {
            color: (parent.checked) ? parent.reservedColor : "transparent"
            border.color: parent.reservedColor
            border.width: (!parent.checked && parent.hovered) ? 1 : 0
            radius: 5
        }
        
        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = false
            cursorShape: !parent.checked ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
           
        laiinComponents.Hint {
            visible: parent.hovered
            text: parent.text
            pointer.visible: false
            delay: 0
            //textObject.font.bold: true
        }
    }      

    Button {
        id: accountButton
        text: qsTr("Account")//qsTr("User")
        ButtonGroup.group: navBarButtonGroup
        display: AbstractButton.IconOnly//AbstractButton.TextBesideIcon
        hoverEnabled: true
        
        icon.source: "qrc:/assets/images/user.png"
        icon.color: (!checked && this.hovered) ? reservedColor : "#ffffff"
        property string reservedColor: (useDefaultButtonColor) ? defaultButtonColor : "#cd8500"
                        
        background: Rectangle {
            color: (parent.checked) ? parent.reservedColor : "transparent"
            border.color: parent.reservedColor
            border.width: (!parent.checked && parent.hovered) ? 1 : 0
            radius: 5
        }
        
        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = false
            cursorShape: !parent.checked ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
           
        laiinComponents.Hint {
            visible: parent.hovered
            text: parent.text
            pointer.visible: false
            delay: 0
            //textObject.font.bold: true
        }
    }

    Button {
        id: cartButton
        ////ButtonGroup.group: navBarButtonGroup
        ////autoExclusive: true
        // reference: https://doc.qt.io/qt-5/qml-qtquick-layouts-layout.html
        Layout.alignment: Qt.AlignTop
        // tell the layout that this child will have unique dimensions from the other children
        Layout.preferredHeight : 40
        Layout.preferredWidth : 100
        property string reservedColor: "#323232"//(useDefaultButtonColor) ? defaultButtonColor : "#323232"
        hoverEnabled: true
     
        background: Rectangle {
            //width: cartButton.width; height: cartButton.height//width: 100; height: 40
            color: parent.reservedColor//(parent.checked) ? parent.reservedColor : "transparent"
            //border.color: parent.reservedColor
            //border.width: (!parent.checked && parent.hovered) ? 1 : 0
            radius: 5
        }        

        Text {
            id: cartButtonText
            text: !User.logged ? "0" : User.cartQuantity
            color: "#ffffff"
            font.bold: true
            anchors.left: cartButton.background.left
            anchors.leftMargin: 20
            anchors.top: cartButton.background.top
            anchors.topMargin: (cartButton.background.height - this.height) / 2
        }
                
        Image {
            id: cartButtonIcon
            source: "qrc:/assets/images/cart.png"
            height: 24; width: 24
            anchors.left: cartButtonText.right
            anchors.leftMargin: 10
            anchors.top: cartButton.background.top
            anchors.topMargin: (cartButton.background.height - this.height) / 2
        }
        /*ColorOverlay {
            anchors.fill: cartButtonIcon
            source: cartButtonIcon
            color: "#ffffff"//(!parent.checked && parent.hovered) ? parent.reservedColor : "#ffffff"
            visible: cartButtonIcon.visible
        }*/
        
        onClicked: {
            navBar.uncheckAllButtons();
            searchBar.children[0].text = ""
            pageStack.pushPage("qrc:/qml/pages/CartPage.qml", StackView.Immediate)
        }
                
        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = false
            cursorShape: Qt.PointingHandCursor
        }
           
        /*laiinComponents.Hint {
            visible: parent.hovered
            text: "Cart"
            pointer.visible: false
            delay: 0
            //textObject.font.bold: true
        }*/        
    }
}
