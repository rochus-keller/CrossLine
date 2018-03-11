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

#include "AttrView.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QDesktopServices>
#include <Oln2/OutlineUdbCtrl.h>
#include <Udb/Database.h>

namespace Oln
{
	class AttrViewMdl : public Oln::OutlineMdl
	{
	public:
		AttrViewMdl(QObject*p):OutlineMdl(p){}
		void setObject(const Udb::Obj& o)
		{
			if( d_obj.equals( o ) )
				return;
			clear();
			d_obj = o;
			Slot* root = new Slot();
			add( root, 0 );
			if( d_obj.isNull() )
				return;
			Udb::Obj::Names n = o.getNames();
			QMap<QString,quint32> dir;
			for( int i = 0; i < n.size(); i++ )
			{
				QString txt; // TODO = Funcs::getName( n[i], false, o.getTxn() );
				if( !txt.isEmpty() )
					dir[ txt ] = n[i];
			}
			QMap<QString,quint32>::const_iterator j;
			for( j = dir.begin(); j != dir.end(); ++j )
			{
				ObjSlot* s = new ObjSlot();
				s->d_name = j.value();
				add( s, root );
			}
			reset();
		}
		const Udb::Obj& getObj() const { return d_obj; }
	private:
		struct ObjSlot : public Slot
		{
		public:
			quint32 d_name;
			quint64 getId() const { return d_name; }
			QVariant getData(const OutlineMdl* m,int role) const
			{
				const AttrViewMdl* mdl = static_cast<const AttrViewMdl*>(m);
				if( role == Qt::DisplayRole )
				{
					QString html = "<html>";
					// TODO html += "<b>" + Funcs::getName( d_name, false, mdl->getObj().getTxn() ) + ":</b> ";
					// TODO html += Funcs::formatValue( d_name, mdl->getObj(), true );
					html += "</html>";
					return QVariant::fromValue( OutlineMdl::Html( html ) );
				}else
					return QVariant();
			}
		};
		Udb::Obj d_obj;
	};
}
using namespace Oln;


AttrView::AttrView(QWidget *parent, Udb::Transaction *txn) :
	QWidget(parent)
{
	QVBoxLayout* vbox = new QVBoxLayout( this );
	vbox->setMargin( 0 );
	vbox->setSpacing( 0 );

	d_mdl = new AttrViewMdl( this );

	d_tree = new Oln::OutlineTree( this );
	d_tree->setAlternatingRowColors( true );
	d_tree->setIndentation( 10 ); // RISK
	d_tree->setHandleWidth( 7 );
	d_tree->setShowNumbers( false );
	d_tree->setAcceptDrops( false );
	d_tree->setModel( d_mdl );
	Oln::OutlineCtrl* ctrl2 = new Oln::OutlineCtrl( d_tree );
	Oln::OutlineDeleg* deleg = ctrl2->getDeleg();
	deleg->setReadOnly(true);
	connect( deleg->getEditCtrl(), SIGNAL(anchorActivated(QByteArray,bool) ),
		this, SLOT( onAnchorActivated(QByteArray, bool) ) );
	vbox->addWidget( d_tree );

	updateGeometry();

	txn->getDb()->addObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );
}

void AttrView::setObj(const Udb::Obj & o)
{
	d_mdl->setObject( o );
}

void AttrView::onAnchorActivated(QByteArray data, bool url)
{
	if( url )
		QDesktopServices::openUrl( QUrl::fromEncoded( data ) );
	else
	{
		Oln::Link link;
		if( link.readFrom( data ) && d_mdl->getObj().getDb()->getDbUuid() == link.d_db )
			emit signalFollowObject( d_mdl->getObj().getObject( link.d_oid ) );
	}
}

void AttrView::onDbUpdate(Udb::UpdateInfo info)
{
	switch( info.d_kind )
	{
	case Udb::UpdateInfo::ObjectErased:
		if( info.d_id == d_mdl->getObj().getOid() )
			setObj( Udb::Obj() );
		break;
	}
}
