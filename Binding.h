#ifndef BINDING_H
#define BINDING_H

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

#include <Udb/Obj.h>

typedef struct lua_State lua_State;

namespace Oln
{
	class Outliner;

	class Binding
	{
	public:
		static void install(lua_State * L);
		static void setCurrentObject( const Udb::Obj& );
		static void addView( Outliner* );
		static void removeView( Outliner* );
		static void setTop( Outliner* );
	private:
		Binding() {}
	};
}

#endif // BINDING_H
