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

#include "SearchView2.h"
#include "TypeDefs.h"
#include "Outliner.h"
#include "Repository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QShortcut>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QResizeEvent>
#include <QHeaderView>
#include <QProgressDialog>
#include <QFileInfo>
#include <QDir>
#include <QMimeData>
#include <GuiTools/UiFunction.h>
#include <Oln2/OutlineUdbMdl.h>
#include <Oln2/OutlineItem.h>
#include <Udb/Transaction.h>
#include <Udb/Extent.h>
#include <Udb/Database.h>
#include <Fts/IndexEngine.h>
#include <Fts/Tokenizer.h>
#include <Fts/Stemmer.h>
#include <Fts/Stopper.h>
using namespace Oln;

static const QUuid s_index = "{aa4374d8-ce71-4489-8a17-fd16b932dd28}";

enum Cols { _Item, _Date, _Score };

static QString _valuta( const Udb::Obj& o )
{
	Stream::DataCell v = o.getValue( AttrValuta );
	if( !v.isDateTime() )
		v = o.getValue( AttrCreatedOn );
	if( v.isDateTime() )
		return v.getDateTime().toString( "yyMMdd-hhmm " );
	else
		return QString();
}
static QString _scoreStr( const QVariant& v )
{
	return QString("%1").arg( v.toInt(), 8, 10, QLatin1Char('0') );
}

struct _SearchView2Item : public QTreeWidgetItem
{
	Oln::OutlineItem d_item;
	_SearchView2Item( const Udb::Obj& item, QTreeWidget* w ):QTreeWidgetItem( w ),d_item(item) {}
	_SearchView2Item( const Udb::Obj& item, QTreeWidgetItem* w ):QTreeWidgetItem( w ),d_item(item) {}

	bool operator<(const QTreeWidgetItem &other) const
	{
		const int col = treeWidget()->sortColumn();
		switch( col )
		{
		case _Score:
			return _scoreStr(data(_Score,Qt::DisplayRole)) + text(_Date) <
					_scoreStr(other.data(_Score,Qt::DisplayRole)) + other.text(_Date);
		case _Date:
			return text(_Date) + _scoreStr(data(_Score,Qt::DisplayRole)) <
					other.text(_Date) + _scoreStr(other.data(_Score,Qt::DisplayRole ));
		}
		return text(col) < other.text(col);
	}
	QVariant data(int column, int role) const
	{
		if( role == Qt::ToolTipRole || role == Qt::DisplayRole )
		{
			switch( column )
			{
			case _Item:
				if( role == Qt::ToolTipRole )
					return d_item.getString( AttrText );
				else
					return TypeDefs::formatObjectTitle( d_item );
//			case _Date:
//				if( d_item.getType() != Oln::OutlineItem::TID )
//					return _valuta( d_item );
//				else
//					return QVariant();
			}
		}else if( role == Qt::TextAlignmentRole && column == _Score )
			return Qt::AlignRight;
		else if( role == Qt::DecorationRole && column == _Item )
		{
			return Oln::OutlineUdbMdl::getPixmap( d_item.getType() );
		}// else
		return QTreeWidgetItem::data( column, role );
	}
};

static Udb::Obj _getDocument(const Udb::Obj & o)
{
	if( o.getType() == Oln::OutlineItem::TID )
		return o.getValueAsObj( Oln::OutlineItem::AttrHome );
	else
		return Udb::Obj();
}

#define _separate_index_file_

SearchView2::SearchView2(Outliner *parent) :
	QWidget(parent),d_oln(parent)
{
	Udb::Transaction* txnDb = d_oln->getDoc()->getTxn();
	Udb::Obj index;
#ifdef _separate_index_file_
	try
	{
		Udb::Database* db = new Udb::Database( this );
		const QString path = getIndexPath( txnDb );
		db->open( path );
		db->setCacheSize( 10000 ); // RISK
		Udb::Transaction* txn2 = new Udb::Transaction( db, this );
		txn2->setIndividualNotify(false); // RISK
		index = txn2->getObject( s_index );
		if( index.isNull() )
		{
			index = txn2->createObject(s_index);
			txn2->commit();
		}
	}catch( std::exception& e )
	{
		QMessageBox::critical( 0, tr("Create/Open Index"), tr("Error: %1").arg( e.what() ) );
	}
#else
	index = txnDb->getOrCreateObject( s_index );
	txnDb->commit();
#endif
	Fts::IndexEngine::s_getDocument = _getDocument;
	d_idx = new Fts::IndexEngine(index, txnDb, this);
	d_idx->setTokenizer( new Fts::LetterOrNumberTok( d_idx ) );
	d_idx->setStemmer( new Fts::GermanStemmer( d_idx ) );
	d_idx->setStopper( new Fts::GermanStopper( d_idx ) );
	d_idx->addAttrToWatch( Udb::ContentObject::AttrText );
	d_idx->addAttrToWatch( Udb::ContentObject::AttrIdent );
	// d_idx->useReverseIndex(true);
	d_idx->resolveDocuments(true);

	QVBoxLayout* vbox = new QVBoxLayout( this );
	vbox->setMargin( 0 );
	vbox->setSpacing( 2 );
	QHBoxLayout* hbox = new QHBoxLayout();
	hbox->setMargin( 0 );
	hbox->setSpacing( 2 );
	vbox->addLayout( hbox );

	hbox->addWidget( new QLabel( tr("Query:"), this ) );

	d_query = new QLineEdit( this );
	connect( d_query, SIGNAL( returnPressed() ), this, SLOT( doSearch() ) );
	hbox->addWidget( d_query );

	d_fullMatch = new QCheckBox( tr("Full Match"), this );
	d_fullMatch->setToolTip( "Die Treffer muessen vollstaendig mit den Suchbegriffen uebereinstimmen");
	connect( d_fullMatch, SIGNAL(toggled(bool)),this,SLOT(doSearch()) );
	hbox->addWidget( d_fullMatch );

	d_docAnd = new QCheckBox( tr("Doc. AND"), this );
	d_docAnd->setChecked(true);
	connect( d_docAnd, SIGNAL(toggled(bool)),this,SLOT(doSearch()) );
	hbox->addWidget( d_docAnd );

	d_itemAnd = new QCheckBox( tr("Item AND"), this );
	d_itemAnd->setChecked(true);
	connect( d_itemAnd, SIGNAL(toggled(bool)),this,SLOT(doSearch()) );
	hbox->addWidget( d_itemAnd );

	d_curDoc = new QCheckBox( tr("Curr. Doc."), this );
	d_curDoc->setToolTip( tr("Selection only includes item of the current outline") );
	d_curDoc->setChecked(false);
	connect( d_curDoc, SIGNAL(toggled(bool)),this,SLOT(doSearch()) );
	hbox->addWidget( d_curDoc );

	QPushButton* doit = new QPushButton( tr("&Suchen"), this );
	doit->setDefault(true);
	connect( doit, SIGNAL( clicked() ), this, SLOT( doSearch() ) );
	hbox->addWidget( doit );

	d_result = new QTreeWidget( this );
	d_result->header()->setStretchLastSection( false );
	d_result->setAllColumnsShowFocus( true );
	d_result->setRootIsDecorated( true );
	d_result->setSortingEnabled(true);
	d_result->sortByColumn( _Date, Qt::DescendingOrder );
	d_result->setExpandsOnDoubleClick(false);
	d_result->setHeaderLabels( QStringList() << tr("Outline/Items") << tr("Valid") << tr("Hits") );
	d_result->setAlternatingRowColors( true );
    d_result->header()->setSectionResizeMode( _Item, QHeaderView::Stretch );
	d_result->setColumnWidth( _Date, 80 );
	d_result->setColumnWidth( _Score, 40 );
	connect( d_result, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ), this, SLOT( doGoto() ) );
	vbox->addWidget( d_result );
}

Udb::Obj SearchView2::getItem() const
{
	QTreeWidgetItem* cur = d_result->currentItem();
	if( cur == 0 )
		return Udb::Obj();
	else
		return d_idx->getTxn()->getObject( cur->data( 0,Qt::UserRole).toULongLong() );
}

QStringList SearchView2::tokenize(const QString & s)
{
	QStringList res;
	QString str = s.simplified();
	int start = 0;
	enum Status { Idle, InString } status = Idle;
	const QChar star('*');
	const QChar shout('!');
	for( int i = 0; i < str.size(); i++ )
	{
		const QChar ch = str[i];
		switch( status )
		{
		case Idle:
			if( ch.isLetterOrNumber() || ch == star || ch == shout )
			{
				start = i;
				status = InString;
			}
			break;
		case InString:
			if( !ch.isLetterOrNumber() && ch != star && ch != shout )
			{
				res.append( str.mid(start, i - start ).trimmed() );
				status = Idle;
			}
			break;
		}
	}
	if( status == InString )
		res.append( str.mid(start) );
	// qDebug() << name << res;
	return res;
}

QString SearchView2::getIndexPath(Udb::Transaction *txn)
{
	Q_ASSERT( txn != 0 );
	return txn->getDb()->getFilePath() + QLatin1String( ".index" );
}

void SearchView2::doSearch()
{
	if( d_idx->isEmpty() )
	{
		if( QMessageBox::question( this, tr("CrossLine Search"),
			tr("The index is empty. Do you want to create it? This will take some minutes." ),
			QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
			return;
		if( !rebuildIndex() )
		{
			return;
		}
	}
	const QString title = tr("Checking Query Terms - CrossLine");
	const QChar star('*');
	const QChar shout('!');
	QStringList tokens = tokenize( d_query->text() );
	for( int i = 0; i < tokens.size(); i++ )
	{
		QString& t = tokens[i];
		if( d_fullMatch->isChecked() )
		{
			const int starCount = t.count( star );
			if( starCount > 1 || ( starCount != 0 && ! t.endsWith( star )) || t.contains(shout) || t == star )
			{
				QMessageBox::information( this, title, tr("Only one terminating wildcard per term supported.") );
				return;
			}
		}else
		{
			const int shoutCount = t.count( shout );
			if( shoutCount > 1 || ( shoutCount != 0 && ! t.endsWith( shout )) || t.contains(star) || t == shout )
			{
				QMessageBox::information( this, title, tr("Only one terminating exlamation mark per term supported.") );
				return;
			}
			t.replace( shout, star );
		}
	}

	Fts::IndexEngine::DocHits res = d_idx->find( tokens, d_docAnd->isChecked(), d_itemAnd->isChecked(),
												 true, !d_fullMatch->isChecked() );
	d_result->clear();

	for( int i = 0; i < res.size(); i++ )
	{
		Udb::Obj doc = d_idx->getTxn()->getObject( res[i].d_doc );
		if( !d_curDoc->isChecked() || d_oln->currentDoc().equals(doc) )
		{
			_SearchView2Item* p = new _SearchView2Item( doc, d_result );
			p->setData( _Score, Qt::DisplayRole, res[i].d_rank );
			p->setText( _Date, _valuta( doc ) );
            QSize s = p->sizeHint(0);
            s.setHeight( s.height() * 1.3);
            p->setSizeHint(0,s);
            foreach( const Fts::IndexEngine::ItemHit& h, res[i].d_items )
			{
				Udb::Obj o = d_idx->getTxn()->getObject( h.d_item );
				_SearchView2Item* i = new _SearchView2Item( o, p );
                i->setSizeHint( 0, s );
				i->setData( _Score, Qt::DisplayRole, h.d_rank );
			}
		}
	}
	d_result->expandAll();
}

void SearchView2::doNew()
{
	d_query->selectAll();
	d_query->setFocus();
}

void SearchView2::onRebuildIndex()
{
	ENABLED_IF(true);

	rebuildIndex();
}

void SearchView2::onClearSearch()
{
	ENABLED_IF( d_result->topLevelItemCount() > 0 );

	d_result->clear();
	d_query->clear();
	d_query->setFocus();
}

void SearchView2::onCopyRef()
{
	ENABLED_IF( d_result->currentItem() );
	_SearchView2Item* item = static_cast<_SearchView2Item*>( d_result->currentItem() );
	QMimeData* mimeData = new QMimeData();
	QList<Udb::Obj> objs;
	objs.append( item->d_item );
	Udb::Obj::writeObjectRefs( mimeData, objs );
}

void SearchView2::doGoto()
{
	QTreeWidgetItem* cur = d_result->currentItem();
	ENABLED_IF( cur );
	_SearchView2Item* item = static_cast<_SearchView2Item*>( d_result->currentItem() );
	if( !item->d_item.isNull() && !item->d_item.isErased() )
		emit sigFollow( item->d_item.getOid() );
}

bool SearchView2::rebuildIndex()
{
	d_idx->clearIndex();

	QProgressDialog progress( tr("Indexing repository..."), tr("Abort"), 0,
		d_idx->getTxn()->getDb()->getMaxOid(), this );
	progress.setMinimumDuration( 0 );
	progress.setWindowTitle( tr( "CrossLine Index" ) );
	progress.setWindowModality(Qt::WindowModal);
	progress.setAutoClose( true );

	Udb::Extent e( d_idx->getTxn() );
	if( e.first() ) do
	{
		Udb::Obj obj = e.getObj();
		d_idx->indexObject( obj, false );
		progress.setValue( obj.getOid() );
		QApplication::processEvents();
		if( progress.wasCanceled() )
		{
			d_idx->clearIndex();
			return false;
		}
	}while( e.next() );
	progress.setValue( d_idx->getTxn()->getDb()->getMaxOid() );
	d_idx->commit(true);
	// d_idx->test(); //  TEST
	return true;
}

void SearchView2::onTest()
{
#ifdef _DEBUG
	ENABLED_IF(true);
	d_idx->test();
#endif
}

void SearchView2::onCloseAll()
{
	ENABLED_IF(true);
	d_result->collapseAll();
}

void SearchView2::onOpenAll()
{
	ENABLED_IF(true);
	d_result->expandAll();
}
