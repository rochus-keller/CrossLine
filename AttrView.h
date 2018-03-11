#ifndef ATTRVIEW_H
#define ATTRVIEW_H

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
#include <Oln2/OutlineMdl.h>
#include <Udb/UpdateInfo.h>

namespace Oln
{
	class OutlineTree;
}
namespace Oln
{
	class AttrViewMdl;

	class AttrView : public QWidget
	{
		Q_OBJECT
	public:
		explicit AttrView(QWidget *parent, Udb::Transaction *txn);
		void setObj( const Udb::Obj& );
	signals:
		void signalFollowObject( const Udb::Obj& );
	protected slots:
		void onAnchorActivated(QByteArray data,bool url);
		void onDbUpdate( Udb::UpdateInfo info );
	private:
		AttrViewMdl* d_mdl;
		Oln::OutlineTree* d_tree;

	};
}

#endif // ATTRVIEW_H
