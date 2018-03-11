#ifndef SEARCHVIEW2_H
#define SEARCHVIEW2_H

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

#include <QWidget>
#include <Udb/Obj.h>

class QTreeWidget;
class QLineEdit;
class QCheckBox;

namespace Fts
{
	class IndexEngine;
}
namespace Oln
{
	class Outliner;

	class SearchView2 : public QWidget
	{
		Q_OBJECT
	public:
		explicit SearchView2(Outliner *parent = 0);
		Udb::Obj getItem() const;
		void newSearch() { doNew(); }
		Fts::IndexEngine* getIdx() const { return d_idx; }
		static QStringList tokenize( const QString& );
		static QString getIndexPath(Udb::Transaction* txn);
	signals:
		void sigFollow( quint64 );
	public slots:
		void onRebuildIndex();
		void onClearSearch();
		void onCopyRef();
		void onTest();
		void onCloseAll();
		void onOpenAll();
	public slots:
		void doSearch();
		void doNew();
		void doGoto();
	protected:
		bool rebuildIndex();
	private:
		QCheckBox* d_fullMatch;
		QCheckBox* d_docAnd;
		QCheckBox* d_itemAnd;
		QCheckBox* d_curDoc;
		QLineEdit* d_query;
		Outliner* d_oln;
		QTreeWidget* d_result;
		Fts::IndexEngine* d_idx;
	};
}

#endif // SEARCHVIEW2_H
