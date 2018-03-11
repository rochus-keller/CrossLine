/*
* Copyright 2010-2018 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the CrossLine application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.info.
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

// Index basierend auf CLucene; abgelöst durch Fts

#include "Indexer.h"
#include "TypeDefs.h"
#include "AppContext.h"
#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <Udb/ContentObject.h>
#include <Udb/Extent.h>
#include <QProgressDialog>
#include <QtDebug>
#include <QApplication>
#include <QProgressDialog>
#include <QDir>
#include <Oln2/OutlineItem.h>
#include <private/qindexwriter_p.h>
#include <private/qanalyzer_p.h>
#include <private/qindexreader_p.h>
#include <private/qquery_p.h>
#include <private/qsearchable_p.h>
#include <private/qhits_p.h>
#include <private/qqueryparser_p.h>
using namespace Oln;

typedef QVector< QPair<quint64,quint16> > ObjList;
typedef QHash<QString,ObjList> Dict;

// RISK
  class CLuceneError
  {
  public:
  	int error_number;
	char* _awhat;
	TCHAR* _twhat;
  };

static bool toIndex( quint32 t )
{
	return true;
	switch( t )
	{
	default:
		return false;
	}
}

QString Indexer::fetchText( const Udb::Obj& obj, quint32 atom )
{
	if( obj.isNull() )
		return QString();
	Stream::DataCell v;
	Udb::Obj o = obj.getValueAsObj( AttrItemAlias );
	if( !o.isNull() )
		v = o.getValue( atom );
	else
		v = obj.getValue( atom );
	return v.toString(true);
}

static int _findText( const QString& pattern, const QString& text )
{
    QRegExp expr(pattern);
    expr.setPatternSyntax(QRegExp::FixedString);
    expr.setCaseSensitivity(Qt::CaseInsensitive);
    return expr.indexIn(text.simplified());
}

Udb::Obj Indexer::gotoNext( const Udb::Obj& obj )
{
	if( obj.isNull() )
		return Udb::Obj();
	Udb::Obj next = obj;
	while( !next.isNull() )
	{
		if( next.next() ) // wenn false bleibt next auf ursprünglichem Objekt, bzw. wir nicht null.
		{
			// Es gibt einen nächsten
			if( toIndex( next.getType() ) )
				// Wenn es sich um den richtigen Typ handelt, ist es unser Objekt.
				return next;
		}else
			// Es gibt keinen nächsten. Gehe einen Stock nach oben. Der Owner wurde bereits behandelt, darum wieder next.
			next = next.getParent();
	}
	return Udb::Obj();
}

Udb::Obj Indexer::gotoLast( const Udb::Obj& obj )
{
	Udb::Obj res = obj.getLastObj();
	if( res.isNull() || !toIndex( res.getType() ) )
		return obj;
	else
		return gotoLast( res );
}

Udb::Obj Indexer::gotoPrev( const Udb::Obj& obj )
{
	if( obj.isNull() )
		return Udb::Obj();

	Udb::Obj prev = obj;
	while( !prev.isNull() )
	{
		if( prev.prev() )
		{
			// Es gibt einen Vorgänger
			if( toIndex( prev.getType() ) )
			{
				// Der Vorgänger hat den richtigen Type
				// Gehe zuunterst.
				return gotoLast( prev );
			}
		}else
		{
			// Es gibt keinen prev. Gehe also zum Owner. Dieser wurde noch nicht behandelt und ist unser Typ.
			return prev.getParent();

		}
	}
	return Udb::Obj(); 
}

Udb::Obj Indexer::findText( const QString& pattern, const Udb::Obj& cur, bool forward )
{
	if( cur.isNull() )
		return Udb::Obj();
	Udb::Obj obj = cur;
	while( !obj.isNull() )
	{
		//qDebug( "checking %d", obj.getValue( AttrObjRelId ).getInt32() );
		const QString text = fetchText( obj, AttrText );
		const int hit = _findText( pattern, text );
		if( hit != -1 )
		{
			//qDebug( "hit!" );
			return obj;
		}
		if( forward )
			obj = gotoNext( obj );
		else
			obj = gotoPrev( obj );
	}
	return Udb::Obj();
}

const char* Indexer::s_pendingUuid = "{2D826784-B089-4e98-BBB0-F5E4F2F1AD78}";

Indexer::Indexer( Udb::Transaction * txn, QObject * p ):QObject(p)
{
	QUuid uuid = s_pendingUuid;
    d_pending = txn->getOrCreateObject( uuid );
	txn->commit();
	txn->addObserver( this, SLOT(onDbUpdate( Udb::UpdateInfo ) ), false );
}

QString Indexer::getIndexPath() const
{
	if( d_pending.isNull() )
		return QDir::current().absoluteFilePath( QLatin1String( "CrossLine.index" ) );
	QFileInfo info( d_pending.getDb()->getFilePath() );
	return info.absoluteDir().absoluteFilePath( info.completeBaseName() + QLatin1String( ".index" ) );
}

bool Indexer::exists()
{
	return QCLuceneIndexReader::indexExists( getIndexPath() );
}

static QString _fetchId( const Udb::Obj& item )
{
	QString id = item.getString( OutlineItem::AttrAltIdent, true );
	if( id.isEmpty() )
		id = item.getString( OutlineItem::AttrIdent, true );
	if( id.isEmpty() )
	{
		Udb::Obj alias = item.getValueAsObj( OutlineItem::AttrAlias );
		if( !alias.isNull() )
		{
			id = alias.getString( OutlineItem::AttrAltIdent, true );
			if( id.isEmpty() )
				id = alias.getString( OutlineItem::AttrIdent, true );
		}
	}
	return id;
}

static void indexItem( const Udb::Obj& item, const Udb::Obj& doc, QCLuceneIndexWriter& w, QCLuceneAnalyzer& a )
{
	QCLuceneDocument ld;

	const QString text = Indexer::fetchText( item, AttrText );
	const QString id = _fetchId(item);

	int count = 0;

	ld.add(new QCLuceneField(QLatin1String("oid"),
		QString::number( item.getOid(), 16 ), QCLuceneField::STORE_YES | QCLuceneField::INDEX_UNTOKENIZED ) );
	ld.add(new QCLuceneField(QLatin1String("doc"), QString::number( doc.getOid(), 16 ),
							 QCLuceneField::STORE_YES | QCLuceneField::INDEX_UNTOKENIZED ) );
	ld.add(new QCLuceneField(QLatin1String("title"), QString::number( item.getValue(AttrItemIsTitle).getBool() ),
							 QCLuceneField::STORE_YES | QCLuceneField::INDEX_UNTOKENIZED ) );
	ld.add(new QCLuceneField(QLatin1String("alias"), QString::number( item.getValue( AttrItemAlias ).isOid() ),
							 QCLuceneField::STORE_YES | QCLuceneField::INDEX_UNTOKENIZED ) );
	if( !text.isEmpty() )
	{
		ld.add(new QCLuceneField(QLatin1String("content"), text, QCLuceneField::INDEX_TOKENIZED) );
		count++;
	}
	if( !id.isEmpty() )
	{
		ld.add(new QCLuceneField(QLatin1String("ident"), id, QCLuceneField::INDEX_TOKENIZED) );
		ld.add(new QCLuceneField(QLatin1String("content"), id, QCLuceneField::INDEX_TOKENIZED) );
		count++;
	}
	if( count )
		w.addDocument( ld, a );
}

static int calcCount2( const Udb::Obj& o )
{
	int res = 0;
    Udb::Mit i = o.findCells( Udb::Obj::KeyList() );
    if( !i.isNull() ) do
	{
		Udb::Mit::KeyList k = i.getKey();
		if( !k.isEmpty() && k[0].isOid() )
			res++;
	}while( i.nextKey() );
	return res;
}

bool Indexer::hasPendingUpdates() const
{
	Udb::Mit i = d_pending.findCells( Udb::Obj::KeyList() );
	Udb::Mit::KeyList k = i.getKey();
	return !k.isEmpty() && k[0].isOid();
}

bool Indexer::indexIncrements( QWidget* parent )
{
	d_error.clear();
	QString path = getIndexPath();
	try
	{
		QApplication::setOverrideCursor( Qt::WaitCursor );
		QApplication::processEvents();

		const int count = calcCount2( d_pending ) * 2; // je einmal delete und dann index
		if( count == -1 )
		{
			QApplication::restoreOverrideCursor();
			return false;
		}
		QApplication::processEvents();
		QProgressDialog progress( tr("Indexing repository..."), tr("Abort"), 0, count, parent );
		progress.setAutoClose( false );
		// progress.setMinimumDuration( 0 );
		progress.setWindowTitle( tr( "CrossLine Search" ) );
		progress.setWindowModality(Qt::WindowModal);
		progress.setAutoClose( true );

        QList< QPair<Udb::OID,bool> > toHandle;
        QList<Udb::Mit::KeyList> toRemove;
		{ // Remove outdated documents
			QCLuceneIndexReader r = QCLuceneIndexReader::open( path );
			Udb::Mit mit = d_pending.findCells( Udb::Obj::KeyList() );
			if( !mit.isNull() ) do
			{
				Udb::Mit::KeyList k = mit.getKey();
                if( k.size() == 1 && k[0].isOid() )
				{
					r.deleteDocuments(QCLuceneTerm(QLatin1String("oid"),
                                                   QString::number( k[0].getOid(), 16 ) ) );
					toHandle.append( qMakePair(k[0].getOid(), mit.getValue().getBool() ) );
                }else
                {
                    toRemove.append( k );
                }
			}while( mit.nextKey() );
			r.close();
		}
		{ // Reindex
			QCLuceneStandardAnalyzer a;
			QCLuceneIndexWriter w( path, a, false );
			w.setMinMergeDocs( 1000 );
			w.setMaxBufferedDocs( 100 );

            Udb::Mit::KeyList k(1);
            for( int i = 0; i < toHandle.size(); i++ )
			{
                const QPair<Udb::OID,bool>& p = toHandle[i];
                if( p.second )
                {
                    Udb::Obj o = d_pending.getObject( p.first );
                    if( !o.isNull() )
                    {
                        // Text updated
                        Udb::Obj home = o.getValueAsObj( AttrItemHome );
						if( home.isNull() )
							indexItem( o, o, w, a );
						else
							indexItem( o, home, w, a );
                    }
                    //qDebug() << "FullTextIndexer::indexIncrements: indexing document" << p.first;
                }
                k[0].setOid( p.first );
                d_pending.setCell( k, Stream::DataCell().setNull() );
                //qDebug() << "FullTextIndexer::indexIncrements: remove pending" << p.first;
                progress.setValue( progress.value() + 1 );
                if( progress.wasCanceled() )
				{
					w.close();
					QApplication::restoreOverrideCursor();
					d_pending.getTxn()->rollback();
					return false;
				}
			}
            foreach( const Udb::Mit::KeyList& kl, toRemove )
                d_pending.setCell( kl, Stream::DataCell().setNull() );

		}
		progress.setValue( count );
		QApplication::restoreOverrideCursor();
		d_pending.commit();
		return true;
	}catch( CLuceneError& e )
	{
		QApplication::restoreOverrideCursor();
		d_error = QString::fromLatin1( e._awhat );
		d_pending.getTxn()->rollback();
		return false;
	}
}

static void deletePendings( Udb::Obj& o )
{
    Udb::Mit i = o.findCells( Udb::Obj::KeyList() );
    if( !i.isNull() ) do
	{
		Udb::Mit::KeyList k = i.getKey();
		if( !k.isEmpty() && k[0].isOid() )
			o.setCell( k, Stream::DataCell().setNull() );
	}while( i.nextKey() );
}

bool Indexer::indexRepository( QWidget* parent, const Udb::Obj &root )
{
	d_error.clear();
	QString path = getIndexPath();
	try
	{
		QApplication::setOverrideCursor( Qt::WaitCursor );
		QApplication::processEvents();
		QCLuceneStandardAnalyzer a;
		QCLuceneIndexWriter w( path, a, true );
		w.setMinMergeDocs( 1000 );
		w.setMaxBufferedDocs( 100 );

		QApplication::processEvents();
		QProgressDialog progress( tr("Indexing repository..."), tr("Abort"), 0,
			d_pending.getDb()->getMaxOid(), parent );
		progress.setAutoClose( false );
		progress.setMinimumDuration( 0 );
		progress.setWindowTitle( tr( "CrossLine Search" ) );
		progress.setWindowModality(Qt::WindowModal);
		progress.setAutoClose( true );

		Udb::Extent e( d_pending.getTxn() );
		if( e.first() ) do
		{
			Udb::Obj obj = e.getObj();
			Udb::Obj home = obj.getValueAsObj( AttrItemHome );
			if( home.isNull() )
				indexItem( obj, obj, w, a );
			else
				indexItem( obj, home, w, a );
			progress.setValue( obj.getOid() );
			if( progress.wasCanceled() )
			{
				w.close();
				QDir dir( path );
				QStringList files = dir.entryList( QDir::Files );
				for( int i = 0; i < files.size(); i++ )
					dir.remove( files[i] );
				QApplication::restoreOverrideCursor();
				return false;
			}
		}while( e.next() );
		progress.setValue( d_pending.getDb()->getMaxOid() );
		// Bei vollem Index (z.B. bei Rebuild) macht es keinen Sinn, die Pendings zu behalten
		deletePendings(d_pending);
		d_pending.commit();
		QApplication::restoreOverrideCursor();
		return true;
	}catch( CLuceneError& e )
	{
		QApplication::restoreOverrideCursor();
		d_error = QString::fromLatin1( e._awhat );
		d_pending.getTxn()->rollback();
		return false;
	}
}

bool Indexer::query( const QString& query, ResultList& result )
{
	d_error.clear();
	QString path = getIndexPath();
	if( !QCLuceneIndexReader::indexExists( path ) )
	{
		d_error = QLatin1String( "Lucene: " ) + tr("index does not exist!");
		return false;
	}
	try
	{
		QCLuceneStandardAnalyzer a;
		QCLuceneQuery* q = QCLuceneQueryParser::parse( query, "content", a );
        // mit "[]" gibt es hier crash, der von keinem catch aufgehalten wird. Ursache vermutlich in
        // CLucene\config\gunichartables.cpp cl_tolower
		if( q )
		{
			result.clear();
			QCLuceneIndexSearcher s( path );
			QCLuceneHits hits = s.search( *q );
			QApplication::setOverrideCursor( Qt::WaitCursor );
			for( int i = 0; i < hits.length(); i++ )
			{
				QCLuceneDocument doc = hits.document( i );
				Hit hit;
				hit.d_score = hits.score( i );
				hit.d_doc = d_pending.getObject( doc.get( "doc" ).toULongLong( 0, 16 ) );
				hit.d_item = d_pending.getObject( doc.get( "oid" ).toULongLong( 0, 16 ) );
				hit.d_title = doc.get( "title" ).toInt() != 0;
				hit.d_alias = doc.get( "alias" ).toInt() != 0;
				if( hit.d_doc.isNull() || hit.d_item.isNull() )
					continue;
				result.append( hit );
			}
			QApplication::restoreOverrideCursor();
			delete q;
			return true;
		}else
		{
			d_error = QLatin1String( "Lucene: " ) + tr("invalid query!");
			return false;
		}
	}catch( CLuceneError& e )
	{
		QApplication::restoreOverrideCursor();
		d_error = QLatin1String( "Lucene: " ) + QString::fromLatin1( e._awhat );
		return false;
    }catch( std::exception& e )
    {
        QApplication::restoreOverrideCursor();
        d_error = QLatin1String( "Lucene: " ) + QString::fromLatin1( e.what() );
		return false;
    }catch( ... )
    {
        QApplication::restoreOverrideCursor();
        d_error = QLatin1String( "Lucene: unknown internal error" );
		return false;
    }
}

void Indexer::onDbUpdate( Udb::UpdateInfo info )
{
	if( info.d_kind != Udb::UpdateInfo::PreCommit )
		return;

	// mache hier eine richtige Kopie da durch die vorliegende Funktion die Notification List
	// ergänzt wird.
	QList<Udb::UpdateInfo> updates = d_pending.getTxn()->getPendingNotifications();
	Udb::Obj::KeyList k(1);
	for( int i = 0; i < updates.size(); i++ )
	{
		const Udb::UpdateInfo& upd = updates[i];
		if( upd.d_kind == Udb::UpdateInfo::ValueChanged )
		{
			if( upd.d_name == AttrText || upd.d_name == Udb::ContentObject::AttrAltIdent || upd.d_name == AttrIdent )
			{
				k[0].setOid( upd.d_id );
				if( d_pending.getCell( k ).isNull() )
					d_pending.setCell( k, Stream::DataCell().setBool( true ) );
				// NOTE: kein commit, da in Pre-Commit der Transaction, wo die Änderung stattfand
				//qDebug() << "FullTextIndexer::onDbUpdate:" << upd.toString() << HeTypeDefs::prettyName( upd.d_name );
			}
		}else if( upd.d_kind == Udb::UpdateInfo::ObjectErased )
		{
			k[0].setOid( upd.d_id );
			d_pending.setCell( k, Stream::DataCell().setBool( false ) );
			// NOTE: kein commit, da in Pre-Commit der Transaction, wo die Änderung stattfand
			//qDebug() << "FullTextIndexer::onDbUpdate:" << upd.toString() << HeTypeDefs::prettyName( upd.d_name );
		}
	}
}
