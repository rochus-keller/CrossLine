#ifndef DocTraceMdl_H
#define DocTraceMdl_H

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

#include <Udb/InvQueueMdl.h>
#include <QWidget>

namespace Oln
{
	class DocTraceMdl : public Udb::InvQueueMdl
	{
	public:
        DocTraceMdl(QWidget *parent):InvQueueMdl( parent ) {}

		// Overrides
		QVariant data( const Udb::Obj&, int role ) const;
	};
}

#endif // DocTraceMdl_H
