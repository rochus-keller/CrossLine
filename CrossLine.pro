TEMPLATE = app
TARGET = CrossLine
QT += network

INCLUDEPATH += ../NAF ./..
#CONFIG += HasCLucene

win32 {
    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/Qt
    RC_FILE = CrossLine.rc
    DEFINES -= UNICODE
    CONFIG(debug, debug|release) {
		HasCLucene { LIBS += -lQtCLucened }
		LIBS += -lqjpegd -lqgifd
     } else {
		HasCLucene { LIBS += -lQtCLucene }
		LIBS += -lqjpeg -lqgif
     }
 }else {
	INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/Qt
	DESTDIR = ./tmp
	OBJECTS_DIR = ./tmp
	CONFIG(debug, debug|release) {
		DESTDIR = ./tmp-dbg
		OBJECTS_DIR = ./tmp-dbg
		DEFINES += _DEBUG
	}
	RCC_DIR = ./tmp
    UI_DIR = ./tmp
    MOC_DIR = ./tmp
	HasCLucene { LIBS += -lQtCLucene }
	LIBS += -lqjpeg -lqgif
 }

#Resource file(s)
RESOURCES += ./CrossLine.qrc

#Header files
HEADERS += \
	../CrossLine/DocSelector.h \
	../CrossLine/DocTabWidget.h \
	../CrossLine/AppContext.h \
    ../CrossLine/ChangeNameDlg.h \
    ../CrossLine/DocTraceMdl.h \
    ../CrossLine/EccoToOutline.h \
    ../CrossLine/Outliner.h \
    ../CrossLine/TypeDefs.h \
	../CrossLine/Repository.h \
	../CrossLine/SearchView2.h

#Source files
SOURCES += \
	../CrossLine/DocSelector.cpp \
	../CrossLine/DocTabWidget.cpp \
	../CrossLine/AppContext.cpp \
    ../CrossLine/ChangeNameDlg.cpp \
    ../CrossLine/DocTraceMdl.cpp \
    ../CrossLine/EccoToOutline.cpp \
    ../CrossLine/CrossLineMain.cpp \
    ../CrossLine/Outliner.cpp \
	../CrossLine/TypeDefs.cpp \
	../CrossLine/Repository.cpp \
	../CrossLine/SearchView2.cpp

HasCLucene {
	HEADERS +=   ../CrossLine/Indexer.h  ../CrossLine/SearchView.h
	SOURCES +=   ../CrossLine/Indexer.cpp  ../CrossLine/SearchView.cpp
	DEFINES += _HAS_CLUCENE_
}

#include(../Lua/Lua.pri)
#include(../NAF/Script/Script.pri)
#include(../NAF/Script2/Script2.pri)
#include(../NAF/Qtl2/Qtl2.pri)

include(../Sqlite3/Sqlite3.pri)
include(../QtApp/QtApp.pri)
include(../Udb/Udb.pri)
include(../Stream/Stream.pri)
include(../Oln2/Oln2.pri)
include(../Txt/Txt.pri)
include(../Fts/Fts.pri)
include(../NAF/Gui2/Gui2.pri)

HasLua {
SOURCES += \
	../CrossLine/Binding.cpp
HEADERS += \
	../CrossLine/Binding.h
}
