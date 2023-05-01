#ifndef APPCONTEXT_H
#define APPCONTEXT_H

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

#include <QObject>
#include <Txt/Styles.h>
#include <QList>

class QWidget;
class QSettings;

namespace Oln
{
    class Repository;
    class Outliner;

	class AppContext : public QObject
	{
		Q_OBJECT

	public:
		static const char* s_company;
		static const char* s_domain;
		static const char* s_appName;
		static const char* s_rootUuid;
		static const char* s_version;
		static const char* s_date;
		static const char* s_extension;

		AppContext(QObject *parent = 0);
		~AppContext();

        bool open( QString path );
		static AppContext* inst();
		QSettings* getSet() const { return d_set; }
		void setDocFont( const QFont& );
    public slots:
        void onOpen( const QString& path );
    protected slots:
        void onCloseOutliner();
        void onHandleXoid( const QUrl& );
    protected:
        static QString parseFilePath( const QString& );
        static quint64 parseOid( const QString& );
	private:
		QSettings* d_set;
		Txt::Styles* d_styles;
        QList<Outliner*> d_outliner;
	};
}

#endif // APPCONTEXT_H
