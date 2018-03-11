#ifndef __Oln_DocSelector__
#define __Oln_DocSelector__

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
#include <QList>

class QAbstractItemModel;
class QListView;
class QModelIndex;

namespace Mp
{
	class DocSelector : public QWidget
	{
		Q_OBJECT
	public:
		DocSelector(const QList<QWidget*>& );
		~DocSelector();
	signals:
		void sigSelected( QWidget* );
	protected:
		void hideEvent ( QHideEvent * event );
	protected slots:
		void onClicked ( const QModelIndex & index );
	private:
		QAbstractItemModel* d_mdl;
		QListView* d_view;
	};
}

#endif
