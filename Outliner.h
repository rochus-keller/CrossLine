#ifndef OUTLINER2_H
#define OUTLINER2_H

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

#include <QMainWindow>
#include <Udb/Obj.h>
#include <QStack>
#include <Gui2/AutoMenu.h>

class QModelIndex;
class QTreeView;
class QTabWidget;
class QToolButton;
class QMenu;
class QAction;

namespace Mp
{
	class DocTabWidget;
}
namespace Oln
{
	class OutlineUdbCtrl;
	class DocTraceMdl;
	class SearchView;
	class SearchView2;
	class RefByItemMdl;
    class Repository;

	class Outliner : public QMainWindow
	{
		Q_OBJECT
	public:
		Outliner(Repository*,QWidget *parent = 0);
		~Outliner();
		bool gotoItem( const Udb::Obj&, bool expand = false, bool checkDocks = true );
        Repository* getDoc() const { return d_doc; }
        void showOid(quint64 oid);
		Udb::Obj newOutline(bool showDlg = true);
		Udb::Obj currentDoc() const;
	signals:
        void closing();
	protected:
		void setCaption();
		void setupMenu();
		void setupItemMenu( Gui2::AutoMenu* );
		void setupTextMenu( Gui2::AutoMenu* );
		void setupNaviMenu( Gui2::AutoMenu* );
		void setupCopyMenu( Gui2::AutoMenu* );
		void setupTrace();
		void setupAliasList();
		void setupSearch();
		void setupSearch2();
		void setupTerminal();
		OutlineUdbCtrl* addOrShowTab( const Udb::Obj&, bool setCurrent = true, bool addNew = false );
		Udb::Obj getCurrentItem(bool includeRoot = false) const;
		Udb::Obj getCurrentDoc(bool includeRoot = false) const;
		void showAsDock( const Udb::Obj& );
		int dockIndex( const Udb::Obj& );
		void removeDock( const Udb::Obj& );
		void streamDocks();
		OutlineUdbCtrl* showInDocks( const Udb::Obj& oln, const Udb::Obj& item );
		void copyRef( Udb::Obj, bool docRef = true );
        static void toFullScreen( QMainWindow* );
		void addTopCommands( Gui2::AutoMenu* );
		void pushBack(const Udb::Obj & o);
		// Overrides
		void closeEvent ( QCloseEvent * event );
		void changeEvent ( QEvent * event ) ;
	protected slots:
		void onTabChanged();
		void onDockItemSelected();
		void onSetFont();
        void onSetAppFont();
        void onImportHtml();
		void onHistoryDblClck( const QModelIndex & index );
		void onAliasDblClck( const QModelIndex & index );
		void onOpenOutline();
		void onImportEcco();
		void onNewOutline();
		void onMoveToTop();
		void onDeleteOutlines();
		void onFollowAlias();
		void onGoBack();
		void onGoForward();
		void onCurrentItemChanged( quint64 );
		void onSearchItemActivated( quint64 );
		void onSearch();
		void onSearch2();
		void onAbout();
		void onImportStream();
		void onSetDocName();
		void onSetDocProps();
        void onDeleteDoc();
        void onShowInTrace();
		void onAddDock();
		void onRemoveDock();
		void onCopyDocRef();
		void onCopyDocRef2();
		void onCopyDocRef3();
		void onCopyRef();
		void onFullScreen();
		void onOpenFromDock();
		void onExportHtml();
		void onExportOutline();
		void onInsertOutline();
		void onOpenOid( QUrl url );
        void onFollowUrl( const QUrl& );
		void onRebuildBackRefs();
		void onAutoStart();
	private:
		QString d_lastPath;
		DocTraceMdl* d_docTrace;
		RefByItemMdl* d_aliasList;
		QTreeView* d_docTraceTree;
		QList<OutlineUdbCtrl*> d_docks;
		SearchView* d_search;
		SearchView2* d_sv2;
		Mp::DocTabWidget* d_tab;
		QList<Udb::OID> d_backHisto; // d_backHisto.last() ist aktuell angezeigtes Objekt
		QList<Udb::OID> d_forwardHisto;
		Repository* d_doc;
		quint16 d_pushBackLock;
        bool d_fullScreen;
	};
}

#endif // OUTLINER2_H
