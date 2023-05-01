#ifndef __TypeDefs__
#define __TypeDefs__

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

#include <QString> 

namespace Udb
{
	class Database;
	class Obj;
}

namespace Oln
{
	enum OlnNumbers
	{
		OlnStart = 0x20000,
		OlnMax = OlnStart + 27,
		OlnEnd = OlnStart + 1000 // Ab hier werden dynamische Atome angelegt
	};

	struct IndexDefs
	{
		static const char* IdxStartDate; // AttrStartDate
		static const char* IdxEndDate; // AttrEndDate
	};

	enum TypeDef_Object // abstract
	{
		AttrCreatedOn = OlnStart + 1, // DateTime
		AttrModifiedOn = OlnStart + 2, // DateTime
		//AttrName = OlnStart + 3, // String|RTXT, optional; anzeigbarer Titel des Objekts
		AttrText = OlnStart + 4, // String|HTML|RTXT: optional; anzeigbarer Titel oder Inhalt, je nach Objekt
		AttrSummary = OlnStart + 21, // String|HTML|RTXT: optionale Zusatzinformation, Abstrakt, ToolTip
		AttrIdent = OlnStart + 22, // String: optionaler human-readable Identifier, eindeutig pro Aggregat oder global für Toplevels
		AttrValuta = OlnStart + 23 // DateTime: optional, "in Kraft getreten am, freigegeben am, gültig ab"
	};

	// Outline Items bilden einen Baum als Aggregate-Children. Der Root des Outlines ist selber ein
	// Outline Item, das aber nicht explizit dargestellt wird
	enum TypeDef_OutlineItem // inherits Object
	{
		TypeOutlineItem = OlnStart + 10, 
		TypeOutline = OlnStart + 17,	
		AttrIdentCount = OlnStart + 26, // id32: wird nicht mehr gebraucht
		AttrItemIsExpanded = OlnStart + 11, // bool: offen oder geschlossen
		AttrItemIsTitle = OlnStart + 12, // bool: stellt das Item einen Title dar oder einen Text
		AttrItemIsReadOnly = OlnStart + 13, // bool: ist Item ein Fixtext oder kann es bearbeitet werden
		//AttrItemIsFixedSubs = OlnStart + 14, // bool: dem Item können keine Subs beigefügt oder entfernt werden
		//AttrItemIsFixedPos = OlnStart + 15, // bool: das Item kann nicht verschoben oder gelöscht werden.
		AttrItemHome = OlnStart + 16, // OID: Referenz auf Root des Outlines vom Typ TypeOutline
		AttrItemAlias = OlnStart + 18 // OID: optionale Referenz auf anderes Item, dessen Body statt des eigenen angezeigt wird.
	};

	enum TypeDef_Root
	{
		AttrRootDockList = OlnStart + 24, // BML (OID...OID), Liste aller OutlineDocks
		AttrRootTabList = OlnStart + 25, // BML (OID..OID), Liste aller geöffneten Tabs
		AttrAutoOpen = OlnStart + 27   // OID: optionale Referenz auf ein Outline, das beim Start geöffnet wird
	};

	struct TypeDefs
	{
		static void init( Udb::Database& db );
		static QString prettyTitle( const Udb::Obj&, bool withTime = true, bool fullInfo = false );
		static QString formatObjectTitle(const Udb::Obj &o, bool showId = true);
	};
}

#endif // __TypeDefs__
