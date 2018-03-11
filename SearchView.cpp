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

#include "SearchView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QShortcut>
#include <QLabel>
#include <QSettings>
#include <QResizeEvent>
#include <QHeaderView>
#include <Gui2/UiFunction.h>
#include "Indexer.h"
#include "AppContext.h"
#include "TypeDefs.h"
#include "Repository.h"
using namespace Oln;

static const int s_item = 0;
static const int s_doc = 1;
static const int s_date = 2;
static const int s_score = 3;

struct _SearchViewItem : public QTreeWidgetItem
{
	_SearchViewItem( QTreeWidget* w ):QTreeWidgetItem( w ) {}

	bool operator<(const QTreeWidgetItem &other) const
	{
		const int col = treeWidget()->sortColumn();
		switch( col )
		{
		case s_score:
			return text(s_score) + text(s_date) < 
				other.text(s_score) + other.text(s_date);
		case s_doc:
			return text(s_doc) + text(s_score) < other.text(s_doc) + other.text(s_score );
		case s_item:
			return text(s_item) + text(s_date) < other.text(s_item) + other.text(s_date);
		case s_date:
			return text(s_date) + text(s_score) < other.text(s_date) + other.text(s_score );
		}
		return text(col) < other.text(col);
	}
	QVariant data(int column, int role) const
	{
		if( role == Qt::ToolTipRole )
			role = Qt::DisplayRole;
		return QTreeWidgetItem::data( column, role );
	}
};

SearchView::SearchView(Repository * doc, QWidget * p):QWidget( p ), d_doc( doc )
{
	setWindowTitle( tr("CrossLine Search") );

	d_idx = new Indexer( d_doc->getTxn(), this );

	QVBoxLayout* vbox = new QVBoxLayout( this );
	vbox->setMargin( 0 );
	vbox->setSpacing( 1 );
	QHBoxLayout* hbox = new QHBoxLayout();
	hbox->setMargin( 1 );
	hbox->setSpacing( 1 );
	vbox->addLayout( hbox );

	hbox->addWidget( new QLabel( tr("Enter query:"), this ) );

	d_query = new QLineEdit( this );
	connect( d_query, SIGNAL( returnPressed() ), this, SLOT( onSearch() ) );
	hbox->addWidget( d_query );

	QPushButton* doit = new QPushButton( tr("&Search"), this );
	doit->setDefault(true);
	connect( doit, SIGNAL( clicked() ), this, SLOT( onSearch() ) );
	hbox->addWidget( doit );

	d_result = new QTreeWidget( this );
        d_result->setHeaderLabels( QStringList() << tr("Text") << tr("Document")
		<< tr("Date") << tr("Score") ); // s_item, s_doc, s_date, s_score
	d_result->header()->setStretchLastSection( false );
	d_result->setAllColumnsShowFocus( true );
	d_result->setRootIsDecorated( false );
	d_result->setSortingEnabled(true);
	QPalette pal = d_result->palette();
	pal.setColor( QPalette::AlternateBase, QColor::fromRgb( 245, 245, 245 ) );
	d_result->setPalette( pal );
	d_result->setAlternatingRowColors( true );
	connect( d_result, SIGNAL( itemActivated ( QTreeWidgetItem *, int ) ), this, SLOT( onGotoImp() ) );
	connect( d_result, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ), this, SLOT( onGotoImp() ) );
	vbox->addWidget( d_result );
}

SearchView::~SearchView()
{
}

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

void SearchView::onSearch()
{
	Indexer::ResultList res;
	if( !d_idx->exists() )
	{
		if( QMessageBox::question( this, tr("CrossLine Search"), 
			tr("The index does not yet exist. Do you want to build it? This will take some minutes." ),
			QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
			return;
		if( !d_idx->indexRepository( this, d_doc->getRoot() ) )
		{
			if( !d_idx->getError().isEmpty() )
				QMessageBox::critical( this, tr("CrossLine Indexer"), d_idx->getError() );
			return;
		}
	}else if( d_idx->hasPendingUpdates() )
	{
		// Immer aktualisieren ohne zu fragen
		if( true ) /*QMessageBox::question( this, tr("CrossLine Search"), 
			tr("The index is not up-do-date. Do you want to update it?" ),
			QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Ok )*/
		{
			if( !d_idx->indexIncrements( this ) )
			{
				if( !d_idx->getError().isEmpty() )
					QMessageBox::critical( this, tr("CrossLine Indexer"), d_idx->getError() );
				return;
			}
		}
	}
	QApplication::setOverrideCursor( Qt::WaitCursor );
	if( !d_idx->query( d_query->text(), res ) )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, tr("CrossLine Search"), d_idx->getError() );
		return;
	}
	d_result->clear();
	for( int i = 0; i < res.size(); i++ )
	{
		QTreeWidgetItem* item = new _SearchViewItem( d_result );
		item->setText( s_item, Indexer::fetchText( res[i].d_item, AttrText ) );
		item->setText( s_score, QString::number( res[i].d_score, 'f', 1 ) );
		item->setText( s_doc, Indexer::fetchText( res[i].d_doc, AttrText ) );
		item->setText( s_date, _valuta( res[i].d_doc ) );
		item->setData( s_item, Qt::UserRole, res[i].d_item.getOid() );
		item->setData( s_doc, Qt::UserRole, res[i].d_doc.getOid() );
		QFont f = item->font( s_item );
		if( res[i].d_title )
		{
			f.setBold(true);
			item->setFont( s_item, f );
		}
		if( res[i].d_alias )
		{
			f.setItalic(true);
			item->setFont( s_item, f );
		}
	}
    d_result->header()->setResizeMode( s_item, QHeaderView::Stretch );
	d_result->header()->setResizeMode( s_doc, QHeaderView::Interactive );
	d_result->header()->setResizeMode( s_date, QHeaderView::ResizeToContents );
	d_result->header()->setResizeMode( s_score, QHeaderView::ResizeToContents );
    d_result->resizeColumnToContents( s_date );
	d_result->resizeColumnToContents( s_score );
	d_result->sortByColumn( s_date, Qt::DescendingOrder );
    d_result->scrollToItem( d_result->topLevelItem( 0 ) );
	QApplication::restoreOverrideCursor();
}

void SearchView::onNew()
{
	d_query->selectAll();
	d_query->setFocus();
}

void SearchView::onGotoImp()
{
	QTreeWidgetItem* cur = d_result->currentItem();
	if( cur == 0 )
		return;

	emit sigShowItem( cur->data( s_item,Qt::UserRole).toULongLong() );
}

void SearchView::onGoto()
{
	ENABLED_IF( d_result->currentItem() );
	onGotoImp();
}

quint64 SearchView::getItem() const
{
	QTreeWidgetItem* cur = d_result->currentItem();
	if( cur == 0 )
		return 0;
	else
		return cur->data( s_item,Qt::UserRole).toULongLong();
}

void SearchView::onRebuildIndex()
{
	ENABLED_IF( true );

	if( !d_idx->indexRepository( this, d_doc->getRoot() ) )
	{
		if( !d_idx->getError().isEmpty() )
			QMessageBox::critical( this, tr("CrossLine Indexer"), d_idx->getError() );
		return;
	}
}

void SearchView::onUpdateIndex()
{
	ENABLED_IF( d_idx->exists() && d_idx->hasPendingUpdates() );

	if( !d_idx->indexIncrements( this ) )
	{
		if( !d_idx->getError().isEmpty() )
			QMessageBox::critical( this, tr("CrossLine Indexer"), d_idx->getError() );
	}
}

void SearchView::onClearSearch()
{
	ENABLED_IF( d_result->topLevelItemCount() > 0 );

	d_result->clear();
	d_query->clear();
	d_query->setFocus();
}
