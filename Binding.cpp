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

#include "Binding.h"
#include <Oln2/LuaBinding.h>
#include <Oln2/OutlineItem.h>
#include <Script2/QtValue.h>
#include <Script2/QtObject.h>
#include <Script/Engine2.h>
#include <Udb/Idx.h>
#include <Udb/Transaction.h>
#include "TypeDefs.h"
#include "Outliner.h"
#include "Repository.h"
using namespace Oln;
using namespace Udb;

struct _View
{
	static int getFilePath(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		lua_pushstring( L, oln->getDoc()->getDb()->getFilePath().toUtf8().data() );
		return 1;
	}
	static int showObj(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		Udb::ContentObject* obj = CoBin<Udb::ContentObject>::check( L, 2 );
		oln->gotoItem( *obj, true );
		return 0;
	}
	static int newOutline(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		bool interactive = false;
		if( lua_gettop(L) > 1 )
			interactive = lua_toboolean(L,2);
		Udb::LuaBinding::pushObject( L, oln->newOutline(interactive) );
		return 1;
	}
	static int getOutlineCount(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		lua_pushinteger( L, oln->getDoc()->getRoot().getLastSlot().getSlotNr() );
		return 1;
	}
	static int getOutline(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		lua_pushinteger( L, oln->getDoc()->getRoot().getLastSlot().getSlotNr() );
		const int nr = luaL_checkinteger( L, 2 );
		if( nr <= 0 )
			luaL_argerror( L, 2, "expecting index >= 1" );
		Udb::Qit q = oln->getDoc()->getRoot().getSlot( nr );
		if( !q.isNull() )
		{
			Udb::Obj o = oln->getDoc()->getTxn()->getObject( q.getValue() );
			Udb::LuaBinding::pushObject( L, o );
		}else
			lua_pushnil( L );
		return 1;
	}
	static int commit(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		oln->getDoc()->getTxn()->commit();
		return 0;
	}
	static int rollback(lua_State *L)
	{
		Outliner* oln = Lua::QtObject<Outliner>::check( L, 1 );
		oln->getDoc()->getTxn()->rollback();
		return 0;
	}
};

static const luaL_reg _View_reg[] =
{
	{ "newOutline", _View::newOutline },
	{ "getFilePath", _View::getFilePath },
	{ "getOutlineCount", _View::getOutlineCount },
	{ "getOutline", _View::getOutline },
	{ "showObj", _View::showObj },
	{ "commit", _View::commit },
	{ "rollback", _View::rollback },
	{ 0, 0 }
};

static int _getPrettyTitle(lua_State *L)
{
	Udb::ContentObject* obj = CoBin<Udb::ContentObject>::check( L, 1 );
	*Lua::QtValue<QString>::create(L) = TypeDefs::formatObjectTitle( *obj );
	return 1;
}

static int _erase(lua_State *L)
{
	Udb::ContentObject* obj = CoBin<Udb::ContentObject>::check( L, 1 );
	obj->erase();
	// *obj = Udb::Obj(); // Nein, es koennte ja danach Rollback stattfinden
	return 1;
}

static bool _isValidAggregate(quint32 parent, quint32 child)
{
	if( parent == Outline::TID || parent == OutlineItem::TID )
		return child == OutlineItem::TID;
	return false;
}

static int _moveTo(lua_State *L)
{
	Udb::ContentObject* obj = CoBin<Udb::ContentObject>::check( L, 1 );
	Udb::ContentObject* parent = CoBin<Udb::ContentObject>::check( L, 2 );
	Udb::ContentObject* before = CoBin<Udb::ContentObject>::cast( L, 3 );
	if( before && !before->getParent().equals( *parent ) )
		luaL_argerror( L, 3, "'before' must be a child of 'parent'" );
	if( !_isValidAggregate( parent->getType(), obj->getType() ) )
		luaL_argerror( L, 2, "invalid 'parent' for object" );
	if( before )
		obj->aggregateTo( *parent, *before );
	else
		obj->aggregateTo( *parent, Udb::Obj() );
	return 0;
}

static int getValuta(lua_State *L)
{
	Udb::ContentObject* obj = CoBin<Udb::ContentObject>::check( L, 1 );
	*Lua::QtValue<QDateTime>::create(L) = obj->getValue(AttrValuta).getDateTime();
	return 1;
}

static const luaL_reg _ContentObject_reg[] =
{
	{ "getValuta", getValuta },
	{ "getPrettyTitle", _getPrettyTitle },
	{ "delete", _erase },
	{ "moveTo", _moveTo },
	{ 0, 0 }
};

static void _pushObject(lua_State * L, const Obj& o )
{
	const Udb::Atom t = o.getType();
	if( o.isNull() )
		lua_pushnil(L);
	else if( t == Oln::OutlineItem::TID )
		CoBin<Oln::OutlineItem>::create( L, o );
	else if( t == Oln::Outline::TID )
		CoBin<Oln::Outline>::create( L, o );
	else
		luaL_error( L, "cannot create binding object for type ID %d", t );
}

void Binding::install(lua_State *L)
{
	Lua::StackTester test(L, 0 );
	Udb::LuaBinding::pushObject = _pushObject;
	CoBin<Udb::ContentObject>::addMethods( L, _ContentObject_reg );
	Lua::QtObject<Outliner,QMainWindow,Lua::NotCreatable>::install( L, "View", _View_reg );
	lua_createtable( L, 0, 0 ); // Views
	lua_createtable( L, 0, 0 ); // list
	lua_setfield( L, -2, "list" );
	lua_setfield( L, LUA_GLOBALSINDEX, "Views" );
}

void Binding::setCurrentObject(const Udb::Obj & o)
{
	lua_State* L = Lua::Engine2::getInst()->getCtx();
	Lua::StackTester test(L, 0 );
	lua_getfield( L, LUA_GLOBALSINDEX, "Views" );
	Udb::LuaBinding::pushObject( L, o );
	lua_setfield( L, -2, "hit" );
	lua_pop(L, 1); // Views
}

void Binding::addView(Outliner * oln)
{
	lua_State* L = Lua::Engine2::getInst()->getCtx();
	Lua::StackTester test(L, 0 );
	lua_getfield( L, LUA_GLOBALSINDEX, "Views" );
	lua_getfield( L, -1, "list" );
	lua_pushstring( L, oln->getDoc()->getDb()->getFilePath().toUtf8().constData() );
	Lua::QtObject<Outliner>::create( L, oln, false );
	lua_rawset( L, -3 );
	lua_pop(L, 2); // Views, list
}

void Binding::removeView(Outliner * oln)
{
	lua_State* L = Lua::Engine2::getInst()->getCtx();
	Lua::StackTester test(L, 0 );
	lua_getfield( L, LUA_GLOBALSINDEX, "Views" );
	lua_getfield( L, -1, "list" );
	lua_pushstring( L, oln->getDoc()->getDb()->getFilePath().toUtf8().constData() );
	lua_pushnil( L );
	lua_rawset( L, -3 );
	lua_pop(L, 2); // Views, list
}

void Binding::setTop(Outliner * oln)
{
	lua_State* L = Lua::Engine2::getInst()->getCtx();
	Lua::StackTester test(L, 0 );
	lua_getfield( L, LUA_GLOBALSINDEX, "Views" );
	Lua::QtObject<Outliner>::create( L, oln, false );
	lua_setfield( L, -2, "top" );
	lua_pop(L, 1); // Views
}


