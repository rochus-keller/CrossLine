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

#include "DocSelector.h"
#include <QKeyEvent>
#include <QtDebug>
#include <QPainter>
#include <QListView>
#include <QVBoxLayout>
#include <QAbstractListModel>
#include <QDesktopWidget>
#include <QApplication>

namespace Mp
{
class DocSelectorMdl : public QAbstractListModel
{
public:
	DocSelectorMdl( QObject* p, const QList<QWidget*>& l ):QAbstractListModel(p),d_order(l) 
	{
	}
	~DocSelectorMdl()
	{
	}
	QList<QWidget*> d_order;

    int rowCount ( const QModelIndex & = QModelIndex() ) const { return d_order.size(); }
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const
	{
		if( role == Qt::DisplayRole || role == Qt::ToolTipRole )
			return d_order[ d_order.size() - 1 - index.row() ]->objectName();
		else if( role == Qt::UserRole )
			return QVariant::fromValue( d_order[ d_order.size() - 1 - index.row() ] );
		return QVariant();
	}
};

class DocSelectorList : public QListView
{
public:
	DocSelectorList( QWidget* p ):QListView(p) {}
	void keyPressEvent ( QKeyEvent * event )
	{
		//qDebug() << "Keypress";
		if( event->key() == Qt::Key_Backtab && ( event->modifiers() & Qt::ControlModifier ) )
		{
			QModelIndex i = currentIndex();
			if( i.row() == 0  )
				setCurrentIndex( model()->index( model()->rowCount() - 1, 0 ) );
			else
				setCurrentIndex( model()->index( i.row() - 1, 0 ) );
		}else if( event->key() == Qt::Key_Tab && event->modifiers() == Qt::ControlModifier )
		{
			QModelIndex i = currentIndex();
			if( i.row() + 1 < model()->rowCount() )
				setCurrentIndex( model()->index( i.row() + 1, 0 ) );
			else
				setCurrentIndex( model()->index( 0, 0 ) );
		}else
			QListView::keyPressEvent( event );
	}
	void keyReleaseEvent ( QKeyEvent * event )
	{
		if( event->key() == Qt::Key_Control )
			parentWidget()->hide();
		else
			QListView::keyReleaseEvent( event );
	}
};

DocSelector::DocSelector(const QList<QWidget*>& l):QWidget( 0, Qt::Popup )
{
	d_mdl = new DocSelectorMdl( this, l );
	QVBoxLayout* box = new QVBoxLayout( this );
	box->setMargin(0);
	QListView* v = new DocSelectorList( this );
	d_view = v;
	v->setSelectionMode( QAbstractItemView::SingleSelection );
	v->setSelectionRectVisible( true );
	v->setFrameStyle( QFrame::Box | QFrame::Plain );
	v->setModel( d_mdl );
	v->setFocus();
	v->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	box->addWidget( v );
	connect( v, SIGNAL( clicked ( const QModelIndex & ) ), this, SLOT( onClicked ( const QModelIndex & ) ) );

	QDesktopWidget dw;
	QRect r = dw.screenGeometry();
	resize( qMax( int( r.width() * 0.2 ), 300 ), qMax( int( r.height() * 0.2 ), 200 ) );
    move( r.left() + r.width() / 2 - width() / 2, r.top() + r.height() / 2 - height() / 2 ); // ansonsten auf Linux links oben
	show();

	if( QApplication::keyboardModifiers() == Qt::ControlModifier )
		d_view->setCurrentIndex( d_mdl->index( 1, 0 ) );
	else if( QApplication::keyboardModifiers() == ( Qt::ControlModifier | Qt::ShiftModifier ) )
		d_view->setCurrentIndex( d_mdl->index( d_mdl->rowCount() - 1, 0 ) );
}

DocSelector::~DocSelector()
{
	//qDebug() << "Delete";
}

void DocSelector::hideEvent ( QHideEvent * event )
{
	QWidget::hideEvent( event );
	deleteLater();
	//qDebug() << "hide";
	if( d_view->currentIndex().isValid() )
		emit sigSelected( d_view->currentIndex().data( Qt::UserRole ).value<QWidget*>() );
}

void DocSelector::onClicked ( const QModelIndex & )
{
	hide();
}
}
