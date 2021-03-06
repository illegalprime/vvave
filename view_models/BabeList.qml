import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import org.kde.kirigami 2.2 as Kirigami
import QtQuick.Controls.Material 2.1
import org.kde.maui 1.0 as Maui

Maui.Page
{
    id: babeListRoot

    property alias list : babeList
    property alias model : babeList.model
    property alias delegate : babeList.delegate
    property alias count : babeList.count
    property alias currentIndex : babeList.currentIndex
    property alias currentItem : babeList.currentItem
    property alias holder : holder
    property alias section : babeList.section

    property bool wasPulled : false

    signal pulled()

    focus: true
    margins: 0

    function clearTable()
    {
        list.model.clear()
    }

    Maui.Holder
    {
        id: holder
        visible: babeList.count === 0
        focus: true
    }

    ListView
    {
        id: babeList
        anchors.fill: parent
        clip: true

        highlight: Rectangle
        {
            width: babeList.width
            height: babeList.currentItem.height
            color: highlightColor
        }

        focus: true
        interactive: true
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 0
        keyNavigationWraps: true
        keyNavigationEnabled : true

        Keys.onUpPressed: decrementCurrentIndex()
        Keys.onDownPressed: incrementCurrentIndex()
        Keys.onReturnPressed: rowClicked(currentIndex)
        Keys.onEnterPressed: quickPlayTrack(currentIndex)

        boundsBehavior: !isMobile? Flickable.StopAtBounds : Flickable.DragAndOvershootBounds
        flickableDirection: Flickable.AutoFlickDirection

        snapMode: ListView.SnapToItem

        addDisplaced: Transition
        {
            NumberAnimation { properties: "x,y"; duration: 100 }
        }

        ScrollBar.vertical:BabeScrollBar { }

        onContentYChanged:
        {
            if(contentY < -120)
                wasPulled = true

            if(contentY == 0 && wasPulled)
            { pulled(); wasPulled = false}
        }
    }
}
