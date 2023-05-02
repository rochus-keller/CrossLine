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

#include "Outliner.h"
#include "AppContext.h"
#include <Oln2/OutlineUdbCtrl.h>
#include <Oln2/HtmlToOutline.h>
#include <Oln2/OutlineStream.h>
#include <Oln2/OutlineUdbStream.h>
#include <Oln2/RefByItemMdl.h>
#include <GuiTools/AutoMenu.h>
#include <GuiTools/AutoShortcut.h>
#include <QFontDialog>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDesktopServices>
#include <QHeaderView>
#include <QToolButton>
#include <QClipboard>
#include <QIcon>
#include "TypeDefs.h"
#include "DocTraceMdl.h"
#include "EccoToOutline.h"
#ifdef _HAS_CLUCENE_
#include "SearchView.h"
#endif
#include "SearchView2.h"
#include "ChangeNameDlg.h"
#include <Oln2/OutlineToHtml.h>
#include <Oln2/OutlineItem.h>
#include <QMimeData>
#include <QTabWidget>
#include <QtDebug>
#include "DocTabWidget.h"
#ifdef _HAS_LUA_
#include <Script/Terminal2.h>
#include "Binding.h"
#endif
#include "Repository.h"
using namespace Oln;
using namespace Gui;

Outliner::Outliner(Repository *doc, QWidget *parent)
    : QMainWindow(parent), d_pushBackLock( 0 ), d_doc( doc ), d_show(Normal)
{
	Udb::ContentObject::AttrText = AttrText;
	Udb::ContentObject::AttrCreatedOn = AttrCreatedOn;
	Udb::ContentObject::AttrModifiedOn = AttrModifiedOn;
	Udb::ContentObject::AttrIdent = AttrIdent;
	// AttrAltIdent?
	OutlineItem::TID = TypeOutlineItem;
	OutlineItem::AttrIsTitle = AttrItemIsTitle;
	OutlineItem::AttrIsReadOnly = AttrItemIsReadOnly;
	OutlineItem::AttrIsExpanded = AttrItemIsExpanded;
	OutlineItem::AttrHome = AttrItemHome;
	OutlineItem::AttrAlias = AttrItemAlias;
	Outline::TID = TypeOutline;


    d_tab = new Oln::DocTabWidget( this, false );
	d_tab->setFocusPolicy(Qt::StrongFocus);
	d_tab->setCloserIcon( ":/CrossLine/Images/close.png" );
	setCentralWidget( d_tab );
	connect( d_tab, SIGNAL( currentChanged(int) ), this, SLOT( onTabChanged() ) );

	setDockNestingEnabled(true);
	setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );
	setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
	setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
	setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );

	setupTrace();
	setupSearch();
	setupSearch2();
	setupTerminal();

	Oln::OutlineUdbMdl::registerPixmap( TypeOutlineItem, QString( ":/CrossLine/Images/outline_item.png" ) );
	Oln::OutlineUdbMdl::registerPixmap( TypeOutline, QString( ":/CrossLine/Images/outline.png" ) );

	setupMenu();
	setCaption();
	setupAliasList();
	QDesktopServices::setUrlHandler( "oid", this, "onOpenOid" );

	Stream::DataReader r( d_doc->getRoot().getValue(AttrRootDockList) );
	while( r.nextToken() == Stream::DataReader::Slot )
		showAsDock( d_doc->getTxn()->getObject( r.readValue().getOid() ) );

	QVariant state = AppContext::inst()->getSet()->value( "MainFrame/State/" +
		d_doc->getDb()->getDbUuid().toString() ); // Da DB-individuelle Docks
	if( !state.isNull() )
		restoreState( state.toByteArray() );

    d_show = AppContext::inst()->getSet()->value( "MainFrame/Show" ).toInt();
    switch( d_show )
    {
    case Normal:
        {
            showNormal();
            QRect r = AppContext::inst()->getSet()->value( "MainFrame/Size" ).toRect();
            if( r.isValid() )
                setGeometry(r);
        }
        break;
    case Maximized:
        showMaximized();
        break;
    case FullScreen:
        toFullScreen( this );
        break;
    }

	new QShortcut( tr("CTRL+Q"), this, SLOT( close() ) );
    new Gui::AutoShortcut( tr("CTRL+TAB"), this, d_tab, SLOT(onDocSelect()) );
    new Gui::AutoShortcut( tr("CTRL+SHIFT+TAB"), this, d_tab, SLOT(onDocSelect()) );
    new Gui::AutoShortcut( tr("CTRL+W"), this, d_tab, SLOT(onCloseDoc()) );
    new Gui::AutoShortcut( tr("ALT+LEFT"),  this, this, SLOT(onGoBack()) );
    new Gui::AutoShortcut( tr("ALT+RIGHT"), this, this, SLOT(onGoForward()) );
    new Gui::AutoShortcut( tr("F11"), this, this, SLOT( onFullScreen() ) );
    new Gui::AutoShortcut( tr("CTRL+N"), this, this, SLOT( onNewOutline() ) );

	QApplication::restoreOverrideCursor();

#ifdef _HAS_LUA_
	Binding::addView( this );
#endif

	gotoItem( d_doc->getRoot().getValueAsObj(AttrAutoOpen) );
}

void Outliner::onOpenOid( QUrl url )
{
	// oid:<object number>@<db-uuid>
	Q_ASSERT( url.scheme() == "oid" );
	QStringList l = url.path().split( QChar('@') );
	quint64 id = 0;
	if( l.size() > 1 )
	{
		if( QUuid( l[1] ) != d_doc->getDb()->getDbUuid() )
			return;
	}
	if( !l.isEmpty() )
		id = l[0].toLong();
    gotoItem( d_doc->getTxn()->getObject( id ) );
}

void Outliner::onFollowUrl(const QUrl & url)
{
    if( url.scheme() == "file" )
    {
        // RISK: folgende Lösung ist temporär; ev. ein konfigurierbares Mapping einführen.
        QString path = url.toLocalFile();
        path.replace( QString("c:/users/rochus"), QString("/home/me"), Qt::CaseInsensitive );
        QFileInfo info( path );
        if( !info.exists() )
            QMessageBox::critical( this, tr("Open URL - CrossLine"), tr("File not found: %1").arg(path) );
        QDesktopServices::openUrl( QUrl::fromLocalFile( path ) );
    }else
		QDesktopServices::openUrl( url );
}

void Outliner::onRebuildBackRefs()
{
	ENABLED_IF(true);

	if( QMessageBox::warning( this, tr("Update Back Reference Index - CrossLine"),
		tr("This operation can take some Time. Do you want to continue?" ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::No )
		return;

	QApplication::setOverrideCursor( Qt::WaitCursor );
	OutlineItem::updateAllRefs( d_doc->getTxn() );
	QApplication::restoreOverrideCursor();
}

void Outliner::onAutoStart()
{
	Udb::Obj oln = getCurrentDoc( true );
	Udb::Obj root = d_doc->getRoot();
	CHECKED_IF( !oln.isNull(), root.getValueAsObj(AttrAutoOpen).equals(oln) );

	if( root.getValueAsObj(AttrAutoOpen).equals(oln) )
		root.clearValue(AttrAutoOpen);
	else
		root.setValue(AttrAutoOpen,oln);
	root.commit();
}

OutlineUdbCtrl *Outliner::addOrShowTab( const Udb::Obj& oln, bool setCurrent, bool addNew )
{
#ifdef _HAS_LUA_
	Binding::setCurrentObject( oln );
#endif
	int pos = d_tab->showDoc( oln );
	QWidget* w = 0;
	Oln::OutlineUdbCtrl* ctrl = 0;
	if( pos == -1 )
	{
		QApplication::setOverrideCursor( Qt::WaitCursor );
		ctrl = OutlineUdbCtrl::create( d_tab, d_doc->getTxn() );
		w = ctrl->getTree();
		connect( ctrl, SIGNAL( sigCurrentChanged( quint64 ) ), this, SLOT( onCurrentItemChanged( quint64 ) ) );
		connect( ctrl, SIGNAL(sigUrlActivated(QUrl)), this, SLOT(onFollowUrl(QUrl)) ); //, Qt::QueuedConnection );
        connect( ctrl, SIGNAL(sigLinkActivated(quint64)), this, SLOT(onSearchItemActivated(quint64)) );
		ctrl->setOutline( oln, setCurrent );
		if( addNew )
            ctrl->addItem();
		pos = d_tab->addDoc( w, oln, TypeDefs::prettyTitle( oln ) );
		QApplication::restoreOverrideCursor();
	}else
	{
		w = d_tab->widget( pos );
		const QObjectList& ol = w->children();
		for( int i = 0; i < ol.size(); i++ )
		{
			if( ctrl = dynamic_cast<OutlineUdbCtrl*>( ol[i] ) )
				break;
		}
	}
	if( w )
		w->setFocus();
	return ctrl;
}

Outliner::~Outliner()
{
#ifdef _HAS_LUA_
	Binding::removeView( this );
#endif
}

void Outliner::setupMenu()
{
	AutoMenu* pop = new AutoMenu( d_tab, true );

	pop->addCommand( tr("New Outline..."), this, SLOT(onNewOutline()), tr( "CTRL+N" ) );
	//pop->addCommand( tr("Insert Outline..."), this,  SLOT(onInsertOutline()), tr( "CTRL+SHIFT+N" ), true );
	pop->addCommand( tr("Delete Outline..."),  this, SLOT(onDeleteDoc()), tr("CTRL+SHIFT+D"), true );
	pop->addSeparator();

	AutoMenu* sub = new AutoMenu( tr("Item" ), pop );
	pop->addMenu( sub );
	setupItemMenu(sub);

	sub = new AutoMenu( tr("Text" ), pop );
	pop->addMenu( sub );
	setupTextMenu(sub);

	pop->addSeparator();
	setupCopyMenu(pop);

	pop->addSeparator();
	pop->addCommand( tr("Set Outline Properties..."), this, SLOT(onSetDocProps()),tr("ALT+CTRL+Return"), true );
	pop->addAutoCommand( tr("Outline locked"), SLOT(onDocReadOnly()) );
	pop->addCommand( tr("Open on Start"), this, SLOT(onAutoStart()) );
	pop->addSeparator();
	pop->addAutoCommand( tr("Expand all"),  SLOT(onExpandAll()), tr("CTRL+SHIFT+A"), true );
	pop->addAutoCommand( tr("Collapse all"),  SLOT(onCollapseAll()) );
	pop->addCommand( tr("Show in History"), this, SLOT(onShowInTrace()) );
	pop->addCommand( tr("Dock Outline"), this, SLOT(onAddDock()) );
	pop->addCommand( tr("Close Outline"),  d_tab, SLOT(onCloseDoc()), tr("CTRL+W"), true );
	pop->addSeparator();
	pop->addCommand( tr("Export Outline..."), this, SLOT(onExportOutline()) );
	pop->addCommand( tr("Export HTML..."), this, SLOT(onExportHtml()) );

	addTopCommands( pop );

	new AutoShortcut( this,  SLOT(onSave()), tr("CTRL+S") );
#ifdef _HAS_CLUCENE_
	new AutoShortcut( tr("CTRL+SHIFT+F"), this, this,  SLOT(onSearch()) );
#endif
	new AutoShortcut( tr("CTRL+F"), this, this,  SLOT(onSearch2()) );

}

void Outliner::setupItemMenu(AutoMenu * sub)
{
	sub->addAutoCommand( tr("Add next"),  SLOT(onAddNext()), tr("Return"), true );
	sub->addAutoCommand( tr("Add right"),  SLOT(onAddRight()), tr("CTRL+R"), true );
	sub->addAutoCommand( tr("Add left"),  SLOT(onAddLeft()), tr("CTRL+L"), true );
	sub->addAutoCommand( tr("Add TOC..."), SLOT(onInsertToc()) );
	sub->addAutoCommand( tr("Remove..."),  SLOT(onRemove()), tr("CTRL+D"), true );
	sub->addSeparator();
	sub->addAutoCommand( tr("Set Title"),  SLOT(onSetTitle()), tr("CTRL+T"), true );
	sub->addAutoCommand( tr("Set Readonly"),  SLOT(onReadOnly()) );
	sub->addAutoCommand( tr("Properties..."), SLOT(onEditProps()), tr("ALT+Return"), true );
	sub->addSeparator();
	sub->addAutoCommand( tr("Indent"),  SLOT(onIndent()), tr("TAB"), true );
	new AutoShortcut( d_tab,  SLOT(onIndent()), tr("SHIFT+RIGHT") );
	sub->addAutoCommand( tr("Unindent"),  SLOT(onUnindent()), tr("SHIFT+TAB"), true );
	new AutoShortcut( d_tab,  SLOT(onUnindent()), tr("SHIFT+LEFT") );
	sub->addAutoCommand( tr("Move up"),  SLOT(onMoveUp()), tr("SHIFT+UP"), true );
	sub->addAutoCommand( tr("Move down"),  SLOT(onMoveDown()), tr("SHIFT+DOWN"), true );
	sub->addSeparator();
	sub->addAutoCommand( tr("Expand selected"),  SLOT(onExpandSubs()) );
	sub->addAutoCommand( tr("Follow Alias"),  SLOT(onFollowAlias()) );
}

void Outliner::setupTextMenu(AutoMenu * sub)
{
	sub->addAutoCommand( tr("Bold"),  SLOT(onBold()), tr("CTRL+B"), true );
	sub->addAutoCommand( tr("Underline"),  SLOT(onUnderline()), tr("CTRL+U"), true );
	sub->addAutoCommand( tr("Italic"),  SLOT(onItalic()), tr("CTRL+I"), true );
	sub->addAutoCommand( tr("Strikeout"),  SLOT(onStrike()), tr("SHIFT+CTRL+U"), true );
	sub->addAutoCommand( tr("Fixed"),  SLOT(onFixed()), tr("SHIFT+CTRL+I"), true );
	sub->addAutoCommand( tr("Superscript"),  SLOT(onSuper()), tr("SHIFT+CTRL+P"), true );
	sub->addAutoCommand( tr("Subscript"),  SLOT(onSub()), tr("CTRL+P"), true );
	sub->addSeparator();
	sub->addAutoCommand( tr("Insert Timestamp"),  SLOT(onTimestamp()), tr("CTRL+M"), true );
	sub->addAutoCommand( tr("Insert Date"),  SLOT(onInsertDate()), tr("CTRL+SHIFT+M"), true );
	sub->addAutoCommand( tr("Strip Whitespace"),  SLOT(onStripWhitespace()), tr("CTRL+SHIFT+W"), true );
	sub->addSeparator();
	sub->addAutoCommand( tr("Edit Link..."),  SLOT(onEditUrl()), tr("CTRL+SHIFT+L"), true );
	sub->addAutoCommand( tr("Follow Link"),  SLOT(onOpenUrl()) );
}

void Outliner::setupNaviMenu(AutoMenu * sub)
{
	sub->addCommand( tr("Back"),  this, SLOT(onGoBack()), tr("ALT+LEFT") );
	sub->addCommand( tr("Forward"), this, SLOT(onGoForward()), tr("ALT+RIGHT") );
	sub->addSeparator();
	sub->addCommand( tr("Search Outlines..."),  this, SLOT(onSearch2()), tr("CTRL+F") );
}

void Outliner::setupCopyMenu(AutoMenu * pop)
{
	pop->addAutoCommand( tr("Cut"),  SLOT(onCut()), tr("CTRL+X"), true );
	pop->addAutoCommand( tr("Copy"),  SLOT(onCopy()), tr("CTRL+C"), true );
	pop->addCommand( tr("Copy Document Ref."), this, SLOT(onCopyDocRef()), tr("CTRL+SHIFT+C"), true );
	pop->addAutoCommand( tr("Paste"),  SLOT(onPaste()), tr("CTRL+V"), true );
	pop->addAutoCommand( tr("Paste Special"),  SLOT(onPasteSpecial()), tr("CTRL+SHIFT+V"), true );
}

static inline QDockWidget* createDock( QMainWindow* parent, const QString& title, quint32 id, bool visi )
{
	QDockWidget* dock = new QDockWidget( title, parent );
	if( id )
		dock->setObjectName( QString( "{%1" ).arg( id, 0, 16 ) );
	else
		dock->setObjectName( QChar('[') + title );
	dock->setAllowedAreas( Qt::AllDockWidgetAreas );
	dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
	dock->setFloating( false );
	dock->setVisible( visi );
	return dock;
}

static QString _formatTitle(const Udb::Obj & o )
{
	return TypeDefs::prettyTitle( o, true );
}

void Outliner::setupAliasList()
{
	Oln::RefByItemMdl::formatTitle = _formatTitle;
	QTreeView* tree = new QTreeView( this );
	QPalette pal = tree->palette();
	pal.setColor( QPalette::AlternateBase, QColor::fromRgb( 245, 245, 245 ) );
	tree->setPalette( pal );
	tree->setAlternatingRowColors( true );
	//tree->setRootIsDecorated( false );
	//tree->setUniformRowHeights( false );
	tree->setAllColumnsShowFocus(true);
	//tree->setWordWrap( true );
	//tree->setSelectionBehavior( QAbstractItemView::SelectRows );
	//tree->setSelectionMode( QAbstractItemView::ExtendedSelection );
	tree->header()->hide();

	d_aliasList = new RefByItemMdl( tree );
	// TODO d_doc->getDb()->addObserver( d_aliasList, SLOT(onDbUpdate( Udb::UpdateInfo )), false );

	tree->setModel( d_aliasList );

	QDockWidget* dock = createDock( this, tr("Referenced by" ), 0, false );
	dock->setWidget( tree );
	addDockWidget( Qt::RightDockWidgetArea, dock );

	connect( tree, SIGNAL( doubleClicked ( const QModelIndex & ) ), this, SLOT( onAliasDblClck( const QModelIndex & ) ) );
	connect( tree, SIGNAL( activated ( const QModelIndex & ) ), this, SLOT( onAliasDblClck( const QModelIndex & ) ) );
}

void Outliner::setupTrace()
{
	d_docTrace = new DocTraceMdl( this );
	d_doc->getDb()->addObserver( d_docTrace, SLOT(onDbUpdate( Udb::UpdateInfo )), false );
	d_docTrace->setQueue( d_doc->getRoot() );

	QTreeView* tree = new QTreeView( this );
	d_docTraceTree = tree;
	QPalette pal = tree->palette();
	pal.setColor( QPalette::AlternateBase, QColor::fromRgb( 245, 245, 245 ) );
	tree->setPalette( pal );
	tree->setAlternatingRowColors( true );
	tree->setRootIsDecorated( false );
    tree->setUniformRowHeights( true );
	tree->setAllColumnsShowFocus(true);
    tree->setWordWrap( false );
	tree->setSelectionBehavior( QAbstractItemView::SelectRows );
	tree->setSelectionMode( QAbstractItemView::ExtendedSelection );
	tree->header()->hide();
	tree->setModel( d_docTrace );
	connect( tree, SIGNAL( doubleClicked ( const QModelIndex & ) ), this, SLOT( onHistoryDblClck( const QModelIndex & ) ) );
	connect( tree, SIGNAL( activated ( const QModelIndex & ) ), this, SLOT( onHistoryDblClck( const QModelIndex & ) ) );

	QDockWidget* dock = createDock( this, tr("History" ), 0, true );
	dock->setWidget( tree );
	addDockWidget( Qt::LeftDockWidgetArea, dock );

	AutoMenu* pop = new AutoMenu( tree, true );
	pop->addCommand( tr("New Outline"), this, SLOT(onNewOutline()), tr("CTRL+N") );
	pop->addCommand( tr("Move to top"), this, SLOT(onMoveToTop()) );
	pop->addSeparator();
	pop->addCommand( tr("Show Outline"), this, SLOT(onOpenOutline()),tr("CTRL+O"), true );
	pop->addCommand( tr("Dock Outline"), this, SLOT(onAddDock()) );
	pop->addSeparator();
	pop->addCommand( tr("Delete selected..."), this, SLOT(onDeleteOutlines()) );
	pop->addSeparator();
	pop->addCommand( tr("Copy Document Ref."), this, SLOT(onCopyDocRef2()), tr("CTRL+C"), true );
	pop->addSeparator();
	pop->addCommand( tr("Set Properties..."), this, SLOT(onSetDocName()), tr("ALT+Return"), true );
	pop->addSeparator();
	pop->addCommand( tr("Import HTML..."), this, SLOT(onImportHtml()) );
	pop->addCommand( tr("Import Ecco..."), this, SLOT(onImportEcco()) );
	pop->addCommand( tr("Import Outline..."), this, SLOT(onImportStream()) );
	pop->addSeparator();
	pop->addCommand( tr("Export HTML..."), this, SLOT(onExportHtml()) );
	pop->addCommand( tr("Export Outline..."), this, SLOT(onExportOutline()) );
	addTopCommands( pop );
}

void Outliner::setupSearch()
{
#ifdef _HAS_CLUCENE_
	d_search = new SearchView( d_doc, this );
	QDockWidget* dock = createDock( this, tr("Search (CLucene)" ), 0, false );
	dock->setWidget( d_search );
	addDockWidget( Qt::BottomDockWidgetArea, dock );

	connect( d_search, SIGNAL( sigShowItem( quint64 ) ), this, SLOT( onSearchItemActivated( quint64 ) ) );

	AutoMenu* pop = new AutoMenu( d_search, true );
	pop->addCommand( tr("Show Item"), d_search, SLOT(onGoto()) );
	pop->addCommand( tr("Copy"), this, SLOT(onCopyRef()), tr("CTRL+C") );
	new AutoShortcut( d_search,  SLOT(onCopyRef()), tr("CTRL+C") );
	pop->addCommand( tr("Copy Document Ref."), this, SLOT(onCopyDocRef3()), tr("CTRL+SHIFT+C") );
	new AutoShortcut( d_search,  SLOT(onCopyDocRef3()), tr("CTRL+SHIFT+C") );
	pop->addCommand( tr("Clear"), d_search, SLOT(onClearSearch()) );
	pop->addSeparator();
	pop->addCommand( tr("Update Index..."), d_search, SLOT(onUpdateIndex()) );
	pop->addCommand( tr("Rebuild Index..."), d_search, SLOT(onRebuildIndex()) );
	addTopCommands( pop );
#endif
}

void Outliner::setupSearch2()
{
	d_sv2 = new SearchView2( this );
	QDockWidget* dock = createDock( this, tr("Search" ), 0, false );
	dock->setWidget( d_sv2 );
	addDockWidget( Qt::TopDockWidgetArea, dock );
	new QShortcut(tr("ESC"), dock, SLOT(close()) );

	connect( d_sv2, SIGNAL(sigFollow(quint64) ), this, SLOT( onSearchItemActivated( quint64 ) ) );

    Gui::AutoMenu* pop = new Gui::AutoMenu( d_sv2, true );
	pop->addCommand( tr("Clear"), d_sv2, SLOT(onClearSearch()) );
	pop->addCommand( tr("Expand All"), d_sv2, SLOT(onOpenAll()) );
	pop->addCommand( tr("Collapse All"), d_sv2, SLOT(onCloseAll()) );
	pop->addSeparator();
	pop->addCommand( tr("Copy"), d_sv2, SLOT(onCopyRef()), tr("CTRL+C"), true );
	pop->addSeparator();
	pop->addCommand( tr("Rebuild Index..."), d_sv2, SLOT(onRebuildIndex()) );
#ifdef _DEBUG
	pop->addCommand( tr("Test"), d_sv2, SLOT(onTest()) );
#endif
	addTopCommands( pop );
}

void Outliner::setupTerminal()
{
#ifdef _HAS_LUA_
	QDockWidget* dock = createDock( this, tr("Lua Terminal"), 0, false );
	Lua::Terminal2* term = new Lua::Terminal2( dock );
	dock->setWidget( term );
	addDockWidget( Qt::BottomDockWidgetArea, dock );
#endif
}

void Outliner::onHistoryDblClck(const QModelIndex & )
{
	onOpenOutline();
}

void Outliner::onAliasDblClck(const QModelIndex & i )
{
	Udb::Obj o = d_aliasList->getObject( i );
	gotoItem( o );
}

void Outliner::onOpenOutline()
{
	ENABLED_IF( d_docTraceTree->currentIndex().isValid() );
	Udb::Obj o = d_docTrace->getObj( d_docTraceTree->currentIndex() );
	if( !o.isNull() )
	{
		addOrShowTab( o );
	}
}

void Outliner::onExportHtml()
{
	Udb::Obj oln = getCurrentDoc( true );
	ENABLED_IF( !oln.isNull() );

	if( d_lastPath.isEmpty() )
		d_lastPath = QFileInfo( d_doc->getDb()->getFilePath() ).absolutePath();
	QString path = QFileDialog::getSaveFileName( this, tr("Export to HTML"), 
		QDir( d_lastPath ).absoluteFilePath( TypeDefs::prettyTitle( oln ) ),
		"HTML files (*.html)", 0, QFileDialog::DontUseNativeDialog ); 
	if( path.isNull() )
		return;
	QFileInfo info( path );
	d_lastPath = info.absolutePath();
	if( info.suffix().toLower() != "html" )
		path += ".html";

	QFile f( path );
	if( !f.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( this, tr("Export to HTML" ), tr( "cannot open '%1' for writing" ).arg(path) );
		return;
	}
	QTextStream out( &f );
	OutlineToHtml writer(true,true);
    writer.writeTo( out, oln, TypeDefs::prettyTitle( oln, false ) );
}

void Outliner::onSetDocName()
{
	if( !d_docTraceTree->currentIndex().isValid() )
		return;
	Udb::Qit i = d_docTrace->getItem( d_docTraceTree->currentIndex() );
	if( i.isNull() )
		return;
	Udb::Obj o =  d_doc->getTxn()->getObject( i.getValue().getOid() );
	if( o.isNull() )
		return;
	ENABLED_IF( !o.getValue( AttrItemIsReadOnly ).getBool() && !d_doc->getDb()->isReadOnly());
	ChangeNameDlg dlg( this );
	if( dlg.edit( o ) )
		o.commit();
}

void Outliner::onSetDocProps()
{
	Udb::Obj oln = getCurrentDoc();
	ENABLED_IF( !oln.isNull() && !oln.getValue( AttrItemIsReadOnly ).getBool() && 
		!d_doc->getDb()->isReadOnly());
	ChangeNameDlg dlg( this );
	if( dlg.edit( oln ) )
        oln.commit();
}

void Outliner::onDeleteDoc()
{
	Udb::Obj oln = getCurrentDoc();
    ENABLED_IF( !oln.isNull() && !oln.getValue( AttrItemIsReadOnly ).getBool() &&
        !d_doc->getDb()->isReadOnly());
    if( QMessageBox::warning( this, tr("Delete Outline"),
        tr("Do you really want to permanently delete this outline? This cannot be undone." ),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::No )
        return;
	removeDock(oln);
    Udb::Qit q = d_doc->getRoot().getFirstSlot();
    if( !q.isNull() ) do
    {
        if( q.getValue().getOid() == oln.getOid() )
        {
            q.erase();
            break;
        }
    }while( q.next() );
    oln.erase();
    d_doc->getTxn()->commit();
}

void Outliner::onShowInTrace()
{
	Udb::Obj oln = getCurrentDoc();
    ENABLED_IF( !oln.isNull() );
    Udb::Qit q = d_doc->getRoot().getFirstSlot();
    if( !q.isNull() ) do
    {
        if( q.getValue().getOid() == oln.getOid() )
        {
            d_docTraceTree->setCurrentIndex( d_docTrace->getIndex( q, true ) );
            return;
        }
    }while( q.next() );
}

void Outliner::onInsertOutline()
{
	Udb::Obj p = getCurrentItem();
	if( p.isNull() )
		return;
	Udb::Obj home = p.getValueAsObj( AttrItemHome );
	ENABLED_IF( !home.isNull() && !home.getValue( AttrItemIsReadOnly ).getBool() && 
		!d_doc->getDb()->isReadOnly());

	Udb::Obj oln = d_doc->getTxn()->createObject( TypeOutline );
	oln.setValue( AttrCreatedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	oln.setValue( AttrValuta, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	ChangeNameDlg dlg( this );
	dlg.setWindowTitle( tr("New Outline") );
	if( !dlg.edit( oln ) )
	{
		d_doc->getTxn()->rollback();
		return;
	}

	Udb::Obj sub = p.createAggregate( TypeOutlineItem );
	sub.setValue( AttrCreatedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	sub.setValue( AttrItemHome, home );
	sub.setValue( AttrItemAlias, oln );

	Udb::Obj root = d_doc->getRoot();
	Udb::Qit q = root.appendSlot( oln );
	oln.commit();
	d_docTraceTree->setCurrentIndex( d_docTrace->getIndex( q ) );
	addOrShowTab( oln, false, true );
}

void Outliner::onSetFont()
{
	ENABLED_IF(true);

    QFont f1 = Txt::Styles::inst()->getFont();
	bool ok;
    QFont f2 = QFontDialog::getFont( &ok, f1, this, tr("Select Outline Font - CrossLine" ) );
    if( !ok )
        return;
    f1.setFamily( f2.family() );
    f1.setPointSize( f2.pointSize() );
    AppContext::inst()->setDocFont( f1 );

}

void Outliner::onSetAppFont()
{
    ENABLED_IF( true );
    QFont f1 = QApplication::font();
    bool ok;
    QFont f2 = QFontDialog::getFont( &ok, f1, this, tr("Select Application Font - CrossLine" ) );
    if( !ok )
        return;
    f1.setFamily( f2.family() );
    f1.setPointSize( f2.pointSize() );
    QApplication::setFont( f1 );
    AppContext::inst()->getSet()->setValue( "App/Font", f1 );
}

void Outliner::onImportHtml()
{
	ENABLED_IF(!d_doc->getDb()->isReadOnly());
	QString filter;
	QString path = QFileDialog::getOpenFileName( this, tr("Import HTML Document"), d_lastPath,
		"HTML files (*.html *.htm)", 
		&filter, QFileDialog::DontUseNativeDialog ); 
	if( path.isNull() || filter.isNull() )
		return;
	QFileInfo info( path );
	d_lastPath = info.absolutePath();

	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::critical( this, tr("Import HTML Document" ), tr( "cannot open '%1' for reading" ).arg(path) );
		return;
	}

	QApplication::setOverrideCursor( Qt::WaitCursor );

	HtmlToOutline hi;
	hi.setContext( info.absoluteDir() );
	Udb::Obj o = hi.parse( OutlineCtrl::fetchHtml( f.readAll() ), d_doc->getTxn() );
	if( o.isNull() )
	{
		QApplication::restoreOverrideCursor();
		d_doc->getTxn()->rollback();
		QMessageBox::critical( this, tr("Import HTML Document" ), tr( "Error parsing HTML: %1" ).arg( hi.getError() ) );
		return;
	}
	o.setTimeStamp( AttrValuta );
	QApplication::restoreOverrideCursor();
	Udb::Obj root = d_doc->getRoot();
	o.setType( TypeOutline );
	if( o.getValue( AttrText ).isNull() )
		o.setValue( AttrText, Stream::DataCell().setString( info.completeBaseName() ) );
	root.appendSlot( o );
	d_doc->getTxn()->commit();
	addOrShowTab( o );
}

void Outliner::setCaption()
{
	QFileInfo info( d_doc->getDb()->getFilePath() );
	setWindowTitle( tr("%1 - CrossLine").arg( info.baseName() ) );
}

void Outliner::closeEvent( QCloseEvent * event )
{
	AppContext::inst()->getSet()->setValue("MainFrame/State/" +
		d_doc->getDb()->getDbUuid().toString(), saveState() );
    AppContext::inst()->getSet()->setValue("MainFrame/Size", geometry() );
    QMainWindow::closeEvent( event );
	if( event->isAccepted() )
		emit closing();
}

void Outliner::changeEvent(QEvent *event)
{
#ifdef _HAS_LUA_
	if( event->type() == QEvent::ActivationChange )
		Binding::setTop( this );
#endif
    if( event->type() == QEvent::WindowStateChange )
    {
        if( ( windowState() & Qt::WindowFullScreen ) ||
                ( windowState() & Qt::WindowMaximized && windowFlags() & Qt::FramelessWindowHint ) )
            d_show = FullScreen;
        else if( windowState() & Qt::WindowMaximized )
            d_show = Maximized;
        else
        {
            AppContext::inst()->getSet()->setValue("MainFrame/Size", geometry() );
            d_show = Normal;
        }
        AppContext::inst()->getSet()->setValue("MainFrame/Show", d_show );
    }
	QMainWindow::changeEvent( event );
}

void Outliner::onImportEcco()
{
	ENABLED_IF(!d_doc->getDb()->isReadOnly());
	QString filter;
	QString path = QFileDialog::getOpenFileName( this, tr("Import Ecco File"), d_lastPath,
		"Ecco files (*.txt)", 
		&filter, QFileDialog::DontUseNativeDialog ); 
	if( path.isNull() || filter.isNull() )
		return;
	QFileInfo info( path );
	d_lastPath = info.absolutePath();

	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::critical( this, tr("Import Ecco File" ), tr( "cannot open '%1' for reading" ).arg(path) );
		return;
	}
	Udb::Database::TxnGuard guard( d_doc->getDb() );
	QApplication::setOverrideCursor( Qt::WaitCursor );
	EccoToOutline in;
	Udb::Obj root = d_doc->getRoot();
	if( !in.parse( &f, root ) )
	{
		QApplication::restoreOverrideCursor();
		d_doc->getTxn()->rollback();
		guard.rollback();
		QMessageBox::critical( this, tr("Import Ecco File" ), tr( "Error parsing file: %1" ).arg( in.getError() ) );
		return;
	}
	QApplication::restoreOverrideCursor();
	d_doc->getTxn()->commit();
}

void Outliner::onImportStream()
{
	ENABLED_IF(!d_doc->getDb()->isReadOnly());
	QString filter;
	QString path = QFileDialog::getOpenFileName( this, tr("Import Outline Stream"), d_lastPath,
		"Outline Stream (*.cldx)", 
		&filter, QFileDialog::DontUseNativeDialog ); 
	if( path.isNull() || filter.isNull() )
		return;
	QFileInfo info( path );
	d_lastPath = info.absolutePath();

	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::critical( this, tr("Import Ecco File" ), tr( "cannot open '%1' for reading" ).arg(path) );
		return;
	}
	Udb::Database::TxnGuard guard( d_doc->getDb() );
	QApplication::setOverrideCursor( Qt::WaitCursor );
	Stream::DataReader r( &f );
	Udb::Obj oln;
	if( r.nextToken(true) == Stream::DataReader::Slot && r.readValue().getArr() == "CrossLineStream" )
	{
		if( r.nextToken() != Stream::DataReader::Slot && r.readValue().getArr() != "CrossLineStream" ||
			r.nextToken() != Stream::DataReader::Slot && r.readValue().getArr() != "0.1" ||
			r.nextToken() != Stream::DataReader::Slot && !r.readValue().isDateTime() )
		{
			QApplication::restoreOverrideCursor();
			d_doc->getTxn()->rollback();
			guard.rollback();
			QMessageBox::critical( this, tr("Import Outline Stream" ), tr( "Invalid file format" ) );
			return;
		}
		OutlineStream in;
		oln = in.readFrom( r, d_doc->getTxn() );
		if( oln.isNull() )
		{
			QApplication::restoreOverrideCursor();
			d_doc->getTxn()->rollback();
			guard.rollback();
			QMessageBox::critical( this, tr("Import Outline Stream" ), tr( "Error parsing file: %1" ).arg( in.getError() ) );
			return;
		}
		oln.setType( TypeOutline );
		oln.setValue( AttrText, Stream::DataCell().setString( info.completeBaseName() ) );
	}else
	{
		oln = d_doc->getTxn()->createObject( TypeOutline );
		oln.setValue( AttrCreatedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
		oln.setValue( AttrValuta, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
		const QByteArray res = OutlineUdbStream::readOutline( r, oln );
		if( !res.isEmpty() )
		{
			QApplication::restoreOverrideCursor();
			d_doc->getTxn()->rollback();
			guard.rollback();
			QMessageBox::critical( this, tr("Import Outline Stream" ), tr( "Error parsing file: %1" ).arg( res.data() ) );
			return;
		}
	}
	Udb::Obj root = d_doc->getRoot();
	Udb::Qit q = root.appendSlot( oln );
	d_doc->getTxn()->commit();
	d_docTraceTree->setCurrentIndex( d_docTrace->getIndex( q ) );
	addOrShowTab( oln, false );
	QApplication::restoreOverrideCursor();
}

void Outliner::onExportOutline()
{
	Udb::Obj oln = getCurrentDoc( true );
	ENABLED_IF( !oln.isNull() );

	QString path = QFileDialog::getSaveFileName( this, tr("Export Outline Stream"), 
		QDir( d_lastPath ).absoluteFilePath( TypeDefs::prettyTitle( oln ) ),
		"Outline Stream (*.cldx)", 0, QFileDialog::DontUseNativeDialog ); 
	if( path.isNull() )
		return;
	QFileInfo info( path );
	d_lastPath = info.absolutePath();
	if( info.suffix().toLower() != "cldx" )
		path += ".cldx";

	QFile f( path );
	if( !f.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( this, tr("Export Outline Stream" ), tr( "cannot open '%1' for writing" ).arg(path) );
		return;
	}
	Stream::DataWriter w( &f );
#ifdef _old_oln_format_
	w.writeSlot( Stream::DataCell().setAscii( "CrossLineStream" ) );
	w.writeSlot( Stream::DataCell().setAscii( "0.1" ) );
	w.writeSlot( Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	OutlineStream::writeTo( w, oln );
	d_doc->getTxn()->commit(); // wegen UUID
#else
	OutlineUdbStream::writeOutline( w, oln );
#endif
}

void Outliner::onNewOutline()
{
	ENABLED_IF(!d_doc->getDb()->isReadOnly());

	newOutline();
}

void Outliner::onMoveToTop()
{
	if( !d_docTraceTree->currentIndex().isValid() )
		return;
	Udb::Qit i = d_docTrace->getItem( d_docTraceTree->currentIndex() );
	if( i.isNull() )
		return;
	Udb::Obj o =  d_doc->getTxn()->getObject( i.getValue().getOid() );
	if( o.isNull() )
		return;
	ENABLED_IF( !o.getValue( AttrItemIsReadOnly ).getBool() && !d_doc->getDb()->isReadOnly());

	Udb::Obj root = d_doc->getRoot();
	o.setValue( AttrValuta, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	i.erase();
	i = root.appendSlot( o );
	o.commit();
	d_docTraceTree->setCurrentIndex( d_docTrace->getIndex( i.getSlotNr() ) );
}

void Outliner::onDeleteOutlines()
{
	ENABLED_IF( d_docTraceTree->selectionModel()->hasSelection() && !d_doc->getDb()->isReadOnly());

	QModelIndexList l = d_docTraceTree->selectionModel()->selectedRows();
	if( l.isEmpty() )
		return;
	const int res = QMessageBox::warning( this, tr("Delete Outlines"), 
		tr("Do you really want to permanently delete the %1 selected outlines? This cannot be undone." ).arg( l.size() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
	if( res == QMessageBox::No )
		return;
	for( int i = 0; i < l.size(); i++ )
	{
		d_docTrace->getObj( l[i] ).erase();
		d_docTrace->getItem( l[i] ).erase();
	}
	d_doc->getTxn()->commit();
}

Udb::Obj Outliner::getCurrentItem(bool includeRoot) const
{
	if( Oln::OutlineTree* tree = dynamic_cast<Oln::OutlineTree*>( QApplication::focusWidget() ) )
	{
		QModelIndex idx = tree->currentIndex();
		if( !includeRoot && !idx.isValid() )
			return Udb::Obj();
		return d_doc->getTxn()->getObject( tree->model()->data( idx, Oln::OutlineMdl::OidRole ).toULongLong() );
	}else if( includeRoot && QApplication::focusWidget() == d_docTraceTree && d_docTraceTree->currentIndex().isValid() )
	{
		return d_docTrace->getObj( d_docTraceTree->currentIndex() );
	}else
		return Udb::Obj();
}

Udb::Obj Outliner::getCurrentDoc(bool includeRoot) const
{
	Udb::Obj oln = getCurrentItem( includeRoot );
	if( oln.isNull() )
		return oln;
	if( oln.getType() == TypeOutlineItem )
		return oln.getValueAsObj( AttrItemHome );
	else
		return oln;
}

bool Outliner::gotoItem( const Udb::Obj& o, bool expand, bool checkDocks )
{
	if( o.isNull(true,true) )
		return false;

	Udb::Obj oln;
	Udb::Obj item;

	if( o.getType() == TypeOutlineItem )
	{
		oln = o.getValueAsObj( AttrItemHome );
		item = o;
	}else
		oln = o;

	if( oln.isNull(true,true) )
		return false;

#ifdef _HAS_LUA_
	Binding::setCurrentObject( o );
#endif

	OutlineUdbCtrl* ctrl = 0;
	if( checkDocks )
		ctrl = showInDocks( oln, item );
	if( ctrl == 0 )
		ctrl = addOrShowTab( oln, item.isNull() );
	if( ctrl != 0 && !item.isNull() )
	{
		pushBack(item);
		if( ctrl->gotoItem( item.getOid() ) && expand )
			ctrl->getTree()->expand( ctrl->getMdl()->getIndex( item.getOid() ) );
	}
    return true;
}

void Outliner::showOid(quint64 oid)
{
    Udb::Obj o = d_doc->getTxn()->getObject( oid );
    if( !o.isNull() )
		gotoItem( o, true );
}

Udb::Obj Outliner::newOutline(bool showDlg)
{
	Udb::Obj oln = d_doc->getTxn()->createObject( TypeOutline );
	oln.setValue( AttrCreatedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	oln.setValue( AttrValuta, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	ChangeNameDlg dlg( this );
	dlg.setWindowTitle( tr("New Outline") );
	if( showDlg && !dlg.edit( oln ) )
	{
		d_doc->getTxn()->rollback();
		return Udb::Obj();
	}
	Udb::Obj root = d_doc->getRoot();
	Udb::Qit q = root.appendSlot( oln );
	oln.commit();
	if( showDlg )
	{
		d_docTraceTree->setCurrentIndex( d_docTrace->getIndex( q ) );
		addOrShowTab( oln, false, true );
	}
	return oln;
}

Udb::Obj Outliner::currentDoc() const
{
	return d_tab->getCurrentObj();
}

void Outliner::onFollowAlias()
{
	Udb::Obj o = getCurrentItem();
	ENABLED_IF( !o.isNull() && o.getValue( AttrItemAlias ).isOid() );

	Udb::Obj a = o.getValueAsObj( AttrItemAlias );
	if( !gotoItem( a ) )
		QMessageBox::information( this, tr("Follow Alias"), tr("This is a dangling link!") );
}

void Outliner::onCopyDocRef()
{
	Udb::Obj o = getCurrentDoc();
	ENABLED_IF( !o.isNull() );
	copyRef( o );
}

void Outliner::onCopyDocRef2()
{
	ENABLED_IF( d_docTraceTree->currentIndex().isValid() );
	copyRef( d_docTrace->getObj( d_docTraceTree->currentIndex() ) );
}

void Outliner::onCopyDocRef3()
{
#ifdef _HAS_CLUCENE_
	ENABLED_IF( d_search->getItem() );
	copyRef( d_doc->getTxn()->getObject( d_search->getItem() ) );
#endif
}

void Outliner::onCopyRef()
{
#ifdef _HAS_CLUCENE_
	ENABLED_IF( d_search->getItem() );
	copyRef( d_doc->getTxn()->getObject( d_search->getItem() ), false );
#endif
}

void Outliner::copyRef( Udb::Obj o, bool docRef )
{
	if( o.isNull() )
		return;
	if( docRef && o.getType() == TypeOutlineItem )
		o = o.getValueAsObj( AttrItemHome );
	if( o.isNull() )
		return; // RISK
	QMimeData* mimeData = new QMimeData();
	QList<Udb::Obj> objs;
	objs << o;
	Udb::Obj::writeObjectRefs( mimeData, objs );
	Udb::Obj::writeObjectUrls( mimeData, objs, Udb::ContentObject::AttrAltIdent, Udb::ContentObject::AttrText );
	QApplication::clipboard()->setMimeData( mimeData );
}

void Outliner::onSearchItemActivated( quint64 oid )
{
	Udb::Obj o = d_doc->getTxn()->getObject( oid );
	if( !gotoItem( o, true ) )
		QMessageBox::information( this, tr("Show Item"), tr("The item does no longer exist!") );
	else
	{
#ifdef _HAS_LUA_
	Binding::setCurrentObject( o );
#endif
		d_aliasList->setObj( o, true );
	}
}

void Outliner::onTabChanged()
{
	Udb::Obj o = getCurrentItem(true);
	if( o.isNull() )
		o = d_tab->getCurrentObj();
	d_aliasList->setObj( o, true );
#ifdef _HAS_LUA_
	Binding::setCurrentObject( o );
#endif
}

void Outliner::onDockItemSelected()
{
	Udb::Obj o = getCurrentItem(true);
	d_aliasList->setObj( o, true );
#ifdef _HAS_LUA_
	Binding::setCurrentObject( o );
#endif
}

void Outliner::pushBack(const Udb::Obj & o)
{
	if( d_pushBackLock )
		return;
	if( o.isNull( true, true ) )
		return;
#ifdef _HAS_LUA_
		Binding::setCurrentObject( o );
#endif
	if( !d_backHisto.isEmpty() && d_backHisto.last() == o.getOid() )
		return; // o ist bereits oberstes Element auf dem Stack.
	d_backHisto.removeAll( o.getOid() );
	d_backHisto.push_back( o.getOid() );
}

void Outliner::onGoBack()
{
	// d_backHisto.last() ist Current
	ENABLED_IF( d_backHisto.size() > 1 );

	d_pushBackLock++;
	d_forwardHisto.push_back( d_backHisto.last() );
	d_backHisto.pop_back();
	gotoItem( d_doc->getTxn()->getObject( d_backHisto.last() ) );
	d_pushBackLock--;
}

void Outliner::onGoForward()
{
	ENABLED_IF( !d_forwardHisto.isEmpty() );

	Udb::Obj cur = d_doc->getTxn()->getObject( d_forwardHisto.last() );
	d_forwardHisto.pop_back();
	gotoItem( cur );

}

void Outliner::onCurrentItemChanged( quint64 oid )
{
	Udb::Obj o = d_doc->getTxn()->getObject( oid );
	pushBack(o);
	Udb::Obj home = o.getValueAsObj( AttrItemHome );
	if( home.isNull() )
		return;
	d_aliasList->setObj( o );
	d_aliasList->focusOn( home );
}

void Outliner::onSearch()
{
#ifdef _HAS_CLUCENE_
	ENABLED_IF( true );

	d_search->parentWidget()->show();
	d_search->parentWidget()->raise();
	d_search->onNew();
#endif
}

void Outliner::onSearch2()
{
	ENABLED_IF( true );

	d_sv2->parentWidget()->show();
	d_sv2->parentWidget()->raise();
	d_sv2->doNew();
}

void Outliner::onAbout()
{
	ENABLED_IF( true );

	QMessageBox::about( this, tr("About CrossLine"), 
		tr("<html><body><p>Release: <strong>%1</strong>   Date: <strong>%2</strong> </p>"
           "<p>CrossLine is an Outliner in the tradition of Ecco Pro with cross-link capabilities.</p>"
		   "<p>Author: Rochus Keller, me@rochus-keller.ch</p>"
		   "<p>Copyright (c) 2009-%3</p>"
           "<p>See <a href='https://github.com/rochus-keller/CrossLine'>the Github repository</a> for more information.</p>"
           "<h4>Additional Credits:</h4>"
           "<p>"
           "<a href='https://github.com/rochus-keller/LeanQt'>LeanQt</a>, Copyright (c) 2022 Rochus Keller, "
                "2016 The Qt Company Ltd, 2008 Nokia, 1992 Trolltech AS<br>"
		   "Sqlite 3.5, <a href='http://sqlite.org/copyright.html'>dedicated to the public domain by the authors</a><br>"
#ifdef _HAS_CLUCENE_
           "<a href='http://www.sourceforge.net/projects/clucene'>CLucene</a>, Copyright (c) "
                "2006 Ben van Klinken and the CLucene Team<br>"
#endif
           "<a href='http://snowball.tartarus.org/'>Libstemmer</a>, Copyright (c) 2002 Dr Martin Porter, Richard Boulton<br>"
#ifdef _HAS_LUA_
           "Lua 5.1, (c) 2006 Tecgraf, PUC-Rio<br>"
#endif
           "<p><h4>Terms of use:</h4>"
           "CrossLine is available under <a href=\"https://www.gnu.org/licenses/license-list.html#GNUGPL\">GNU GPL v2 or v3</a>."
           "</body></html>" )
        .arg( AppContext::s_version ).arg( AppContext::s_date ).arg( QDate::currentDate().year() ) );
}

void Outliner::onAddDock()
{
	Udb::Obj oln = getCurrentDoc( true );
	ENABLED_IF( !oln.isNull() );
	for( int i = 0; i < d_docks.size(); i++ )
		if( d_docks[i]->getOutline().equals( oln ) )
		{
			d_docks[i]->getTree()->parentWidget()->show();
			return;
		}
	showAsDock( oln );
	streamDocks();
}

void Outliner::streamDocks()
{
	Stream::DataWriter dw;
	for( int i = 0; i < d_docks.size(); i++ )
		dw.writeSlot( d_docks[i]->getOutline() );
	Udb::Obj r = d_doc->getRoot();
	r.setValue( AttrRootDockList, dw.getBml() );
	r.commit();
}

OutlineUdbCtrl *Outliner::showInDocks(const Udb::Obj &oln, const Udb::Obj &item)
{
	if( d_tab->findDoc(oln) != -1 )
		return 0; // oln ist bereits in einem Tab offen; suche nicht weiter nach Docks

	foreach( OutlineUdbCtrl* ctrl, d_docks )
	{
		if( ctrl->getOutline().equals(oln) )
		{
			ctrl->getTree()->parentWidget()->show();
			ctrl->getTree()->setFocus();
			ctrl->gotoItem(item.getOid());
			return ctrl;
		}
	}
	return 0;
}

void Outliner::onRemoveDock()
{
	Udb::Obj oln = getCurrentDoc( true );

	ENABLED_IF( dockIndex( oln ) != -1 );

	removeDock( oln );
}

void Outliner::showAsDock( const Udb::Obj& oln )
{
	if( oln.isNull() )
		return;
	QApplication::setOverrideCursor( Qt::WaitCursor );
	QDockWidget* dock = createDock( this, oln.getValue( AttrText ).toString(), oln.getOid(), true );
	Oln::OutlineUdbCtrl* ctrl = OutlineUdbCtrl::create( dock, d_doc->getTxn() );
	ctrl->getTree()->setShowNumbers( false );
	ctrl->getTree()->setIndentation( 10 );
    ctrl->getTree()->setHandleWidth( 7 );

	// connect( ctrl->getTree(), SIGNAL( identClicked() ), this, SLOT( onDockItemSelected() ) ); // zu extrem
	connect( ctrl, SIGNAL( sigCurrentChanged( quint64 ) ), this, SLOT( onCurrentItemChanged( quint64 ) ) );
	connect( ctrl->getTree()->selectionModel(), SIGNAL( currentChanged ( const QModelIndex &, const QModelIndex & ) ), this, SLOT( onDockItemSelected() ) );
	connect( ctrl, SIGNAL(sigUrlActivated(QUrl)), this, SLOT(onFollowUrl(QUrl)) ); //, Qt::QueuedConnection );
    connect( ctrl, SIGNAL(sigLinkActivated(quint64)), this, SLOT(onSearchItemActivated(quint64)) );

	ctrl->getDeleg()->setBiggerTitle( false );
	ctrl->getDeleg()->setShowIcons( true );
	d_docks.append( ctrl );

	d_pushBackLock++;
	ctrl->setOutline( oln, true );
	// Durch diesen Trick ist oberstes Item current ohne dass es in backHisto eingefügt wird.
	// Bei false wird current Event ausgelöst beim ersten Fokus was schwieriger zu vermeiden ist.
	d_pushBackLock--;

	dock->setWidget( ctrl->getTree() );
	addDockWidget( Qt::LeftDockWidgetArea, dock );
	QApplication::restoreOverrideCursor();

	AutoMenu* pop = new AutoMenu( ctrl->getTree(), true );


	pop->addCommand( tr("New Outline..."), this, SLOT(onNewOutline()), tr( "CTRL+N" ) );
	//pop->addCommand( tr("Insert Outline..."), this, SLOT(onInsertOutline()), tr( "CTRL+SHIFT+N" ), true );
	pop->addCommand( tr("Delete Outline..."),  this, SLOT(onDeleteDoc()), tr("CTRL+SHIFT+D"), true );

	pop->addSeparator();

	AutoMenu* sub = new AutoMenu( tr("Item" ), pop );
	pop->addMenu( sub );
	setupItemMenu(sub);

	sub = new AutoMenu( tr("Text" ), pop );
	pop->addMenu( sub );
	setupTextMenu(sub);

	pop->addSeparator();
	setupCopyMenu(pop);

	pop->addSeparator();
	pop->addCommand( tr("Set Outline Properties..."), this, SLOT(onSetDocProps()),tr("ALT+CTRL+Return") );
	pop->addAutoCommand( tr("Outline locked"), SLOT(onDocReadOnly()) );
	pop->addCommand( tr("Open on Start"), this, SLOT(onAutoStart()) );
	pop->addSeparator();
	pop->addAutoCommand( tr("Expand all"),  SLOT(onExpandAll()), tr("CTRL+SHIFT+A"), true );
	pop->addAutoCommand( tr("Collapse all"),  SLOT(onCollapseAll()) );
	pop->addCommand( tr("Show in History"), this, SLOT(onShowInTrace()) );
	pop->addCommand( tr("Show in Tab"), this, SLOT(onOpenFromDock()),tr("CTRL+O"), true );
	pop->addCommand( tr("Undock Outline"), this, SLOT(onRemoveDock()) );
	pop->addSeparator();
	pop->addCommand( tr("Export Outline..."), this, SLOT(onExportOutline()) );
	pop->addCommand( tr("Export HTML..."), this, SLOT(onExportHtml()) );

	pop->addSeparator();
	addTopCommands( pop );
}

int Outliner::dockIndex(const Udb::Obj & oln)
{
	for( int i = 0; i < d_docks.size(); i++ )
		if( d_docks[i]->getOutline().equals(oln) )
			return i;
	return -1;
}

void Outliner::removeDock(const Udb::Obj & oln)
{
	const int cur = dockIndex(oln);
	if( cur == -1 )
		return;
	QWidget* dock = d_docks[cur]->getTree()->parentWidget();
	d_docks.removeAt( cur );
	removeDockWidget( static_cast<QDockWidget*>( dock ) );
	dock->deleteLater();
	streamDocks();
}

void Outliner::toFullScreen(QMainWindow * w)
{
#ifdef _WIN32
    w->showFullScreen();
#else
    w->setWindowFlags( Qt::Window | Qt::FramelessWindowHint  );
    w->showMaximized();
    w->setWindowIcon( qApp->windowIcon() );
#endif
}

void Outliner::addTopCommands(Gui::AutoMenu * pop )
{
	Q_ASSERT( pop != 0 );
	pop->addSeparator();

    Gui::AutoMenu * sub = new AutoMenu( tr("Navigate" ), pop );
	pop->addMenu( sub );
	setupNaviMenu(sub);

	sub = new AutoMenu( tr("Configuration" ), pop );
	pop->addMenu( sub );
	sub->addCommand( tr("Select Outline Font..."), this, SLOT(onSetFont()) );
	sub->addCommand( tr("Select Application Font..."), this, SLOT(onSetAppFont()) );
	sub->addCommand( tr("Update Indices..."), this, SLOT(onRebuildBackRefs()) );
	sub->addCommand( tr("Show FullScreen"), this, SLOT( onFullScreen() ), tr("F11") );
	QMenu* sub2 = createPopupMenu();
	sub2->setTitle( tr("Show Window") );
	pop->addMenu( sub2 );
	pop->addCommand( tr("About CrossLine..."), this, SLOT(onAbout()) );
	pop->addSeparator();
	pop->addAction( tr("Quit"), this, SLOT(close()), tr("CTRL+Q") );
}

void Outliner::onFullScreen()
{
    CHECKED_IF( true, d_show == FullScreen );

    if( d_show == FullScreen )
    {
#ifndef _WIN32
        setWindowFlags( Qt::Window );
#endif
        showNormal();
    }else
        toFullScreen( this );
}

void Outliner::onOpenFromDock()
{
	Udb::Obj o = getCurrentItem(true);
	ENABLED_IF( !o.isNull() );
	gotoItem( o, false, false );
}
