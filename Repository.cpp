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

#include "Repository.h"
#include "TypeDefs.h"
#include <Udb/DatabaseException.h>
#include <Oln2/OutlineItem.h>
#include <QApplication>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include "AppContext.h"
using namespace Oln;
using namespace Udb;

Repository::Repository(QObject *parent) :
    QObject(parent),d_db(0),d_txn(0)
{
}

Repository::~Repository()
{
    d_root = Obj();
	if( d_txn )
		delete d_txn;
	if( d_db )
		delete d_db;
}

Udb::Database* Repository::getDb() const
{
	return d_txn->getDb();
}

bool Repository::open( QString path )
{
	if( !path.toLower().endsWith( QLatin1String( AppContext::s_extension ) ) )
		path += QLatin1String( AppContext::s_extension );
	d_db = new Database( this );
	try
	{
		d_db->open( path );
		d_db->setCacheSize( 10000 ); // RISK
		d_txn = new Transaction( d_db, this );
		TypeDefs::init( *d_db );
		QUuid uuid = AppContext::s_rootUuid;
		d_root = d_txn->getObject( uuid );
		if( d_root.isNull() )
			d_root = d_txn->createObject( uuid );
		d_txn->commit();
		d_txn->setIndividualNotify(false); // RISK
		OutlineItem::doBackRef();
		d_txn->addCallback( OutlineItem::itemErasedCallback );
        d_db->registerDatabase();
		return true;
	}catch( DatabaseException& e )
	{
		QMessageBox::critical( 0, tr("Create/Open Repository"),
			tr("Error <%1>: %2").arg( e.getCodeString() ).arg( e.getMsg() ) );
		return false;
	}
}

