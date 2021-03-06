import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import org.kde.kirigami 2.2 as Kirigami
import org.kde.maui 1.0 as Maui

import "../InfoView"

import "../../utils/Player.js" as Player
import "../../db/Queries.js" as Q
import "../../utils"
import "../../widgets"
import "../../view_models"
import "../../view_models/BabeTable"

Item
{
    id: mainPlaylistRoot

    property alias artwork : artwork
    property alias albumsRoll : albumsRoll
    property alias cover : cover
    property alias list : table.list
    property alias table: table
    property alias infoView : infoView

    property alias contextMenu : table.contextMenu
    property alias mainlistContext : mainlistContext
    property alias headerMenu : table.headerMenu
    property alias stack : stackView

    signal coverDoubleClicked(var tracks)
    signal coverPressed(var tracks)
    focus: true

    PlaylistMenu
    {
        id: playlistMenu
        onClearOut: Player.clearOutPlaylist()
        onHideCover: cover.visible = !cover.visible
        onClean: Player.cleanPlaylist()
        onSaveToClicked: table.saveList()
    }

    //    Rectangle
    //    {
    //        anchors.fill: parent
    //        color: darkDarkColor
    //        z: -999
    //    }

    GridLayout
    {
        id: playlistLayout
        anchors.fill: parent
        width: parent.width
        rowSpacing: 0
        rows: 4
        columns: 1

        Item
        {
            id: cover
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? coverSize : 0
            Layout.maximumHeight: coverSize
            visible:  !mainlistEmpty

            Rectangle
            {
                visible: cover.visible
                anchors.fill: parent
                color: viewBackgroundColor
                z: -999

                Image
                {
                    id: artwork
                    visible: true
                    anchors.fill: parent
                    sourceSize.height: coverSize * 0.2
                    sourceSize.width: coverSize * 0.2
                    source: currentArtwork ? "file://"+encodeURIComponent(currentArtwork)  : "qrc:/assets/cover.png"
                    fillMode: Image.PreserveAspectCrop
                }

                FastBlur
                {
                    visible: artwork.visible
                    anchors.fill: parent
                    source: artwork
                    radius: 100
                    transparentBorder: false
                    cached: true
                }
            }

            Item
            {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter

                AlbumsRoll
                {
                    id: albumsRoll
                    height: parent.height
                    width: parent.width
                    anchors.verticalCenter: parent.vertical
                }

            }
        }

        Maui.ToolBar
        {
            id: mainlistContext
            clip: false
            width: parent.width
            implicitHeight: toolBarHeightAlt
            visible : !focusMode

            Layout.row: 2
            Layout.column: 1
            Layout.fillWidth: true

            MouseArea
            {
                anchors.fill: parent
                drag.target: mainlistContext
                drag.axis: Drag.YAxis
                drag.minimumY: 0
                drag.maximumY:stackView.currentItem === table ?  coverSize : 0
                z: -1
                onMouseYChanged:
                {
                    if(stackView.currentItem === table )
                    {
                        cover.height = mainlistContext.y

                        if(mainlistContext.y < coverSize*0.8)
                        {
                            cover.visible = false
                            mainlistContext.y = 0
                        }else cover.visible = true
                    }
                }
            }

            leftContent: Maui.ToolButton
            {
                id: infoBtn
                iconName: stackView.currentItem === table ? "documentinfo" : "go-previous"
                onClicked:
                {
                    if( stackView.currentItem !== table)
                    {
                        cover.visible  = true
                        stackView.pop(table) }
                    else {
                        cover.visible  = false
                        stackView.push(infoView)
                    }
                }
            }


            //                Item
            //                {
            //                    Layout.fillWidth: true

            //                    BabeButton
            //                    {
            //                        anchors.centerIn: parent
            ////                        iconColor: darkTextColor
            //                        iconName: "headphones"
            //                        onClicked: goFocusMode()
            //                    }
            //                }

            middleContent: Maui.PieButton
            {
                iconName: "list-add"

                model: ListModel
                {
                    ListElement{iconName: "amarok-video" ; btn: "video"}
                    ListElement{iconName: "documentinfo" ; btn: "info"}
                    ListElement{iconName: "artists" ; btn: "similar"}
                }

                onItemClicked:
                {
                    if(item.btn === "video")
                    {
                        youtubeView.openVideo = 1
                        youtube.getQuery(currentTrack.title+" "+currentTrack.artist)
                        pageStack.currentIndex = 1
                        currentView = viewsIndex.youtube
                    }

                    if(item.btn === "info")
                    {
                        if( stackView.currentItem !== table)
                        {
                            cover.visible  = true
                            stackView.pop(table) }
                        else {
                            cover.visible  = false
                            stackView.push(infoView)
                        }
                    }
                }
            }

            rightContent : Maui.ToolButton
            {
                id: menuBtn
                iconName: "overflow-menu"
                onClicked: isMobile ? playlistMenu.open() : playlistMenu.popup()
            }
        }


        Item
        {
            id: mainPlaylistItem
            visible : !focusMode

            Layout.row: 4
            Layout.column: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.top: mainlistContext.bottom
            focus: true
            //            anchors.bottom: mainPlaylistRoot.searchBox
            StackView
            {
                id: stackView
                anchors.fill: parent
                focus: true

                pushEnter: Transition
                {
                    PropertyAnimation
                    {
                        property: "opacity"
                        from: 0
                        to:1
                        duration: 200
                    }
                }

                pushExit: Transition
                {
                    PropertyAnimation
                    {
                        property: "opacity"
                        from: 1
                        to:0
                        duration: 200
                    }
                }

                popEnter: Transition
                {
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to:1
                        duration: 200
                    }
                }

                popExit: Transition
                {
                    PropertyAnimation
                    {
                        property: "opacity"
                        from: 1
                        to:0
                        duration: 200
                    }
                }

                initialItem: BabeTable
                {
                    id: table
                    headBarVisible: false
                    quickPlayVisible: false
                    coverArtVisible: true
                    trackRating: true
                    showIndicator : true
                    menuItemVisible: false
                    holder.message : "<h2>Meh!</h2><p>Start putting together your playlist!</p>"
                    holder.emoji: "qrc:/assets/face-sleeping.png"

                    onRowClicked: play(index)

                    onArtworkDoubleClicked: contextMenu.babeIt(index)

                    Component.onCompleted:
                    {
                        var list = bae.lastPlaylist()
                        var n = list.length

                        if(n>0)
                        {
                            for(var i = 0; i < n; i++)
                            {
                                var where = "url = \""+list[i]+"\""
                                var query = Q.GET.tracksWhere_.arg(where)
                                var track = bae.get(query)
                                Player.appendTrack(track[0])
                            }
                        }else
                        {
                            where = "babe = 1"
                            query = Q.GET.tracksWhere_.arg(where)
                            var tracks = bae.get(query)

                            for(var pos=0; pos< tracks.length; pos++)
                                Player.appendTrack(tracks[pos])

                        }

                        if(autoplay)
                            Player.playAt(0)

                        //                                    var pos = bae.lastPlaylistPos()
                        //                                    console.log("POSSS:", pos)
                        //                                    list.currentIndex = pos
                        //                                    play(list.model.get(pos))
                    }
                }

                InfoView
                {
                    id: infoView
                }

            }
        }
    }

    function goFocusMode()
    {

        if(focusMode)
        {
            if(isMobile)
            {
                root.width = screenWidth
                root.height= screenHeight
            }else
            {
                cover.y = 0
                root.maximumWidth = screenWidth
                root.minimumWidth = columnWidth
                root.maximumHeight = screenHeight
                root.minimumHeight = columnWidth

                root.width = columnWidth
                root.height = 700
            }

        }else
        {
            if(isMobile)
            {

            }else
            {
                root.maximumWidth = columnWidth
                root.minimumWidth = columnWidth
                root.maximumHeight = columnWidth
                root.minimumHeight = columnWidth
                //                root.footer.visible = false
                //                mainlistContext.visible = false


            }
        }

        focusMode = !focusMode
    }

    function play(index)
    {
        prevTrackIndex = currentTrackIndex
        Player.playAt(index)

    }
}
