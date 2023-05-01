#ifndef __Oln_EccoToOutline__
#define __Oln_EccoToOutline__

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

#include <Udb/Transaction.h>
#include <QDir>

namespace Oln
{
	class EccoToOutline
	{
	public:
		EccoToOutline(){}

		bool parse( QIODevice* ecco, Udb::Obj& trace ); 
		const QString& getError() const { return d_error; }
	protected:
		Udb::Obj createOutline( const Udb::Obj& trace );
		Udb::Obj createItem(const Udb::Obj& outline);
	private:
		QString d_error;
	};
}

#endif
