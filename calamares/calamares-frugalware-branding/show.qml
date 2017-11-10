/* === This file is part of Calamares - <http://github.com/calamares> ===
 *
 *   Copyright 2015, Teo Mrnjavac <teo@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0;
import calamares.slideshow 1.0;

Presentation
{
    id: presentation

    Timer {
        interval: 30000
        running: true
        repeat: true
        onTriggered: presentation.goToNextSlide()
    }

    Slide {

        Image {
            id: background1
            source: "official_background.jpg"
            width: 600; height: 332
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
        }
        Text {
            anchors.horizontalCenter: background1.horizontalCenter
            anchors.top: background1.bottom
            text: "Welcome to FrugalWare Linux.<br/>"+
                  "FrugalWare Linux is driven by a hardworking and dedicated community.<br/>"+
                  "During the installation, this slideshow will provide a quick introduction."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.Center
        }
    }

    Slide {

        Image {
            id: background2
            source: "official_background.jpg"
            width: 600; height: 332
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
        }
        Text {
            anchors.horizontalCenter: background2.horizontalCenter
            anchors.top: background2.bottom
            text: "All of FrugalWare Linux versions are completely customizable<br/>"+
                  "to exactly how you want it. From theming, to the very<br/>"+
                  "kernel itself, it can be changed."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.Center
        }
    }

    Slide {

        Image {
            id: background3
            source: "official_background.jpg"
            width: 600; height: 332
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
        }
        Text {
            anchors.horizontalCenter: background3.horizontalCenter
            anchors.top: background3.bottom
            text: "FrugalWare Linux has three different officially supported editions.<br/>"+
                  "Additionally, there's a multitude of community editions to <br/>"+
                  "choose from, built by the community, for the community."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.Center
        }
    }

    Slide {

        Image {
            id: background4
            source: "official_background.jpg"
            width: 600; height: 332
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
        }
        Text {
            anchors.horizontalCenter: background4.horizontalCenter
            anchors.top: background4.bottom
            text: "FrugalWare Linux uses a derived version of the orignal <br/>"+
                  "pacman from Arch Linux called pacman-g2."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.Center
        }
    }

    Slide {

        Image {
            id: background5
            source: "official_background.jpg"
            width: 600; height: 332
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
        }
        Text {
            anchors.horizontalCenter: background5.horizontalCenter
            anchors.top: background5.bottom
            text: "We appreciate you choosing FrugalWare Linux, and hope you enjoy<br/>"+
                  "it as much as we do making it! If you have any questions<br/>"+
                  "or feedback, please feel free to visit the forum, IRC, or wiki."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.Center
        }
    }
}
