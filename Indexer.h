#ifndef _Ds_DocIndex_
#define _Ds_DocIndex_

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
#include <QList>
#include <QMutex>
#include <Udb/UpdateInfo.h>

class QWidget;

namespace Oln
{
	class Indexer : public QObject
	{
		Q_OBJECT
	public:
		static const char* s_pendingUuid;
		struct Hit
		{
			Udb::Obj d_doc;
			Udb::Obj d_item;
			bool d_title;
			bool d_alias;
			qreal d_score;
		};
		typedef QList<Hit> ResultList;

		static QString fetchText( const Udb::Obj&, quint32 atom ); // not simplified, original case
		static Udb::Obj findText( const QString& pattern, const Udb::Obj& start, bool forward = true ); 
		static Udb::Obj gotoNext( const Udb::Obj& obj );
		static Udb::Obj gotoPrev( const Udb::Obj& obj );
		static Udb::Obj gotoLast( const Udb::Obj& obj ); // zuunterst

		Indexer( Udb::Transaction*, QObject*  );
		bool exists();
		bool hasPendingUpdates() const;
		bool indexRepository( QWidget*, const Udb::Obj& root ); // Blocking
		bool indexIncrements( QWidget* ); // Blocking
		const QString& getError() const { return d_error; }
		bool query( const QString& query, ResultList& result );
        QString getIndexPath() const;
	protected slots:
		void onDbUpdate( Udb::UpdateInfo );
	private:
		QString d_error;
		Udb::Obj d_pending;
	};
}

#endif
