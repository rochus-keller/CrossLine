#ifndef REPOSITORY_H
#define REPOSITORY_H

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

#include <QObject>
#include <Udb/Database.h>
#include <Udb/Transaction.h>

namespace Oln
{
    class Repository : public QObject
    {
    public:
        explicit Repository(QObject *parent = 0);
        ~Repository();

        bool open( QString path );
        Udb::Transaction* getTxn() const { return d_txn; }
		Udb::Database* getDb() const;
		const Udb::Obj& getRoot() const { return d_root; }
    private:
        Udb::Database* d_db;
		Udb::Transaction* d_txn;
		Udb::Obj d_root;
		// Root ist die Queue, in der alle Outlines chronologisch referenziert sind, und ebenso der Root des DocTrees.
    };
}

#endif // REPOSITORY_H
