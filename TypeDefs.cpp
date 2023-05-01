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

#include "TypeDefs.h"
#include <Udb/Database.h>
#include <Udb/Obj.h>
#include <Oln2/OutlineItem.h>
#include "DocTabWidget.h"
using namespace Oln;
using namespace Udb;

const char* IndexDefs::IdxStartDate = "IdxStartDate";
const char* IndexDefs::IdxEndDate = "IdxEndDate";

void TypeDefs::init( Database& db )
{
	OutlineItem::AliasIndex = "IdxItemAlias";

	Database::Lock lock( &db);

	db.presetAtom( "OlnStart + 1000", OlnEnd ); 

	if( db.findIndex( OutlineItem::AliasIndex ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrItemAlias ) );
		db.createIndex( OutlineItem::AliasIndex, def );
	}
	/*
	if( db.findIndex( IndexDefs::IdxStartDate ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrStartDate ) );
		db.createIndex( IndexDefs::IdxStartDate, def );
	}
	if( db.findIndex( IndexDefs::IdxEndDate ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrEndDate ) );
		db.createIndex( IndexDefs::IdxEndDate, def );
	}
	*/
	Oln::DocTabWidget::attrText = AttrText;
}

QString TypeDefs::prettyTitle( const Udb::Obj& o, bool withTime, bool fullInfo )
{
	QString name = o.getValue( AttrText ).toString();
	QString id = o.getString( AttrIdent );
	Stream::DataCell v = o.getValue( AttrValuta );
	if( id.isEmpty() )
	{
		if( v.isDateTime() )
		{
			if( withTime )
				return v.getDateTime().toString( "yyMMdd-hhmm " ) + name;
			else
				return v.getDateTime().toString( "yyyy-MM-dd " ) + name;
		}else
			return name;
	}else if( fullInfo )
	{
		if( v.isDateTime() )
		{
			if( withTime )
				return v.getDateTime().toString( "yyMMdd-hhmm " ) + id + QChar(' ') + name;
			else
				return v.getDateTime().toString( "yyyy-MM-dd " ) + id + QChar(' ') + name;
		}
	}
	return id + QChar(' ') + name;
}

QString TypeDefs::formatObjectTitle(const Obj &o, bool showId)
{
	if( o.isNull() )
		return QLatin1String("<null>");
	Udb::Obj resolvedObj = o.getValueAsObj( Oln::OutlineItem::AttrAlias );
	if( resolvedObj.isNull() )
		resolvedObj = o;
	QString id = resolvedObj.getString( Udb::ContentObject::AttrIdent, true );
	QString name = resolvedObj.getString( Udb::ContentObject::AttrText, true ).simplified();
	if( name.isEmpty() )
	{
		switch( resolvedObj.getType() )
		{
		case TypeOutlineItem:
			name = QLatin1String("Outline Item");
			break;
		case TypeOutline:
			name = QLatin1String("Outline");
			break;
		}
	}
	if( id.isEmpty() || !showId )
#ifdef _DEBUG_TEST
		return QString("%1%2 %3").arg( QLatin1String("#" ) ).
			arg( o.getOid(), 3, 10, QLatin1Char('0' ) ).
			arg( name );
#else
		return name;
#endif
	else
		return QString("%1 %2").arg( id ).arg( name );
}
