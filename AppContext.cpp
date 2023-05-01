/*
* Copyright 2010-2018 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the CrossLine application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include "AppContext.h"
#include "TypeDefs.h"
#include <Udb/DatabaseException.h>
#include <QApplication>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QInputDialog>
#include <QLabel>
#include <QDesktopServices>
#include <QVBoxLayout>
#include <QtGui/private/qtextdocument_p.h>
#ifdef _HAS_LUA_
#include <Qtl2/Objects.h>
#include <Qtl2/Variant.h>
#include <Script2/QtObject.h>
#include <Script/Engine2.h>
#include <Oln2/LuaBinding.h>
#include "Binding.h"
#endif
#include "Repository.h"
#include "Outliner.h"
using namespace Oln;
using namespace Udb;

static AppContext* s_inst;
const char* AppContext::s_company = "Dr. Rochus Keller";
const char* AppContext::s_domain = "me@rochus-keller.ch";
const char* AppContext::s_appName = "CrossLine";
const char* AppContext::s_extension = ".cldb";
const char* AppContext::s_rootUuid = "{4A21E972-C254-4282-BD9F-714F8E0C7865}";
const char* AppContext::s_version = "0.9.5";
const char* AppContext::s_date = "2023-05-01";

#ifdef _HAS_LUA_
static int type(lua_State * L)
{
	luaL_checkany(L, 1);
	if( lua_type(L,1) == LUA_TUSERDATA )
	{
		Lua::ValueBindingBase::pushTypeName( L, 1 );
		if( !lua_isnil( L, -1 ) )
			return 1;
		lua_pop( L, 1 );
	}
	lua_pushstring(L, luaL_typename(L, 1) );
	return 1;
}
#endif

AppContext::AppContext(QObject *parent)
	: QObject(parent)
{
	s_inst = this;
	qApp->setOrganizationName( s_company );
	qApp->setOrganizationDomain( s_domain );
	qApp->setApplicationName( s_appName );

    qApp->setStyle("Fusion");

    QTextDocumentPrivate::initialDocumentMargin = 2; // LeanQt specific

	d_set = new QSettings( s_appName, s_appName, this );

	d_styles = new Txt::Styles( this );
	Txt::Styles::setInst( d_styles );
	if( d_set->contains( "Outliner/Font" ) )
	{
		QFont f = d_set->value( "Outliner/Font" ).value<QFont>();
		d_styles->setFontStyle( f.family(), f.pointSize() );
	}
    QDesktopServices::setUrlHandler( tr("xoid"), this, "onHandleXoid" ); // hier ohne SLOT und ohne Args nötig!

#ifdef _HAS_LUA_
	Lua::Engine2::setInst( new Lua::Engine2() );
	Lua::Engine2::getInst()->addStdLibs();
	Lua::Engine2::getInst()->addLibrary( Lua::Engine2::PACKAGE );
	Lua::Engine2::getInst()->addLibrary( Lua::Engine2::OS );
	Lua::Engine2::getInst()->addLibrary( Lua::Engine2::IO );
	Lua::QtObjectBase::install( Lua::Engine2::getInst()->getCtx() );
	Qtl::Variant::install( Lua::Engine2::getInst()->getCtx() );
	Qtl::Objects::install( Lua::Engine2::getInst()->getCtx() );
	Udb::LuaBinding::install( Lua::Engine2::getInst()->getCtx() );
	Oln::LuaBinding::install( Lua::Engine2::getInst()->getCtx() );
	Binding::install( Lua::Engine2::getInst()->getCtx() );
	lua_pushcfunction( Lua::Engine2::getInst()->getCtx(), type );
	lua_setfield( Lua::Engine2::getInst()->getCtx(), LUA_GLOBALSINDEX, "type" );
#endif
}

AppContext::~AppContext()
{
    foreach( Outliner* o, d_outliner )
        o->close(); // das selbst qApp->quit() zuerst alle Fenster schliesst, dürfte das nie vorkommen.
    s_inst = 0;
}

QString AppContext::parseFilePath(const QString & cmdLine )
{
    if( cmdLine.isEmpty() )
        return QString();
    if( cmdLine[0] == QChar('"') )
    {
        // "path"
        const int pos = cmdLine.indexOf( QChar('"'), 1 );
        if( pos != -1 )
            return cmdLine.mid( 1, pos - 1 );
    }else
    {
        // c:\abc.hedb -oid:
        const int pos = cmdLine.indexOf( QChar(' '), 1 );
        if( pos != -1 )
            return cmdLine.left( pos );
    }
    return cmdLine;
}

quint64 AppContext::parseOid(const QString & cmdLine)
{
    if( cmdLine.isEmpty() )
        return 0;
    int pos = -1;
    if( cmdLine[0] == QChar('"') )
        // "path"
       pos = cmdLine.indexOf( QChar('"'), 1 );
    else
        pos = cmdLine.indexOf( QChar(' '), 1 );
    if( pos != -1 )
    {
        // -oid:12345 -xx
        const int pos2 = cmdLine.indexOf( QString("-oid:"), pos + 1 );
        if( pos2 != -1 )
        {
            const int pos3 = cmdLine.indexOf( QChar(' '), pos2 + 5 );
            if( pos3 != -1 )
                return cmdLine.mid( pos2 + 5, pos3 - ( pos2 + 5 ) ).toULongLong();
            else
                return cmdLine.mid( pos2 + 5 ).toULongLong();
        }
    }
    return 0;
}

class _SplashDialog : public QDialog
{
public:
	_SplashDialog( QWidget* p ):QDialog(p){ setFixedSize(10,10); exec(); }
	void paintEvent ( QPaintEvent * event )
	{
		QDialog::paintEvent( event );
		QMetaObject::invokeMethod(this,"accept", Qt::QueuedConnection );
	}
};

bool AppContext::open(QString cmdLine)
{
    const QString path = parseFilePath( cmdLine );
    const quint64 oid = parseOid( cmdLine );

    foreach( Outliner* o, d_outliner )
    {
        if( o->getDoc()->getDb()->getFilePath() == path ) // TODO: Case sensitive paths?
        {
            o->showOid( oid );
			// hide/show, setWindowFlags( Qt::WindowStaysOnTopHint ), QSplashScreen funktionieren nicht
			o->activateWindow();
			o->raise();

#ifndef _WIN32
			// Das funktioniert immer! Alles andere nicht.
			_SplashDialog dlg(o);
			// QMessageBox funktioniert auch, erfordert jedoch Timer oder Click
#else
			o->activateWindow();
			o->raise();
#endif
			return true;
        }
    }

    Repository* r = new Repository( this );
    if( !r->open( path  ) )
    {
        delete r;
        // Wir können das Repository nicht öffnen.
        // Gib false zurück, wenn ansonsten kein Outline offen ist, damit Anwendung enden kann.
        return d_outliner.isEmpty();
    }
	Outliner* o = new Outliner( r );
    connect( o, SIGNAL(closing()), this, SLOT(onCloseOutliner()) );
    o->showOid( oid );
	o->activateWindow();
	o->raise();
#ifndef _WIN32
	_SplashDialog dlg(o);
#endif
    d_outliner.append( o );
    return true;
}

void AppContext::setDocFont( const QFont& f )
{
	d_set->setValue( "Outliner/Font", QVariant::fromValue( f ) );
    d_styles->setFontStyle( f.family(), f.pointSize() );
}

void AppContext::onOpen(const QString &path)
{
    if( !open( path ) )
    {
        if( d_outliner.isEmpty() )
            qApp->quit();
    }
}

void AppContext::onCloseOutliner()
{
    Outliner* o = static_cast<Outliner*>( sender() );
    if( d_outliner.removeAll( o ) > 0 )
    {
        Repository* r = o->getDoc();
        o->deleteLater();
        r->deleteLater();
    }
}

AppContext* AppContext::inst()
{
	return s_inst;
}

void AppContext::onHandleXoid(const QUrl & url)
{
    const QString oid = url.userInfo();
    const QString uid = url.host();
    bool res = Udb::Database::runDatabaseApp( uid, QStringList() << tr("-oid:%1").arg(oid) );
}
