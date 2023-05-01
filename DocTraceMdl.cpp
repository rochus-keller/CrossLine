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

#include "DocTraceMdl.h"
#include <QtDebug>
#include <cassert>
#include "TypeDefs.h"
using namespace Oln;

QVariant DocTraceMdl::data( const Udb::Obj& o, int role ) const
{
	assert( !o.isNull() );
	switch( role )
	{
	case Qt::DisplayRole:
		return TypeDefs::prettyTitle( o );
	case Qt::ToolTipRole:
		return TypeDefs::prettyTitle( o, true, true );
    case Qt::SizeHintRole:
        return QSize(-1,static_cast<QWidget*>(QObject::parent())->fontMetrics().height() * 1.3 );
	}
	return QVariant();
}
