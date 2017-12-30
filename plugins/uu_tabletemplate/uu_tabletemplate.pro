#  This file is part of InyokaEdit.
#  Copyright (C) 2012-2017 The InyokaEdit developers
#
#  InyokaEdit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  InyokaEdit is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with InyokaEdit.  If not, see <http://www.gnu.org/licenses/>.

TEMPLATE      = lib
CONFIG       += plugin
TARGET        = uu_tabletemplate
DESTDIR       = ../

VERSION       = 1.3.0
QMAKE_TARGET_DESCRIPTION = "uu.de table template plugin for InyokaEdit"
QMAKE_TARGET_COPYRIGHT   = "(C) 2012-2017 - Christian Schärf, Thorsten Roth"

DEFINES      += PLUGIN_NAME=\\\"$$TARGET\\\" \
                PLUGIN_VERSION=\"\\\"$$VERSION\\\"\" \
                PLUGIN_COPY=\"\\\"$$QMAKE_TARGET_COPYRIGHT\\\"\"

MOC_DIR       = ./.moc
OBJECTS_DIR   = ./.objs
UI_DIR        = ./.ui
RCC_DIR       = ./.rcc

QT           += network

qtHaveModule(webkitwidgets) {
  QT         += webkitwidgets
  DEFINES    += USEQTWEBKIT
} else {
  qtHaveModule(webenginewidgets) {
    QT       += webenginewidgets
  } else {
    error("Neither QtWebKit nor QtWebEngine installation found!")
  }
}

include(../../application/templates/templates.pri)
include(../../application/parser/parser.pri)

HEADERS      += uu_tabletemplate.h \
                ../../application/syntaxcheck.h

SOURCES      += uu_tabletemplate.cpp \
                ../../application/syntaxcheck.cpp

FORMS        += uu_tabletemplate.ui

RESOURCES     = res/uu_tabletemplate_resources.qrc

TRANSLATIONS += lang/uu_tabletemplate_de.ts
