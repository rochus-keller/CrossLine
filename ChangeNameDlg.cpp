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

#include "ChangeNameDlg.h"
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QCalendarWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <Oln2/OutlineItem.h>
#include "TypeDefs.h"
using namespace Oln;

ChangeNameDlg::ChangeNameDlg( QWidget* p ):QDialog( p )
{
	setWindowTitle( tr( "Document Properties" ) );

	QVBoxLayout* vbox = new QVBoxLayout(this);
	QFormLayout* grid = new QFormLayout();
	vbox->addLayout(grid);

	d_name = new QLineEdit( this );
	grid->addRow( tr("Document Name:"), d_name );
	d_id = new QLineEdit( this );
	grid->addRow( tr("ID:"), d_id );
	QHBoxLayout* hbox = new QHBoxLayout();
	hbox->setMargin( 0 );
	hbox->setSpacing( 2 );
	d_valuta = new QDateEdit( this );
	d_valuta->setCalendarPopup( true );
	d_valuta->setDisplayFormat( tr("yyyy-MM-dd") );
	d_valuta->calendarWidget()->setVerticalHeaderFormat( QCalendarWidget::ISOWeekNumbers );
	d_valuta->calendarWidget()->setFirstDayOfWeek( Qt::Monday );
	d_valuta->calendarWidget()->setGridVisible( true );
	hbox->addWidget( d_valuta );
	d_time = new QLineEdit( this );
	hbox->addWidget( d_time );
	hbox->addStretch();
	grid->addRow( tr("Valid:"), hbox );
	d_createdOn = new QLabel( this );
	grid->addRow( tr("Created on:"), d_createdOn );
	d_modifiedOn = new QLabel( this );
	grid->addRow( tr("Modified on:"), d_modifiedOn );

	vbox->addStretch();
	QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	vbox->addWidget( bb );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
	// setMinimumWidth( 400 );
}

bool ChangeNameDlg::edit( Udb::Obj& o )
{
	d_name->setText( o.getValue( AttrText ).toString() );
	d_id->setText( o.getValue( AttrIdent ).toString() );
	Stream::DataCell v = o.getValue( AttrValuta );
	if( v.isDateTime() )
	{
		d_valuta->setDate( v.getDateTime().date() );
		d_time->setText( v.getDateTime().time().toString("hh:mm") );
	}
	d_createdOn->setText( OutlineItem::formatTimeStamp( o.getValue( AttrCreatedOn ).getDateTime() ) );
	d_modifiedOn->setText( OutlineItem::formatTimeStamp( o.getValue( AttrModifiedOn ).getDateTime() ) );
	if( exec() == QDialog::Accepted )
	{
		o.setValue( AttrText, Stream::DataCell().setString( d_name->text() ) );
		o.setValue( AttrIdent,Stream::DataCell().setString( d_id->text() ) );
		o.setValue( AttrValuta, Stream::DataCell().setDateTime( 
			QDateTime( d_valuta->date(), QTime::fromString( d_time->text(), "h:mm" ) ) ) );
		o.setTimeStamp( AttrModifiedOn );
		return true;
	}else
		return false;
}

