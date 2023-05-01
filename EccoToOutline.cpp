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

#include "EccoToOutline.h"
#include "TypeDefs.h"
#include <QRegExp>
#include <QStack>
#include <cassert>
#include <QtDebug>
using namespace Oln;


Udb::Obj EccoToOutline::createOutline(const Udb::Obj& trace)
{
	Udb::Obj oln = trace.createObject( TypeOutline );
	oln.setValue( AttrCreatedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	oln.setValue( AttrItemIsReadOnly, Stream::DataCell().setBool( true ) ); // RISK
	return oln;
}

Udb::Obj EccoToOutline::createItem(const Udb::Obj& oln)
{
	Udb::Obj o = oln.createObject( TypeOutlineItem );
	o.setValue( AttrCreatedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	o.setValue( AttrItemHome, oln );
	o.setValue( AttrItemIsExpanded, Stream::DataCell().setBool( true ) ); // RISK
	return o;
}

bool EccoToOutline::parse( QIODevice* ecco, Udb::Obj& trace )
{

	int i;
	int level;
	QStack<Udb::Obj> obs;
	QRegExp r( "[0-9]{1,2}.[0-9]{1,2}.[0-9]{1,2} (([0-9]{1,2}:[0-9]{1,2})|([0-9]{4,4})|([0-9]{2,2}))" );
	while( ecco->bytesAvailable() )
	{
		QByteArray line = ecco->readLine();
		if( line.startsWith( "FINITO" ) )
			return true;
		i = 0;
		level = 0;
		while( i < line.size() && line[i] == '\t' )
		{
			i++;
			level++;
		}
		if( obs.size() > level )
			obs.resize( level );
		if( level == 0 )
		{
			obs.push( createOutline( trace ) );
			QString str = QString::fromLatin1( line.mid( i ) ).simplified();
			const int pos = r.indexIn( str );
			if( pos != -1 )
			{
				QString dstr = str.mid( pos, r.matchedLength() );
				// qDebug() << dstr;
				QDateTime dt = QDateTime::fromString( dstr, "d.M.yy h:m" );
				if( !dt.isValid() )
					dt = QDateTime::fromString( dstr, "d.M.yy hhmm" );
				if( !dt.isValid() )
					dt = QDateTime::fromString( dstr, "d.M.yy hh" );
				if( dt.isValid() )
				{
					dt = dt.addYears( 100 );
					// qDebug() << dt;
					obs.top().setValue( AttrValuta, Stream::DataCell().setDateTime( dt ) );
					str = str.left( pos ) + str.mid( pos + dstr.size() ).simplified();
					// qDebug() << str;
					obs.top().setValue( AttrText, Stream::DataCell().setString( str ) );
				}else
				{
					qWarning() << "invalid date " << dstr << " in " << line;
					obs.top().setValue( AttrText, Stream::DataCell().setString( str ) );
				}
			}else
				obs.top().setValue( AttrText, Stream::DataCell().setString( str ) );
			trace.appendSlot( obs.top() );
		}else
		{
			assert( obs.size() > 0 );
			Udb::Obj o = createItem( obs.first() );
			o.aggregateTo( obs.top() );
			obs.push( o );
			obs.top().setValue( AttrText, Stream::DataCell().setString( 
				QString::fromLatin1( line.mid( i ) ).simplified() ) );
		}

	}
	return true;
}
