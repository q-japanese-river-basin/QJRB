/***************************************************************************
                          MyMainWindow.cpp  -  description
                             -------------------
    begin                : Nov. 04, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#include "MyMainWindow.h"
#include "QueryManage.h"
#include "DlgPointCsv.h"
#include "generated/ui_dlgAbout.h"

 //#include <qgsauthmanager.h>
#include <qgsfillsymbollayer.h>
#include <qgscategorizedsymbolrenderer.h>
#include <qgsdatasourceuri.h>
//#include <qgslayertreegroup.h>
//#include <qgslayertreeregistrybridge.h>
#include <qgslinesymbollayer.h>
 //#include <qgsmaplayerregistry.h>
#include <qgsmaptoolpan.h>
#include <qgsmaptoolzoom.h>
#include <qgsmarkersymbollayer.h>
#include <qgsproviderregistry.h>
#include <qgsrasterlayer.h>
#include <qgsrasterrenderer.h>
#include <qgssinglesymbolrenderer.h>
//#include <qgssvgannotationitem.h>
#include <qgssymbollayerutils.h>
//#include <qgsvectorcolorramp.h>
#include <qgsvectorlayer.h>

#include <qgslayertreemapcanvasbridge.h>
#include <qgslayertreemodel.h>
#include <qgslayertreeviewindicator.h>
#include <qgsmapcanvasannotationitem.h>
#include <qgssvgannotation.h>
#include <qgsapplication.h>
#include <qgsstyle.h>
#include <qgsproject.h>
#include <qgsauthmanager.h>
#include <qgsannotationmanager.h>
#include <qgslayertree.h>

#include "MyMapCanvas.h"

#include <QtGui>
#include <QtWidgets>
#include <gdal.h>


bool AppFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch ( event->type())
    {
    //list event you want to prevent here ...
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
	case QEvent::Wheel:
    //...
    return true;
    }
    return QObject::eventFilter( obj, event );
}


MyMainWindow::MyMainWindow( QSplashScreen *pSplash )
	: m_pSplash(pSplash)
{
	QString strm = QgsApplication::qgisMasterDatabaseFilePath();

	// initialize QGIS
	QString dbError;
	if ( !QgsApplication::createDatabase( &dbError ) )
	{
		QMessageBox::critical( this, "Private qgis.db", dbError );
		exit( EXIT_FAILURE );
	}
	QString str = QgsApplication::pluginPath();
//	QgsAuthManager::instance()->init( QgsApplication::pluginPath() );
	//if ( !QgsAuthManager::instance()->isDisabled() )
	//{
	//	masterPasswordSetup();
	//}
	//QThread *p = QCoreApplication::instance()->thread();
	QgsApplication::initQgis();

	setupUi( this );

	QgsApplication::setUITheme( "default" );

	m_pAssistant = NULL;
	m_pPointAnnotation = NULL;
	m_pFilter = new AppFilter();

	m_pSplash->showMessage( _tr("ウィジェットを準備しています"), Qt::AlignLeft | Qt::AlignBottom, Qt::yellow);
	qApp->processEvents();

	centralwidget->enableAntiAliasing(true);
	centralwidget->setCanvasColor(QColor(192, 192, 255));
	centralwidget->freeze(false);
	centralwidget->setVisible(true);
	centralwidget->clearExtentHistory();
//	centralwidget->setCrsTransformEnabled( true );

	// setup map tools
	m_pMapToolPan = new QgsMapToolPan( centralwidget );
	m_pMapToolZoomIn = new QgsMapToolZoom( centralwidget, false );
	m_pMapToolZoomOut = new QgsMapToolZoom( centralwidget, true );
	m_pMapToolSelect = new MapToolSelect( centralwidget );

	m_pgrpViewControl = new QActionGroup( this );
	m_pgrpViewControl->addAction( actionPan );
	m_pgrpViewControl->addAction( actionZoomIn );
	m_pgrpViewControl->addAction( actionZoomOut );
	m_pgrpViewControl->addAction( actionSelect );

	// create map canvas bridge
	mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), centralwidget, this );
	connect( mLayerTreeCanvasBridge, &QgsLayerTreeMapCanvasBridge::canvasLayersChanged, centralwidget, &QgsMapCanvas::setLayers );

	// layer tree view
	auto *model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
	model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
	model->setFlag( QgsLayerTreeModel::UseEmbeddedWidgets );
//	model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
	model->setFlag( QgsLayerTreeModel::AllowLegendChangeState );
	model->setFlag( QgsLayerTreeModel::ActionHierarchical );
	model->setAutoCollapseLegendNodes( 0 );
	treeView_legend->setModel( model );
	treeView_legend->defaultActions();
	QObject::connect( treeView_legend, SIGNAL(currentLayerChanged(QgsMapLayer*)), 
		this, SLOT(enableRasterOpacitySlider( QgsMapLayer*)) );


	// load layers
	LoadMaps();


	// setup legend
//	SetupLegend();

	// setup lutable
	SetupLuTable();

//	centralwidget->setCrsTransformEnabled( true );
	centralwidget->setDestinationCrs( QgsCoordinateReferenceSystem( 3857, QgsCoordinateReferenceSystem::EpsgCrsId ) );
	m_pCoordinateTrans = new QgsCoordinateTransform(
		QgsCoordinateReferenceSystem( 4612, QgsCoordinateReferenceSystem::EpsgCrsId ),
		QgsCoordinateReferenceSystem( 3857, QgsCoordinateReferenceSystem::EpsgCrsId ),
		QgsProject::instance() );
	centralwidget->setExtent( m_pCoordinateTrans->transform( m_lstLayers[m_lstLayers.size()-1]->extent(), QgsCoordinateTransform::ForwardTransform ) );
//	centralwidget->setLayerSet( m_lstLayers );
	centralwidget->setLayers( m_lstLayers );
	QgsProject::instance()->layerTreeRoot()->setCustomLayerOrder( m_lstLayers );

	qDebug() << centralwidget->isParallelRenderingEnabled();

	centralwidget->freeze( false );
//	centralwidget->refreshAllLayers();
//	centralwidget->refresh();

	qDebug() << centralwidget->extent().toString();

	QObject::connect( m_pMapToolSelect, SIGNAL(mousePressed(QgsPointXY&)), this, SLOT(aggregateMesh(QgsPointXY&)) );

	// 3.0 connect annotation manager and canvas.
	connect( QgsProject::instance()->annotationManager(), 
		&QgsAnnotationManager::annotationAdded, this, &MyMainWindow::annotationCreated );

	// connect right button on mapcanvas
	connect( centralwidget, &MyMapCanvas::rightButtonClicked, this, &MyMainWindow::resetSelectedArea );

	m_pSplash->showMessage( _tr("準備完了"), Qt::AlignLeft | Qt::AlignBottom, Qt::yellow);
	qApp->processEvents();
}


MyMainWindow::~MyMainWindow(void)
{
	QgsApplication::exitQgis(); //の中身
/**************************************************************************************************
	delete QgsApplication::authManager();

	//Ensure that all remaining deleteLater QObjects are actually deleted before we exit.
	//This isn't strictly necessary (since we're exiting anyway) but doing so prevents a lot of
	//LeakSanitiser noise which hides real issues
	QgsApplication::sendPostedEvents( nullptr, QEvent::DeferredDelete );

	//delete all registered functions from expression engine (see above comment)
	QgsExpression::cleanRegisteredFunctions();

	delete QgsProject::instance();

	delete QgsProviderRegistry::instance();

	// invalidate coordinate cache while the PROJ context held by the thread-locale
	// QgsProjContextStore object is still alive. Otherwise if this later object
	// is destroyed before the static variables of the cache, we might use freed memory.
	QgsCoordinateTransform::invalidateCache();

	QgsStyle::cleanDefaultStyle();

	// tear-down GDAL/OGR
	OGRCleanupAll();
	GDALDestroyDriverManager();
**************************************************************************************************/

	delete m_pgrpViewControl;
	delete m_pMapToolPan;
	delete m_pMapToolZoomIn;
	delete m_pMapToolSelect;
	delete m_pFilter;
	delete m_pCoordinateTrans;

	delete QueryManage::instance();
}


void MyMainWindow::showEvent( QShowEvent *event )
{
	QgsProject::instance()->layerTreeRoot()->findLayer( m_lstLayers[1] )->setItemVisibilityChecked( false );
}


void MyMainWindow::resetSelectedArea()
{
	QueryManage *q = QueryManage::instance();
	q->clearResult();
	QgsProject::instance()->annotationManager()->removeAnnotation( m_pPointAnnotation );
	m_pPointAnnotation = NULL;
	dynamic_cast<QgsVectorLayer *>(m_lstLayers[m_nBasinLayerIdx])->setSubsetString( "OGC_FID = -1" );
	dynamic_cast<QgsVectorLayer *>(m_lstLayers[m_nLumeshLayerIdx])->setSubsetString("gid = -1" );
	resetResultTree();
}


void MyMainWindow::resetResultTree()
{
	for ( int i = 0; i < treeWidget_result->topLevelItemCount(); i++ )
	{
		QTreeWidgetItem *pItem = treeWidget_result->topLevelItem( i );
		pItem->setText( 1, QString() );
		pItem->setText( 2, QString() );
		for ( int j = 0; j < pItem->childCount(); j++ )
		{
			QTreeWidgetItem *pChildItem = pItem->child( j );
			pChildItem->setText( 1, QString() );
			pChildItem->setText( 2, QString() );
			for ( int k = 0; k < pChildItem->childCount(); k++ )
			{
				QTreeWidgetItem *pGChild = pChildItem->child( k );
				pGChild->setText( 1, QString() );
				pGChild->setText( 2, QString() );
			}
		}
	}
}


void MyMainWindow::closeEvent( QCloseEvent *event )
{
}


void MyMainWindow::LoadMaps()
{
	int nLayer = 0;

	centralwidget->freeze( true );

	m_pSplash->showMessage( _tr("河川データをロードしています"), Qt::AlignLeft | Qt::AlignBottom, Qt::yellow);
	qApp->processEvents();

	// open spatialite database
	QString strAppDirPath = QFileInfo( QCoreApplication::applicationDirPath() ).absoluteFilePath();
	QgsDataSourceUri uriNet = QgsDataSourceUri();

	QueryManage *q = QueryManage::instance();
	uriNet.setDatabase( q->getDbName() );

	if ( !QFile::exists( uriNet.database() ) )
	{
		QMessageBox::warning( this, _tr("エラー"), _tr("データベースを開けません [%1]").arg( uriNet.database() ) );
		QApplication::exit( 1 );
	}

	// legend 
//	QgsLegendSymbologyList listSymbol;
	QSize sizeIcon( 16, 16 );

	//treeView_legend->setHeaderLabel( tr("凡例") );

	/******************     layers      *******************/

	QgsVectorLayer::LayerOptions options;
	options.loadDefaultStyle = false;

	/******************  bgmap  *******************/

#ifdef ENABLE_BGMAP
	//#if 1

	m_pSplash->showMessage( _tr("背景図を準備しています"), Qt::AlignLeft | Qt::AlignBottom, Qt::yellow);
	qApp->processEvents();

	// load gsi tile map
	//QgsDataSourceUri uriBGMap( QString("http://gsi-cyberjapan.github.io/experimental_wmts/gsitiles_wmts.xml") );
	//uriBGMap.setParam( "url", "http://gsi-cyberjapan.github.io/experimental_wmts/gsitiles_wmts.xml" );
	//uriBGMap.setParam( "crs", "EPSG:3857" );	
	//uriBGMap.setParam( "dpiMode", "7" );
	//uriBGMap.setParam( "featureCount", "10" );
	//uriBGMap.setParam( "format", "image/png" );
	//uriBGMap.setParam( "layers", "pale" );
	//uriBGMap.setParam( "styles", "default" );
	//uriBGMap.setParam( "tileMatrixSet", "z2to18" );

	// TMS
	QgsDataSourceUri uriBGMap;
	uriBGMap.setParam("url", "https://cyberjapandata.gsi.go.jp/xyz/pale/{z}/{x}/{y}.png");
	uriBGMap.setParam("crs", "EPSG:3857");
	uriBGMap.setParam("type", "xyz");

	QgsRasterLayer *playerBgmap = new QgsRasterLayer( uriBGMap.encodedUri(), _tr("背景図"), QString("wms") );
	if ( !playerBgmap->isValid() )
	{
		QMessageBox::critical( this, "ERROR", _tr( "背景図を開けません" ) );
		delete playerBgmap;
		QApplication::exit( 1 );
	}
	QgsCoordinateReferenceSystem srs;
	srs.createFromSrid( 3857 );
	playerBgmap->setCrs( srs );

	qDebug() << playerBgmap->dataProvider()->name();
	qDebug() << uriBGMap.encodedUri();
	qDebug() << playerBgmap->source();
	qDebug() << playerBgmap->isValid();
	qDebug() << playerBgmap->extent().toString();
	qDebug() << playerBgmap->crs().toProj4();

	// legend
	//QTreeWidgetItem *pItemBGMap = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( _tr("背景図") ) );
	//pItemBGMap->setCheckState( 0, Qt::Checked );
	//pItemBGMap->setData( 0, Qt::UserRole, QVariant( nLayer++ ) );
	//pItemBGMap->setIcon( 0, QIcon(":/icons/mIconRaster.png") );
	//m_plistTopItems.append( pItemBGMap );
	//treeWidget_legend->insertTopLevelItem( 0, pItemBGMap );

	QgsProject::instance()->addMapLayer( playerBgmap, TRUE );
	m_lstLayers.append( playerBgmap );
	nLayer++;
#endif

	/******************  lu raster  *******************/

	QString strLuRaster( strAppDirPath + "/../../data/luraster.tif" );
	if ( QFile::exists( strLuRaster ) )
	{
		QgsRasterLayer *pLayerLuRaster = new QgsRasterLayer( strLuRaster, _tr("土地利用図"), "gdal" );
		if ( pLayerLuRaster->isValid() )
		{
			// set opacity
			QgsRasterRenderer* rasterRenderer = pLayerLuRaster->renderer();
			rasterRenderer->setOpacity( 0.5 );
			QgsProject::instance()->addMapLayer( pLayerLuRaster, TRUE );

			// legend
			//QTreeWidgetItem *pItemLuRaster = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("土地利用図") ) );
			//pItemLuRaster->setCheckState( 0, Qt::Unchecked );
			//pItemLuRaster->setData( 0, Qt::UserRole, QVariant( nLayer++ ) );
			//pItemLuRaster->setIcon( 0, QIcon(":/icons/mIconRaster.png") );
			//m_plistTopItems.append( pItemLuRaster );
			//treeView_legend->insertTopLevelItem( 0, pItemLuRaster );
			//QObject::connect( treeWidget_legend, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
			//	this, SLOT(enableRasterOpacitySlider(QTreeWidgetItem *, int)) );
			bool bResultFlag;
			pLayerLuRaster->loadNamedStyle
			( strAppDirPath + "/../../data/luraster/luraster.qml", bResultFlag );

			// set invisible default
			//			QgsMapCanvasLayer clayerLuRaster( pLayerLuRaster );
			//			clayerLuRaster.setVisible( false );
			//			m_lstLayers.append( clayerLuRaster );
			m_lstLayers.append( pLayerLuRaster );
//			QgsProject::instance()->layerTreeRoot()->findLayer( pLayerLuRaster )->setItemVisibilityChecked( false );
			//			QgsProject::instance()->layerTreeRegistryBridge()->setNewLayersVisible( false );
			m_nRasterLayerIdx = nLayer++;
		}
		else
		{
			delete pLayerLuRaster;
		}
	}

	/******************  basin poly  *******************/

	QString strBasinPolyName( strAppDirPath + "/../../data/basin_poly.shp" );
	if ( QFile::exists( strBasinPolyName ) )
	{
		QgsVectorLayer *pLayerBasinPoly = new QgsVectorLayer( strBasinPolyName, _tr("支流域"), "ogr" );
		if ( pLayerBasinPoly->isValid() )
		{
			// style
			QgsSymbol *pSymbolBasinPoly = QgsSymbol::defaultSymbol( QgsWkbTypes::PolygonGeometry );
			QgsSimpleFillSymbolLayer *pSLayer4 = new QgsSimpleFillSymbolLayer(
				QColor::fromRgb( 0, 0, 0 ), Qt::NoBrush, QColor::fromRgb( 0, 192, 192 ), Qt::SolidLine, 0.2 );
			pSymbolBasinPoly->changeSymbolLayer( 0, pSLayer4 );
			QgsSingleSymbolRenderer *pRendererBasinPoly = new QgsSingleSymbolRenderer( pSymbolBasinPoly );
			pLayerBasinPoly->setRenderer( pRendererBasinPoly );

			// legend
			//			QPixmap pixBP = QgsSymbolLayerV2Utils::symbolPreviewPixmap( pRendererBasinPoly->symbols()[0], sizeIcon );
			//			QIcon icoBP = QgsSymbolLayerUtils::symbolPreviewIcon( pSymbolBasinPoly, sizeIcon );
			//			QTreeWidgetItem *pItemBasinPoly = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("支流域界") ) );
			//			pItemBasinPoly->setCheckState( 0, Qt::Checked );
			//			pItemBasinPoly->setData( 0, Qt::UserRole, QVariant( nLayer++ ) );
			////			pItemBasinPoly->setIcon( 0, QIcon( pixBP ) );
			//			pItemBasinPoly->setIcon( 0, icoBP );
			//			treeWidget_legend->insertTopLevelItem( 0, pItemBasinPoly );

			QgsProject::instance()->addMapLayer( pLayerBasinPoly, TRUE );
			m_lstLayers.append( pLayerBasinPoly );
			nLayer++;
		}
		else
		{
			delete pLayerBasinPoly;
		}
	}

	/******************  lu mesh  *******************/

	uriNet.setDataSource( "", "lumesh", "GEOMETRY", "", "OGC_FID" );
#ifdef _DEBUG
	qDebug() << uriNet.database();
#endif
	QgsVectorLayer *pLayerLu = new QgsVectorLayer( uriNet.database() + "|layername=lumesh",
		_tr("当該支流域"), "ogr", options );
	if ( !pLayerLu->isValid() )
	{
		QMessageBox::critical( this, "ERROR", _tr( "土地利用レイヤを開けません" ) );
		delete pLayerLu;
		QApplication::exit( 1 );
	}

	// style 
	QgsSymbol *pSymbolLu = QgsSymbol::defaultSymbol( QgsWkbTypes::PolygonGeometry );
	QgsSimpleFillSymbolLayer *pSLayerLu = new QgsSimpleFillSymbolLayer(
		QColor::fromRgb( 255, 128, 255 ), Qt::SolidPattern, QColor::fromRgb( 255, 255, 192 ), Qt::NoPen, 0.0 );
	pSymbolLu->changeSymbolLayer( 0, pSLayerLu );
	QgsSingleSymbolRenderer *pRendererLu = new QgsSingleSymbolRenderer( pSymbolLu );
	pLayerLu->setRenderer( pRendererLu );
	pLayerLu->setOpacity( 50 );
	pLayerLu->setSubsetString( "OGC_FID = -1" );

	// Add the Vector Layer to the Layer Registry
	QgsProject::instance()->addMapLayer( pLayerLu, FALSE );
	// Add the Layer to the Layer Set
	m_lstLayers.append( pLayerLu );
	m_nLumeshLayerIdx = nLayer++;

	/******************  basin  *******************/

	QgsVectorLayer *pLayerBasin = new QgsVectorLayer( uriNet.database() + "|layername=basin",
		_tr("上流支流域"), "ogr", options );
	if ( !pLayerBasin->isValid() )
	{
		QMessageBox::critical( this, "ERROR", _tr( "支流域ポリゴンを開けません" ) );
		delete pLayerBasin;
		QApplication::exit( 1 );
	}

	// style
	QgsSymbol *pSymbolBasin = QgsSymbol::defaultSymbol( QgsWkbTypes::PolygonGeometry );
	QgsSimpleFillSymbolLayer *pSLayer3 = new QgsSimpleFillSymbolLayer(
		QColor::fromRgb( 255, 128, 255 ), Qt::SolidPattern, QColor::fromRgb( 255, 255, 192 ), Qt::NoPen, 0.0 );
	pSymbolBasin->changeSymbolLayer( 0, pSLayer3 );
	QgsSingleSymbolRenderer *pRendererBasin = new QgsSingleSymbolRenderer( pSymbolBasin );
	pLayerBasin->setRenderer( pRendererBasin );
	pLayerBasin->setOpacity( 50 );
	pLayerBasin->setSubsetString( "gid = -1" );

	// legend
	//	QPixmap pixB = QgsSymbolLayerV2Utils::symbolPreviewPixmap( pRendererBasin->symbols()[0], sizeIcon );
	//	QIcon icoB = QgsSymbolLayerUtils::symbolPreviewIcon( pSymbolBasin, sizeIcon );
	//	QTreeWidgetItem *pItemBasin = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("選択支流域") ) );
	//	pItemBasin->setCheckState( 0, Qt::Checked );
	//	pItemBasin->setData( 0, Qt::UserRole, QVariant( nLayer++ ) );
	////	pItemBasin->setIcon( 0, QIcon(pixB) );
	//	pItemBasin->setIcon( 0, icoB );
	//	m_plistTopItems.append( pItemBasin );
	//	treeWidget_legend->insertTopLevelItem( 0, pItemBasin );

	QgsProject::instance()->addMapLayer( pLayerBasin, FALSE );
	m_lstLayers.append( pLayerBasin );
	m_nBasinLayerIdx = nLayer++;

	//QIcon icoB = QgsSymbolLayerUtils::symbolPreviewIcon( pRendererBasin->symbol(), sizeIcon );
	//QgsLayerTreeViewIndicator *pIndicator = new QgsLayerTreeViewIndicator();
	//pIndicator->setIcon( icoB );
	//treeView_legend->addIndicator( pGroup, pIndicator );

	/**************  check points  *****************/

	//QgsVectorLayer *pLayerBasin = new QgsVectorLayer( uriNet.database() + "|layername=basin",
	//	_tr( "上流支流域" ), "ogr", options );
	QString strCheckPoints( strAppDirPath + "/../../data/MM2009_select_4612.gpkg" );

	if ( QFile::exists( strCheckPoints ) )
	{
		QgsVectorLayer *pLayerCheckPoints = 
			new QgsVectorLayer( strCheckPoints + "|layername=MM2009",
				_tr( "環境基準点" ), "ogr", options );
		if ( pLayerCheckPoints->isValid() )
		{
			// style
			QgsCategoryList listCats;
			QgsFields fields = pLayerCheckPoints->fields();
			int nIndex = fields.indexFromName( _tr("基準・補助" ) );

			//QgsStringMap mapSymbolMain;
			//mapSymbolMain["name"] = "triangle";
			//mapSymbolMain["color"] = "255,0,0";
			//mapSymbolMain["style"] = "solid";
			//mapSymbolMain["style_border"] = "no";
			//mapSymbolMain["size"] = "10.0";
			//mapSymbolMain["size_unit"] = "points";
			//QgsMarkerSymbol *pSymbolMain = QgsMarkerSymbol::createSimple( mapSymbolMain );
			//QgsStringMap mapSymbolSub( mapSymbolMain );
			//mapSymbolSub["color"] = "255,192,0";
			//mapSymbolSub["size"] = "8.0";
			//QgsMarkerSymbol *pSymbolSub = QgsMarkerSymbol::createSimple( mapSymbolSub );

			//QSet<QVariant> set = pLayerCheckPoints->uniqueValues( nIndex );
			//Q_FOREACH( auto s, set )
			//{
			//	qDebug() << s.toInt();
			//}

			QgsSvgMarkerSymbolLayer *pmslMain =
				new QgsSvgMarkerSymbolLayer( ":/svg/refpointmaiin.svg" );
			pmslMain->setSize( 20.0 );
			pmslMain->setOutputUnit( QgsUnitTypes::RenderPoints );
			QgsSymbol *pSymbolMain = 
				QgsMarkerSymbol::defaultSymbol( QgsWkbTypes::PointGeometry );
			pSymbolMain->changeSymbolLayer( 0, pmslMain );
			QgsSvgMarkerSymbolLayer *pmslSub =
				new QgsSvgMarkerSymbolLayer( ":/svg/refpointsub.svg" );
			pmslSub->setSize( 18.0 );
			pmslSub->setOutputUnit( QgsUnitTypes::RenderPoints );
			QgsSymbol *pSymbolSub =
				QgsMarkerSymbol::defaultSymbol( QgsWkbTypes::PointGeometry );
			pSymbolSub->changeSymbolLayer( 0, pmslSub );

			listCats.append( QgsRendererCategory( 1, pSymbolMain, _tr( "基準点" ) ) );
			listCats.append( QgsRendererCategory( 2, pSymbolSub, _tr( "補助点" ) ) );
			QgsCategorizedSymbolRenderer *pRendererCheckPoints =
				new QgsCategorizedSymbolRenderer( _tr( "基準・補助"), listCats );
			pLayerCheckPoints->setRenderer( pRendererCheckPoints );

			QgsProject::instance()->addMapLayer( pLayerCheckPoints );
			m_lstLayers.append( pLayerCheckPoints );
			nLayer++;
		}
		else
		{
			delete pLayerCheckPoints;
		}
	}

	/******************  river network  *******************/

	uriNet.setDataSource( "", "network", "GEOMETRY", "", "OGC_FID" );

	QgsVectorLayer *pLayerRiv = new QgsVectorLayer(
		uriNet.database() + "|layername=network", _tr("河川"), "ogr", options );
	if ( !pLayerRiv->isValid() )
	{
		QMessageBox::critical( this, "ERROR", _tr( "河川ネットワークレイヤを開けません" ) );
		delete pLayerRiv;
		QApplication::exit( 1 );
	}

	// style 
	QgsSymbol *pSymbol = QgsSymbol::defaultSymbol( QgsWkbTypes::LineGeometry );
	QgsSimpleLineSymbolLayer *pSLayer1 = new QgsSimpleLineSymbolLayer( Qt::blue );
	//	pSLayer1->setOffset( 2.0 );
	pSymbol->changeSymbolLayer( 0, pSLayer1 );

	QgsMarkerLineSymbolLayer *pSLayer2 = new QgsMarkerLineSymbolLayer();
	QgsSymbolLayer *pSubSLayer = new QgsSimpleMarkerSymbolLayer(
		QgsSimpleMarkerSymbolLayerBase::ArrowHeadFilled, 2.0, 0.0, QgsSymbol::ScaleArea,
		Qt::blue, Qt::black );
	QgsSymbol *pSubSymbol = QgsSymbol::defaultSymbol( QgsWkbTypes::PointGeometry );
	pSubSymbol->changeSymbolLayer( 0, pSubSLayer );
	pSLayer2->setSubSymbol( pSubSymbol );
	pSLayer2->setInterval( 15.0 );

	pSymbol->appendSymbolLayer( pSLayer2 );

	QgsSingleSymbolRenderer *pRenderer = new QgsSingleSymbolRenderer( pSymbol );
	pLayerRiv->setRenderer( pRenderer );

	// legend
	//	QPixmap pixR = QgsSymbolLayerV2Utils::symbolPreviewPixmap( pRenderer->symbols()[0], sizeIcon );
	//	QIcon icoR = QgsSymbolLayerUtils::symbolPreviewIcon( pSymbol, sizeIcon );
	//	QTreeWidgetItem *pItemRiv = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("河川") ) );
	//	pItemRiv->setCheckState( 0, Qt::Checked );
	//	pItemRiv->setData( 0, Qt::UserRole, QVariant( nLayer++ ) );
	////	pItemRiv->setIcon( 0, QIcon(pixR) );
	//	pItemRiv->setIcon( 0, icoR );
	//	m_plistTopItems.append( pItemRiv );
	//	treeWidget_legend->insertTopLevelItem( 0, pItemRiv );

	// Add the Vector Layer to the Layer Registry
	//	QgsMapLayerRegistry::instance()->addMapLayer( pLayerRiv, TRUE );
	QgsProject::instance()->addMapLayer( pLayerRiv, true );
	// Add the Layer to the Layer Set
	m_lstLayers.append( pLayerRiv );
	m_nNetworkLayerIdx = nLayer;

	// connect checksignal - slot from legend tree
	//QObject::connect( treeWidget_legend, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
	//	this, SLOT(toggleLayerVisible(QTreeWidgetItem *, int)) );

	/*************  sync basin layers  *************/

	//QgsLayerTreeLayer *pBasinLayer = QgsProject::instance()->layerTreeRoot()->findLayer( pLayerBasin );
	//QObject::connect(
	//	pBasinLayer, SIGNAL(visibilityChanged(QgsLayerTreeNode *)),
	//		this, SLOT(toggleLayerVisible(QgsLayerTreeNode *) ) );

	/******************  group    *******************/

	QgsLayerTreeGroup *pGroup =
		QgsProject::instance()->layerTreeRoot()->insertGroup( 2, _tr("検索支流域") );
	pGroup->addLayer( pLayerLu );
	pGroup->addLayer( pLayerBasin );
	/*QgsLayerTreeGroup *pTreeGroup = 
		QgsProject::instance()->layerTreeRoot()->findGroup( _tr("検索支流域") );*/
	pGroup->setItemVisibilityCheckedRecursive( TRUE );

	treeView_legend->collapseAll();

	//QgsLayerTreeModel *pModel = treeView_legend->layerTreeModel();
	//QModelIndex nGroupIndex = pModel->node2index( pGroup );
	//treeView_legend->layerTreeModel()->setData(
	//	nGroupIndex, QVariant(icoB), Qt::DecorationRole
	//);
}


#if 0
void MyMainWindow::SetupLegend()
{
	QgsLegendSymbologyList listSymbol;
	QSize sizeIcon( 16, 16 );
	QgsVectorLayer *pLayerRiv, *pLayerBasin, *pLayerBasinPoly;
	QgsFeatureRendererV2 *pRenderer;

	treeWidget_legend->setHeaderLabel( tr("凡例") );

	// river network
	pLayerRiv = dynamic_cast<QgsVectorLayer *>(m_lstLayers[0].layer());
	pRenderer = pLayerRiv->rendererV2();
	QPixmap pixR = QgsSymbolLayerV2Utils::symbolPreviewPixmap( pRenderer->symbols()[0], sizeIcon );
	QTreeWidgetItem *pItemRiv = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("河川") ) );
	pItemRiv->setCheckState( 0, Qt::Checked );
	pItemRiv->setData( 0, Qt::UserRole, QVariant( NETWORK_LAYER_NO ) );
	pItemRiv->setIcon( 0, QIcon(pixR) );
	m_plistTopItems.append( pItemRiv );
	treeWidget_legend->insertTopLevelItem( 0, pItemRiv );

	// basin
	pLayerBasin = dynamic_cast<QgsVectorLayer *>(m_lstLayers[1].layer());
	pRenderer = pLayerBasin->rendererV2();
	QPixmap pixB = QgsSymbolLayerV2Utils::symbolPreviewPixmap( pRenderer->symbols()[0], sizeIcon );
	QTreeWidgetItem *pItemBasin = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("選択支流域") ) );
	pItemBasin->setCheckState( 0, Qt::Checked );
	pItemBasin->setData( 0, Qt::UserRole, QVariant( BASIN_LAYER_NO ) );
	pItemBasin->setIcon( 0, QIcon(pixB) );
	m_plistTopItems.append( pItemBasin );
	treeWidget_legend->insertTopLevelItem( 0, pItemBasin );

	// basin poly
	QList<QgsMapLayer *> lstBasinPoly = QgsMapLayerRegistry::instance()->mapLayersByName( "basin_poly" );
	if ( lstBasinPoly.size() > 0 )
	{
		pLayerBasinPoly = dynamic_cast<QgsVectorLayer *>( lstBasinPoly[0] );
		pRenderer = pLayerBasinPoly->rendererV2();
		QPixmap pixBP = QgsSymbolLayerV2Utils::symbolPreviewPixmap( pRenderer->symbols()[0], sizeIcon );
		QTreeWidgetItem *pItemBasinPoly = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("支流域界") ) );
		pItemBasinPoly->setCheckState( 0, Qt::Checked );
		pItemBasinPoly->setData( 0, Qt::UserRole, QVariant( BASIN_LAYER_OUTLINE_NO ) );
		pItemBasinPoly->setIcon( 0, QIcon( pixBP ) );
		treeWidget_legend->insertTopLevelItem( 0, pItemBasinPoly );
	}

	// lu raster
	QList<QgsMapLayer *> lstLuRaster = QgsMapLayerRegistry::instance()->mapLayersByName( "luraster" );
	if ( lstLuRaster.size() > 0 )
	{
		QTreeWidgetItem *pItemLuRaster = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("土地利用図") ) );
		pItemLuRaster->setCheckState( 0, Qt::Unchecked );
		pItemLuRaster->setData( 0, Qt::UserRole, QVariant( LURASTER_LAYER_NO ) );
		pItemLuRaster->setIcon( 0, QIcon(":/icons/mIconRaster.png") );
		m_plistTopItems.append( pItemLuRaster );
		treeWidget_legend->insertTopLevelItem( 0, pItemLuRaster );
		QObject::connect( treeWidget_legend, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
			this, SLOT(enableRasterOpacitySlider(QTreeWidgetItem *, int)) );
	}

	// 背景図
	QTreeWidgetItem *pItemBGMap = new QTreeWidgetItem( (QTreeWidgetItem *)NULL, QStringList( tr("背景図") ) );
	pItemBGMap->setCheckState( 0, Qt::Checked );
	pItemBGMap->setData( 0, Qt::UserRole, QVariant( BGMAP_LAYER_NO ) );
	pItemBGMap->setIcon( 0, QIcon(":/icons/mIconRaster.png") );
	m_plistTopItems.append( pItemBGMap );
	treeWidget_legend->insertTopLevelItem( 0, pItemBGMap );

	// connect checksignal - slot from legend tree
	QObject::connect( treeWidget_legend, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
		this, SLOT(toggleLayerVisible(QTreeWidgetItem *, int)) );
}
#endif


void MyMainWindow::SetupLuTable()
{
	m_pSplash->showMessage( _tr("土地利用テーブルを準備しています"), Qt::AlignLeft | Qt::AlignBottom, Qt::yellow);
	qApp->processEvents();

	QueryManage *q = QueryManage::instance();
	if ( !q->isOpened() )
	{
		qApp->exit( 1 );
	}

	QObject::connect( q, SIGNAL(sqlError(QString)), this, SLOT(showError(QString)) );

	QVector<QueryManage::LUINFO> vLuInfo = q->getLuList();

//	for ( int i = 0; i < vstrLuName.size(); i++ )
	for ( auto info : vLuInfo )
	{
		if ( info.nParentItem == 0 )
		{
			QTreeWidgetItem *pItem =
				new QTreeWidgetItem(treeWidget_result, QStringList(info.strLuName), 2);
			pItem->setData( 0, Qt::UserRole, QVariant(info.nID) );
			treeWidget_result->addTopLevelItem(pItem);
		}
		else
		{
			auto infoParent = std::find( vLuInfo.begin(), vLuInfo.end(), info.nParentItem );
			QList<QTreeWidgetItem *> listParent = treeWidget_result->findItems
				( infoParent->strLuName, Qt::MatchExactly | Qt::MatchRecursive );
			if ( !listParent.size() )
			{
				QMessageBox::critical( this, "ERROR", "Parent Item not found.");
			}
			else
			{
				//			QTreeWidgetItem *pItemParent = treeWidget_result->topLevelItem( info.nParentItem );
				QTreeWidgetItem *pItemParent = listParent[0];
				QTreeWidgetItem *pItem =
					new QTreeWidgetItem(pItemParent, QStringList(info.strLuName), 2);
				pItemParent->addChild(pItem);
			}
		}
//		treeWidget_result->setItem( i, 0, new QTreeWidgetItem( vstrLuName[i] ) );
	}

	treeWidget_result->resizeColumnToContents( 0 );

	QObject::disconnect( q, SIGNAL(sqlError(QString)), 0, 0 );
}


//void MyMainWindow::toggleLayerVisible( QTreeWidgetItem *pItem, int nColumn )
//{
//	int nLayer = pItem->data( 0, Qt::UserRole ).toInt();
//	bool bChecked = pItem->checkState( 0 );
//
//	m_lstLayers[nLayer].setVisible( bChecked );
//	if ( m_lstLayers[nLayer].layer()->name() == "basin" )
//	{
//		m_lstLayers[nLayer+1].setVisible( bChecked );
//	}
//
//	centralwidget->setLayerSet( m_lstLayers );
////	centralwidget->refresh();
//}
//void MyMainWindow::toggleLayerVisible( QgsLayerTreeNode *node )
//{
//	QgsLayerTreeLayer *pTreeLayer = dynamic_cast<QgsLayerTreeLayer *>( node );
//	if ( pTreeLayer )
//	{
//		bool bCheck = pTreeLayer->itemVisibilityChecked();
//
//		QgsMapLayer *pLayer = pTreeLayer->layer();
//		if ( pLayer->name() == "basin" )
//		{
//			QgsLayerTreeLayer *p = QgsProject::instance()->layerTreeRoot()->findLayer( m_lstLayers[m_nBasinLayerIdx] );
//			p->setItemVisibilityChecked( bCheck );
//		}
//	}
//}


void MyMainWindow::toolPan( bool bChecked )
{
	if ( bChecked )
	{
		centralwidget->setMapTool( m_pMapToolPan );
	}
}


void MyMainWindow::toolZoomIn( bool bChecked )
{
	if ( bChecked )
	{
		centralwidget->setMapTool( m_pMapToolZoomIn );
	}
}


void MyMainWindow::toolZoomOut( bool bChecked )
{
	if ( bChecked )
	{
		centralwidget->setMapTool( m_pMapToolZoomOut );
	}
}


void MyMainWindow::toolSelect( bool bChecked )
{
	if ( bChecked )
	{
		centralwidget->setMapTool( m_pMapToolSelect );
	}
}


void MyMainWindow::toolZoomReset()
{
	centralwidget->setExtent( m_pCoordinateTrans->transform( m_lstLayers[m_nNetworkLayerIdx]->extent(), QgsCoordinateTransform::ForwardTransform ) );
	centralwidget->refresh();
}


//void MyMainWindow::enableRasterOpacitySlider( QTreeWidgetItem *pItem, int nCol )
//{
//	QgsMapCanvasLayer mapLayer = m_lstLayers[pItem->data( 0, Qt::UserRole).toInt()];
//	qDebug() << mapLayer.layer()->name();
//	mapLayer.layer()->name() == "luraster" ?
//	horizontalSlider_rasterOpacity->setEnabled( true ) :
//	horizontalSlider_rasterOpacity->setEnabled( false );
//}


void MyMainWindow::enableRasterOpacitySlider( QgsMapLayer *pLayer )
{
	(pLayer && pLayer->name() == _tr( "土地利用図" )) ?
		horizontalSlider_rasterOpacity->setEnabled( true ) :
		horizontalSlider_rasterOpacity->setEnabled( false );
}


void MyMainWindow::changeRasterOpacity( int nOpacity )
{
//	QgsRasterLayer *pLayerLu = dynamic_cast<QgsRasterLayer *>( QgsMapLayerRegistry::instance()->mapLayersByName( "luraster" )[0] );

	if ( treeView_legend->currentLayer() == m_lstLayers[m_nRasterLayerIdx] )
	{
		QgsRasterLayer *pLayerLu = dynamic_cast<QgsRasterLayer *>( m_lstLayers[m_nRasterLayerIdx] );
		pLayerLu->renderer()->setOpacity( (double)nOpacity * 0.01 );
		centralwidget->refresh();
	}
}


void MyMainWindow::annotationCreated( QgsAnnotation *annotation )
{
	QgsMapCanvasAnnotationItem *canvasItem = 
		new QgsMapCanvasAnnotationItem( annotation, centralwidget );
	Q_UNUSED( canvasItem ); //item is already added automatically to canvas scene
}


void MyMainWindow::aggregateMesh( QgsPointXY &pntPos )
{
	m_pntLastPos = m_pCoordinateTrans->transform( pntPos, QgsCoordinateTransform::ReverseTransform );
	qApp->setOverrideCursor( Qt::WaitCursor );
	qApp->installEventFilter( m_pFilter );

	// add point annotation
	if ( !m_pPointAnnotation )
	{
		m_pPointAnnotation = new QgsSvgAnnotation();
		m_pPointAnnotation->setFilePath( ":/svg/point.svg" );
		m_pPointAnnotation->setFrameSize( QSize( 30, 30 ) );
		//m_pPointAnnotation->setRelativePosition( QPointF( 0.0, -0.03 ) );
		m_pPointAnnotation->setFrameOffsetFromReferencePoint( QPointF( 0.0, -30.0 ) );
		QgsStringMap mapMarkerProp;
		mapMarkerProp["color"] = "255,255,0";
		mapMarkerProp["size"] = "1.5";
		mapMarkerProp["outline_width"] = "0.2";
		mapMarkerProp["outline_color"] = "red";
		QgsMarkerSymbol *pSymbol = QgsMarkerSymbol::createSimple( mapMarkerProp );
		m_pPointAnnotation->setMarkerSymbol( pSymbol );
	}
	m_pPointAnnotation->setMapPosition( pntPos );
//	m_pPointAnnotation->setMapPosition( m_pntLastPos );
	m_pPointAnnotation->setMapPositionCrs( centralwidget->mapSettings().destinationCrs() );
	//m_pPointAnnotation->setOffsetFromReferencePoint( QPointF(0.0, -30.0) );

	QgsProject::instance()->annotationManager()->addAnnotation( m_pPointAnnotation );

#ifdef _DEBUG
	centralwidget->annotationItems();
#endif

	//centralwidget->setExtent(
	//	QgsRectangle( 
	//	m_pPointAnnotation->mapPosition().x() - 100,
	//	m_pPointAnnotation->mapPosition().y() - 100,
	//	m_pPointAnnotation->mapPosition().x() + 100,
	//	m_pPointAnnotation->mapPosition().y() + 100 ) );
	//centralwidget->refresh();

	QueryManage *q = QueryManage::instance();

	q->setCheckDist( true );
	q->setCondition( m_pntLastPos );

	QObject::connect( q, SIGNAL(finished()), this, SLOT(onQueryFinished()) );
	QObject::connect( q, SIGNAL(sqlError(QString)), this, SLOT(showError(QString)) );
	statusbar->showMessage( _tr("検索中...") );
	q->start();
}


void MyMainWindow::onQueryFinished()
{
	QueryManage *q = QueryManage::instance();
	QObject::disconnect( q, SIGNAL(finished()), 0, 0 );
	QObject::disconnect( q, SIGNAL(sqlError(QString)), 0, 0 );

	if ( !q->isQueryed() )
	{
		while ( qApp->overrideCursor() )
			qApp->restoreOverrideCursor();
		qApp->removeEventFilter( m_pFilter );

		statusbar->showMessage( _tr("解析範囲外") );
		return;
	}

	// clear old items;
	resetResultTree();

	// point
	lineEdit_lat->setText( QString::number( m_pntLastPos.y(), 'f', 6 ) );
	lineEdit_lon->setText( QString::number( m_pntLastPos.x(), 'f', 6  ) );

	QMap<int, double> vResult = q->getQueryResult();
	QVector<QueryManage::LUINFO> vFieldInfo = q->getLuList();
	QVector<double> vProp;

	// total meshs.
	double dTotal = 0;
	for ( QMap<int, double>::iterator p = vResult.begin(); p != vResult.end(); p++ )
	{
		QueryManage::LUINFO *pInfo = std::find(vFieldInfo.begin(), vFieldInfo.end(), p.key());
		if ( pInfo->nParentItem == 0 )
		{
			dTotal += p.value();
		}
	}

	// set values
	for ( QMap<int, double>::iterator p = vResult.begin(); p != vResult.end(); p++ )
	{
		//QTableWidgetItem *pItem = new QTableWidgetItem( QString::number(vResult[i]) );
		//treeWidget_result->setItem( i, 1, pItem );
		//QTableWidgetItem *pItem2 = new QTableWidgetItem( QString::number((double)vResult[i]*100.0/nTotal) );
		//tableWidget_result->setItem( i, 2, pItem2 );
		//QTreeWidgetItem *pItem = treeWidget_result->topLevelItem(i);
		QueryManage::LUINFO *pInfo = std::find( vFieldInfo.begin(), vFieldInfo.end(), p.key() );
		QString strFieldName = pInfo->strLuName;
		QList<QTreeWidgetItem *> listItem = 
			treeWidget_result->findItems( strFieldName, Qt::MatchExactly | Qt::MatchRecursive );
		if ( listItem.size() )
		{
			listItem[0]->setText( 1, QString::number(p.value(), 'f', 4) );
			listItem[0]->setText( 2, QString::number(p.value()*100.0/dTotal, 'f', 4) );
		}
		else
			QMessageBox::critical( this, "ERROR", "tree item not found." );
	}

	// total crops and orchard
	QTreeWidgetItem *pItemCrops = 
		treeWidget_result->findItems( _tr("普通畑、その他畑"), Qt::MatchExactly | Qt::MatchRecursive )[0];
	double dTotalCrops = 0.0;
	for ( int i = 0; i < pItemCrops->childCount(); i++ )
	{
		dTotalCrops += pItemCrops->child( i )->text( 1 ).toDouble();
	}
	pItemCrops->setText( 1, QString::number(dTotalCrops) );
	pItemCrops->setText( 2, QString::number(dTotalCrops*100.0 /dTotal) );
	QTreeWidgetItem *pItemOrch =
		treeWidget_result->findItems(_tr("果樹"), Qt::MatchExactly | Qt::MatchRecursive)[0];
	double dTotalOrch = 0.0;
	for ( int i = 0; i < pItemOrch->childCount(); i++ )
	{
		dTotalOrch += pItemOrch->child( i )->text( 1 ).toDouble();
	}
	pItemOrch->setText( 1, QString::number(dTotalOrch, 'f', 4 ) );
	pItemOrch->setText( 2, QString::number(dTotalOrch*100.0/dTotal, 'f', 4) );

	// searched basins;
	QString strSubsetString( "" );
	QVector<int> vSearchedBasins = q->getSearchedBasins();
	if ( !vSearchedBasins.size() )
	{
		strSubsetString = "gid = -1";
	}
	else
	{
		strSubsetString = "gid in (";
		for ( int i = 0; i < vSearchedBasins.size(); i++ )
		{
			strSubsetString += QString::number( vSearchedBasins[i] );
			if ( i != vSearchedBasins.size() - 1 )
				strSubsetString += ",";
			else
				strSubsetString += ")";
		}
	}

	// なぜか２回設定しないと反映されない
	qDebug() << dynamic_cast<QgsVectorLayer *>( m_lstLayers[m_nBasinLayerIdx] )->setSubsetString( strSubsetString );
//	qDebug() << dynamic_cast<QgsVectorLayer *>( m_lstLayers[nBasinIndex] )->setSubsetString( strSubsetString );
	qDebug() << dynamic_cast<QgsVectorLayer *>( m_lstLayers[m_nBasinLayerIdx] )->subsetString();

	// searched meshes;
	QString strSubsetStringMesh( "" );
	QVector<int> vSearchedMeshes = q->getSearchedMeshes();
	if ( !vSearchedMeshes.size() )
	{
		strSubsetStringMesh = "OGC_FID = -1";
	}
	else
	{
		strSubsetStringMesh = "OGC_FID in (";
		for ( int i = 0; i < vSearchedMeshes.size(); i++ )
		{
			strSubsetStringMesh += QString::number( vSearchedMeshes[i] );
			if ( i != vSearchedMeshes.size() - 1 )
			{
				strSubsetStringMesh += ",";
			}
			else
			{
				strSubsetStringMesh += ")";
			}
		}
	}
	// なぜか２回設定しないと反映されない
	qDebug() << dynamic_cast<QgsVectorLayer *>( m_lstLayers[m_nLumeshLayerIdx] )->setSubsetString( strSubsetStringMesh );
//	qDebug() << dynamic_cast<QgsVectorLayer *>( m_lstLayers[nLumeshIndex] )->setSubsetString( strSubsetStringMesh );
	qDebug() << dynamic_cast<QgsVectorLayer *>( m_lstLayers[m_nLumeshLayerIdx] )->subsetString();

	//QFile fileTemp( "D:\\temp\\subset.txt" );
	//fileTemp.open( QFile::WriteOnly | QFile::Truncate );
	//QTextStream strm( &fileTemp );
	//strm << dynamic_cast<QgsVectorLayer *>(m_lstLayers[nLumeshIndex])->subsetString();
	//fileTemp.close();

	lineEdit_rivname->setText( q->getRivName() );

	while ( qApp->overrideCursor() )
		qApp->restoreOverrideCursor();
	qApp->removeEventFilter( m_pFilter );

	statusbar->showMessage( _tr("検索完了") );
	m_lstLayers[m_nLumeshLayerIdx]->reload();
	m_lstLayers[m_nBasinLayerIdx]->reload();
	centralwidget->refresh();
	//treeView_legend->refreshLayerSymbology( m_lstLayers[nLumeshIndex]->id() );
	//treeView_legend->refreshLayerSymbology( m_lstLayers[nBasinIndex]->id() );

	QgsLayerTreeGroup *pTreeGroup =
		QgsProject::instance()->layerTreeRoot()->findGroup( _tr( "検索支流域" ) );
	pTreeGroup->setItemVisibilityCheckedRecursive( TRUE );

	// force update layer visibility.
	mLayerTreeCanvasBridge->setCanvasLayers();

	//treeView_legend->layerTreeModel()->refreshLayerLegend(
	//	QgsProject::instance()->layerTreeRoot()->findLayer( m_lstLayers[m_nBasinLayerIdx] )
	//);
	//auto *pBasinTreeLayer = 
	//	QgsProject::instance()->layerTreeRoot()->findLayer( m_lstLayers[m_nBasinLayerIdx] );
	//pBasinTreeLayer->setItemVisibilityChecked( TRUE );
}


void MyMainWindow::showError(QString strError)
{
	QMessageBox::warning( this, "ERROR", strError );
}


void MyMainWindow::onExportResult()
{
	QString strFilter = tr("Comma Separated Value(*.csv)");
	QString strSelected("");
	QString strFName = QFileDialog::getSaveFileName( this, _tr( "結果をエクスポート" ), ".", strFilter, &strSelected );

	if ( !strFName.isEmpty() )
	{
		QFile fileCsv( strFName );
		if ( fileCsv.open( QFile::WriteOnly | QFile::Truncate ) )
		{
			QTextStream strm( &fileCsv );
			strm << _tr("指定地点座標") << "," << m_pntLastPos.x() << "," << 
				m_pntLastPos.y() << "," << lineEdit_rivname->text() << "\n" << endl;
			strm << _tr("土地利用集計結果：") << ",(ha),(%)" << endl;
			for ( int i = 0; i < treeWidget_result->topLevelItemCount(); i++ )
			{
				QTreeWidgetItem *pItem = treeWidget_result->topLevelItem( i );
				strm
					<< pItem->text( 0 ) << ","
					<< pItem->text( 1 ) << ","
					<< pItem->text( 2 ) << endl;

				for ( int j = 0; j < pItem->childCount(); j++ )
				{
					QTreeWidgetItem *pItemChild = pItem->child( j );
					for ( int k = 0; k < pItemChild->childCount(); k++ )
					{
						QTreeWidgetItem *pItemGChild = pItemChild->child( k );
						strm
							<< _tr( "┣ ") << pItemGChild->text( 0 ) << ","
							<< pItemGChild->text( 1 ) << ","
							<< pItemGChild->text( 2 ) << endl;
					}
				}
				//strm 
				//	<< tableWidget_result->item( i, 0 )->text() << "," 
				//	<< tableWidget_result->item( i, 1 )->text() << ","
				//	<< tableWidget_result->item( i, 2 )->text() << endl;
			}
		}
		fileCsv.close();
		QMessageBox::information( this, _tr("エクスポート完了"), _tr("エクスポートが完了いたしました。") );
	}
}


void MyMainWindow::onFileOpen()
{
	DlgPointCsv dlgPointCsv;

	if ( dlgPointCsv.exec() == QDialog::Accepted )
	{
		if ( dlgPointCsv.lineEdit_input->text().isEmpty() || dlgPointCsv.lineEdit_output->text().isEmpty() )
		{
			QMessageBox::information( this, "WARNING", _tr("ファイルが指定されていません") );
			return;
		}

		QFile fileInput( dlgPointCsv.lineEdit_input->text() );
		if ( !fileInput.open( QFile::ReadOnly ) )
		{
			QMessageBox::warning( this, "ERROR", _tr("ファイルを開けません[%1]").arg( fileInput.fileName() ) );
			return;
		}
		QFile fileOutput( dlgPointCsv.lineEdit_output->text() );
		if ( !fileOutput.open( QFile::WriteOnly | QFile::Truncate ) )
		{
			QMessageBox::warning( this, "ERROR", _tr("ファイルを作成できません[%1]").arg( fileOutput.fileName() ) );
			fileInput.close();
			return;
		}

		qApp->setOverrideCursor( Qt::WaitCursor );
		qApp->installEventFilter( m_pFilter );

		QueryManage *q = QueryManage::instance();
		q->setCheckDist( false );
//		QObject::connect( q, SIGNAL(sqlError(QString)), this, SLOT(showError(QString)) );

		QTextStream strmInput( &fileInput );
		QTextStream strmOutput( &fileOutput );

		// scan line number;
		int nLines = 0;
		while ( !strmInput.atEnd() )
		{
			strmInput.readLine();
			nLines++;
		}
		strmInput.seek( SEEK_SET );

		// check header
		int nIDCol = -1, nLonCol = -1, nLatCol = -1;
		QStringList strlHeader = strmInput.readLine().split( ',' );
		for ( int i = 0; i < strlHeader.size(); i++ )
		{
			if ( !strlHeader[i].compare( "ID", Qt::CaseInsensitive ) ||
				 !strlHeader[i].compare( _tr("番号"), Qt::CaseInsensitive ) )
			{
				nIDCol = i;
			}
			else if (
				!strlHeader[i].compare( "LON", Qt::CaseInsensitive ) ||
				!strlHeader[i].compare( "L", Qt::CaseInsensitive ) ||
				!strlHeader[i].compare( _tr("経度"), Qt::CaseInsensitive ) )
			{
				nLonCol = i;
			}
			else if (
				!strlHeader[i].compare( "LAT", Qt::CaseInsensitive ) ||
				!strlHeader[i].compare( "B", Qt::CaseInsensitive ) ||
				!strlHeader[i].compare( _tr("緯度"), Qt::CaseInsensitive ) )
			{
				nLatCol = i;
			}
		}
		if ( nIDCol == -1 || nLonCol == -1 || nLatCol == -1 )
		{
			QMessageBox::warning( this, "WARNING", _tr("入力ファイルのフォーマットが正しくありません") );
			fileInput.close();
			return;
		}

		// read data
		QProgressBar progressBar;
		progressBar.setMinimum( 0 );
		progressBar.setMaximum( nLines );
		statusbar->addWidget( &progressBar );

		// write header row;
		QVector<QueryManage::LUINFO> vFieldInfo = q->getLuList();
		strmOutput << strlHeader[nIDCol] << "," << strlHeader[nLonCol] << "," << strlHeader[nLatCol] << ",";
//		for ( int i = 0; i < vLuInfo.size(); i++ )
		for ( auto info : vFieldInfo )
		{
			strmOutput << info.strLuName << ",";
		}
		strmOutput << _tr("河川名") <<endl;

		// write data row
		nLines = 0;
		while ( !strmInput.atEnd() )
		{
			QStringList lstPnt = strmInput.readLine().split( "," );
			QgsPointXY pnt( lstPnt[nLonCol].toDouble(), lstPnt[nLatCol].toDouble() );
			QVector<double> vResult( vFieldInfo.size(), 0.0 );

			strmOutput << lstPnt[nIDCol] << ",";

			// verify point value
			if ( pnt.x() < 130.0 || pnt.x() > 150.0 ||
					pnt.y() < 20.0 || pnt.y() > 50.0 )
			{
				strmOutput << _tr("解析範囲外") << endl;
				continue;
			}

			// query
			q->setCondition( pnt );
			q->start();
			q->wait();

			strmOutput << QString::number( pnt.x(), 'f', 6 )  << "," << QString::number( pnt.y(), 'f', 6 ) << ",";

			if ( !q->isQueryed() )
			{
				strmOutput << _tr("解析範囲外") << endl;
				continue;
			}

			// get result
			QMap<int, double> mapResult = q->getQueryResult();
			qDebug() << vResult.size();
//			for ( auto res : mapResult )
			for ( QMap<int, double>::iterator p = mapResult.begin(); p != mapResult.end(); p++ )
			{
				QueryManage::LUINFO *pInfo = std::find( vFieldInfo.begin(), vFieldInfo.end(), p.key() );
				vResult[pInfo->nID-1] = p.value();
			}
			std::for_each( vResult.begin(), vResult.end(),
						   [&]( double d ){
							   strmOutput << QString::number( d, 'f', 4 ) << ",";
						   }
			);
			strmOutput << q->getRivName() << endl;
			progressBar.setValue( ++nLines );

			QApplication::processEvents();
		}

		while ( qApp->overrideCursor() )
			qApp->restoreOverrideCursor();
		qApp->removeEventFilter( m_pFilter );

		statusbar->removeWidget( &progressBar );
		fileInput.close();

		QMessageBox::information( this, _tr("完了"), _tr("集計が完了しました") );
	}
}


void MyMainWindow::onAppAbout()
{
	QDialog dlg;
	QueryManage *q = QueryManage::instance();
	
	Ui::ui_AboutDialog ui;
	ui.setupUi( &dlg );

//	QString strAbout = ui.textEdit_copyright->toHtml();
	ui.textEdit_copyright->moveCursor( QTextCursor::End );
	QString strAbout = "<br /><hr /><p>QGIS Version       : " + Qgis::QGIS_VERSION +  "  " + Qgis::QGIS_RELEASE_NAME + "</p>";
	strAbout += "</p><p>Qt version : ";
	strAbout += qVersion();
	strAbout += "</p><p>Spatialite Version : ";
	strAbout += q->_spatialite_version();
	strAbout += "</p>";
	ui.textEdit_copyright->insertHtml( strAbout );

	dlg.exec();
}


void MyMainWindow::showDocumentation()
{
	if ( !m_pAssistant )
		m_pAssistant = new MyAssistant();

	m_pAssistant->showDocumentation( "index.html" );

//	qDebug( QString("file://" + QgsApplication::pkgDataPath() + "/doc/index.html").toLocal8Bit() );
/*
	QString strDocPath = QgsApplication::pkgDataPath() + "/doc/index.html";

	statusbar->showMessage( strDocPath );

	QDesktopServices::openUrl( "file:///" + strDocPath );
*/
}

/*
void MyMainWindow::onFrameSymbol()
{
	QgsVectorLayer *pLayer = dynamic_cast<QgsVectorLayer *>( centralwidget->layer( 0 ) );
	if ( pLayer && pLayer->geometryType() == QGis::Polygon )
	{
		QgsSimpleFillSymbolLayerV2 *psLayer = new QgsSimpleFillSymbolLayerV2
			( QColor::fromRgb( 0, 0, 150 ), Qt::NoBrush, QColor::fromRgb( 0, 0, 150 ), Qt::SolidLine, 1.0 );
		QgsSymbolV2 *pSymbol = QgsSymbolV2::defaultSymbol( QGis::Polygon );
		pSymbol->changeSymbolLayer( 0, psLayer );
		//pSymbol->appendSymbolLayer( sLayer );
		QgsSingleSymbolRendererV2 *pRenderer = new QgsSingleSymbolRendererV2( pSymbol );
		pLayer->setRendererV2( pRenderer );

		centralwidget->refresh();
	}
}


void MyMainWindow::onCategorySymbol()
{
	QgsVectorLayer *pLayer = dynamic_cast<QgsVectorLayer *>( centralwidget->layer( 0 ) );
	if ( pLayer )
	{
		QgsCategoryList lstCats;
//		QList<QVariant> lstUniqueVals;
//		lstUniqueVals.reserve( 100*sizeof(QVariant) );
		pLayer->uniqueValues( pLayer->fieldNameIndex( "id" ), lstUniqueVals );
		QgsVectorColorRampV2 *pRamp = new QgsVectorGradientColorRampV2();
		QgsSymbolV2 *pSymbol = QgsSymbolV2::defaultSymbol( pLayer->geometryType() );
		for ( int i = 0; i < lstUniqueVals.count(); i++ )
		{
			QgsSymbolV2 *pNewSymbol = pSymbol->clone();
			QVariant val = lstUniqueVals[i];
			if ( i < 0 )
			{
				QgsLinePatternFillSymbolLayer *psLayer = new QgsLinePatternFillSymbolLayer();
				psLayer->setDistance( 10.0 );
				psLayer->setLineAngle( 45.0 );
				psLayer->setLineWidth( 0.1 );

				pNewSymbol->changeSymbolLayer( 0, psLayer );
			}
			else
			{
				double d = i / (double)lstUniqueVals.count();
				pNewSymbol->setColor( pRamp->color( d ) );
			}
			lstCats.append( QgsRendererCategoryV2( val, pNewSymbol, val.toString() ) );
		}

		QgsCategorizedSymbolRendererV2 *psLayer = new QgsCategorizedSymbolRendererV2( "id", lstCats );
		pLayer->setRendererV2( psLayer );

		centralwidget->refresh();
	}
}


void MyMainWindow::onSvgSymbol()
{
	QgsVectorLayer *pLayer = dynamic_cast<QgsVectorLayer *>( centralwidget->layer( 0 ) );
	if ( pLayer && pLayer->geometryType() == QGis::Point )
	{
		QgsSvgMarkerSymbolLayerV2 *pmsl = new QgsSvgMarkerSymbolLayerV2();
		pmsl->setPath( ":/svg/bozu.svg" );
//		pmsl->setPath( "C:/OSGeo4W/apps/qgis/svg/shogi/kaku.svg" );
		pmsl->setSize( 10.0 );
		QgsSymbolV2 *pSymbol = QgsSymbolV2::defaultSymbol( QGis::Point );
		pSymbol->changeSymbolLayer( 0, pmsl );
		QgsSingleSymbolRendererV2 *pRenderer = new QgsSingleSymbolRendererV2( pSymbol );
		pLayer->setRendererV2( pRenderer );

		centralwidget->refresh();
	}
}


void MyMainWindow::onMultiLine()
{
	QgsVectorLayer *pLayer = dynamic_cast<QgsVectorLayer *>( centralwidget->layer( 0 ) );
	if ( pLayer && pLayer->geometryType() == QGis::Line )
	{
		QgsSymbolV2 *pSymbol = QgsSymbolV2::defaultSymbol( QGis::Line );
		QgsSimpleLineSymbolLayerV2 *pSLayer1 = new QgsSimpleLineSymbolLayerV2( Qt::black );
		pSLayer1->setOffset( 2.0 );
		pSymbol->changeSymbolLayer( 0, pSLayer1 );

		QgsMarkerLineSymbolLayerV2 *pSLayer2 = new QgsMarkerLineSymbolLayerV2();
		QgsSymbolLayerV2 *pSubSLayer = new QgsSimpleMarkerSymbolLayerV2( "triangle", Qt::black, Qt::black, 4.0 );
		QgsSymbolV2 *pSubSymbol = QgsSymbolV2::defaultSymbol( QGis::Point );
		pSubSymbol->changeSymbolLayer( 0, pSubSLayer );
		pSLayer2->setSubSymbol( pSubSymbol );
		pSLayer2->setInterval( 10.0 );

		pSymbol->appendSymbolLayer( pSLayer2 );

		QgsSingleSymbolRendererV2 *pRenderer = new QgsSingleSymbolRendererV2( pSymbol );
		pLayer->setRendererV2( pRenderer );

		centralwidget->refresh();
	}
}


void MyMainWindow::onSimpleMarker()
{
	QgsVectorLayer *pLayer = dynamic_cast<QgsVectorLayer *>( centralwidget->layer( 0 ) );
	if ( pLayer && pLayer->geometryType() == QGis::Point )
	{
		QgsSymbolV2 *pSymbol = QgsSymbolV2::defaultSymbol( QGis::Point );

		QgsSimpleMarkerSymbolLayerV2 *pSLayer = new QgsSimpleMarkerSymbolLayerV2( "pentagon", Qt::darkBlue, Qt::red, 10 );
		pSymbol->changeSymbolLayer( 0, pSLayer );

		QgsSingleSymbolRendererV2 *pRenderer = new QgsSingleSymbolRendererV2( pSymbol );
		pLayer->setRendererV2( pRenderer );

		centralwidget->refresh();
	}
}
*/
