import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import org.kde.kirigami 2.2 as Kirigami
import org.kde.maui 1.0 as Maui

import "../../utils"

import "../../view_models"
import "../../db/Queries.js" as Q
import "../../utils/Help.js" as H
import Link.Codes 1.0

BabeList
{
    id: playlistListRoot

    headBarExit: false
    headBarTitle: "Playlists"

    AddPlaylistDialog
    {
        id:newPlaylistDialog
    }

    signal playSync(int index)   

    headBar.leftContent: Maui.ToolButton
    {
        id : createPlaylistBtn
        anim : true
        iconName : "list-add"
        onClicked : newPlaylistDialog.open()
    }

    headBar.rightContent: Maui.ToolButton
    {
        iconName: "list-remove"
        onClicked: removePlaylist()
    }

    ListModel
    {
        id: playlistListModel

        ListElement { playlist: qsTr("Most Played"); playlistIcon: "view-media-playcount"; /*query: Q.Query.mostPlayedTracks*/ }
        ListElement { playlist: qsTr("Favorites"); playlistIcon: "view-media-favorite"}
        ListElement { playlist: qsTr("Recent"); playlistIcon: "view-media-recent"}
        ListElement { playlist: qsTr("Babes"); playlistIcon: "love"}
        ListElement { playlist: qsTr("Online"); playlistIcon: "internet-services"}
        ListElement { playlist: qsTr("Tags"); playlistIcon: "tag"}
        ListElement { playlist: qsTr("Relationships"); playlistIcon: "view-media-similarartists"}
        ListElement { playlist: qsTr("Popular"); playlistIcon: "view-media-chart"}
        ListElement { playlist: qsTr("Genres"); playlistIcon: "view-media-genre"}
    }

    model: playlistListModel

    delegate : PlaylistViewDelegate
    {
        id: delegate
        width: playlistListRoot.width

        Connections
        {
            target : delegate

            onClicked :
            {
                currentIndex = index
                var playlist = playlistListModel.get(index).playlist
                filterList.section.property = ""

                switch(playlist)
                {
                case "Most Played":

                    playlistViewRoot.populate(Q.GET.mostPlayedTracks);
                    break;

                case "Favorites":

                    filterList.section.property = "stars"
                    playlistViewRoot.populate(Q.GET.favoriteTracks);
                    break;

                case "Recent":

                    playlistViewRoot.populate(Q.GET.recentTracks);
                    break;

                case "Babes":

                    playlistViewRoot.populate(Q.GET.babedTracks);
                    break;

                case "Online":

                    playlistViewRoot.populate(Q.GET.favoriteTracks);
                    break;

                case "Tags":
                    populateExtra(Q.GET.tags, "Tags")
                    break;

                case "Relationships":

                    playlistViewRoot.populate(Q.GET.favoriteTracks);
                    break;

                case "Popular":

                    playlistViewRoot.populate(Q.GET.favoriteTracks);
                    break;

                case "Genres":

                    populateExtra(Q.GET.genres, "Genres")
                    break;

                default:

                    playlistViewRoot.populate(Q.GET.playlistTracks_.arg(playlist));
                    break;

                }
            }
        }
    }
}
