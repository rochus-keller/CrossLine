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

#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <Udb/DatabaseException.h>
#include <Udb/BtreeCursor.h>
#include <Udb/BtreeStore.h>
#include <QSettings>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QtPlugin>
#include <QtSingleApplication>
#include <QtDebug>
#include <iostream>
#include "AppContext.h"
using namespace Oln;

static QtMessageHandler s_oldHandler = 0;
void messageHander(QtMsgType type, const QMessageLogContext& ctx, const QString& message)
{
	QFile file(QString("%1/CrossLine.log").arg(QApplication::applicationDirPath()));
	file.open(QIODevice::Append);
    const QByteArray tmp = message.toUtf8();
    file.write(tmp,tmp.size());
	file.write("\n");
	if( s_oldHandler )
        s_oldHandler(type, ctx, message );
	else
        std::cerr << tmp.constData() << std::endl;
}

class MyApp : public QtSingleApplication
{
public:
	MyApp(const QString &id, int &argc, char **argv):QtSingleApplication(id, argc, argv) {}
	bool notify(QObject * o, QEvent * e)
	{
		try
		{
			return QtSingleApplication::notify(o, e);
		}catch( Udb::DatabaseException& ex )
		{
			qWarning() << "MyApp::notify exception:" << ex.getMsg();
			QMessageBox::critical( activeWindow(), AppContext::tr("Error"),
								   QString("Database Error: [%1] %2").arg( ex.getCodeString() ).arg( ex.getMsg() ), QMessageBox::Abort );
		}catch( std::exception& ex )
		{
			qWarning() << "MyApp::notify exception:" << ex.what();
			QMessageBox::critical( activeWindow(), AppContext::tr("Error"),
				QString("Generic Error: %1").arg( ex.what() ), QMessageBox::Abort );
		}catch( ... )
		{
			qWarning() << "MyApp::notify exception: unknown";
			QMessageBox::critical( activeWindow(), AppContext::tr("Error"),
				QString("Generic Error: unexpected internal exception"), QMessageBox::Abort );
		}
		return false;
	}
};

int main(int argc, char *argv[])
{
	MyApp app( AppContext::s_appName, argc, argv);

    s_oldHandler = qInstallMessageHandler(messageHander);

	QIcon icon;
	icon.addFile( ":/CrossLine/Images/metro16.png" );
	icon.addFile( ":/CrossLine/Images/metro32.png" );
	icon.addFile( ":/CrossLine/Images/metro48.png" );
	icon.addFile( ":/CrossLine/Images/metro64.png" );
	icon.addFile( ":/CrossLine/Images/metro128.png" );
	app.setWindowIcon( icon );

#ifndef _DEBUG
	try
	{
#endif
		AppContext ctx;
        QObject::connect( &app, SIGNAL( messageReceived(const QString&)),
                              &ctx, SLOT( onOpen(const QString&) ) );

		QString path;

		if( ctx.getSet()->contains( "App/Font" ) )
		{
			QFont f1 = QApplication::font();
			QFont f2 = ctx.getSet()->value( "App/Font" ).value<QFont>();
			f1.setFamily( f2.family() );
			f1.setPointSize( f2.pointSize() );
			QApplication::setFont( f1 );
		}
		
		QStringList args = QCoreApplication::arguments();
        QString oidArg;
		for( int i = 1; i < args.size(); i++ ) // arg 0 enthält Anwendungspfad
		{
            if( !args[ i ].startsWith( '-' ) )
			{
				path = args[ i ];
			}else
            {
                if( args[ i ].startsWith( "-oid:") )
                    oidArg = args[ i ];
            }
		}
		
		if( path.isEmpty() )
			path = QFileDialog::getSaveFileName( 0, AppContext::tr("Create/Open Repository - CrossLine"), 
				QString(), QString( "*%1" ).arg( QLatin1String( AppContext::s_extension ) ), 
				0, QFileDialog::DontConfirmOverwrite );
            //| QFileDialog::DontUseNativeDialog : damit funktionieren Aliasse nicht
		if( path.isEmpty() )
			return 0;

        // NOTE: path kommt immer ohne Anführungs- und Schlusszeichen, auch wenn er Spaces enthält
        if( path.contains( QChar(' ') ) && path[0] != QChar('"') )
            path = QChar('"') + path + QChar('"');

        if( !oidArg.isEmpty() )
            path += QChar(' ') + oidArg;

#ifndef _DEBUG
        if( app.sendMessage( path ) )
                 return 0; // Eine andere Instanz ist bereits offen
#endif

		if( !ctx.open( path ) )
			return -1;

		const int res = app.exec();
		//ctx.getTxn()->getDb()->dump();
		return res;
#ifndef _DEBUG
	}catch( Udb::DatabaseException& e )
	{
		QMessageBox::critical( 0, AppContext::tr("CrossLine Error"),
			QString("Database Error: [%1] %2").arg( e.getCodeString() ).arg( e.getMsg() ), QMessageBox::Abort );
		return -1;
	}catch( std::exception& e )
	{
		QMessageBox::critical( 0, AppContext::tr("CrossLine Error"),
			QString("Generic Error: %1").arg( e.what() ), QMessageBox::Abort );
		return -1;
	}catch( ... )
	{
		QMessageBox::critical( 0, AppContext::tr("CrossLine Error"),
			QString("Generic Error: unexpected internal exception"), QMessageBox::Abort );
		return -1;
	}
#endif
	return 0;
}
