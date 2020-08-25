/***************************************************************************
                          MyMainWindow.h  -  description
                             -------------------
    begin                : Nov. 04, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#pragma once

#include "generated/ui_mainwindow.h"
#include "MapToolSelect.h"
#include "MyAssistant.h"

#include <QSplashScreen>

class QgsSvgAnnotationItem;
class QgsLayerTreeMapCanvasBridge;
class QgsSvgAnnotation;
class QgsAnnotation;

/**
 * イベントフィルタ
 * Waitカーソル表示等で使用する
 */
class AppFilter : public QObject
{
protected:
    bool eventFilter( QObject *obj, QEvent *event );
};


/**
 * メインウィンドウクラス
 */
class MyMainWindow :
	public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:

	/** コンストラクタ */
	MyMainWindow( QSplashScreen *pSplash );

	/** デストラクタ */
	~MyMainWindow(void);


private:

	/** 表示する地図を読み込む */
	void LoadMaps();

	/** 凡例を構築する */
	void SetupLegend();

	/** 土地利用テーブルを生成する */
	void SetupLuTable();

	/** 終了イベント */
	void closeEvent( QCloseEvent *event );

	/** 表示イベント */
	void showEvent( QShowEvent *event );

	/** 選択支流域をリセット */
	void resetSelectedArea();

	/** 結果パネルをリセット */
	void resetResultTree();

	/** マップキャンバスレイヤーリスト */
	QList<QgsMapLayer *> m_lstLayers;

	/** ビューコントロールのアクショングループ */
	QActionGroup *m_pgrpViewControl;

	/** キャンバスを移動ツール */
	QgsMapTool *m_pMapToolPan;

	/** キャンバスを拡大ツール */
	QgsMapTool *m_pMapToolZoomIn;

	/** キャンバスを縮小ツール */
	QgsMapTool *m_pMapToolZoomOut;

	/** 要素を選択ツール */
	MapToolSelect *m_pMapToolSelect;

	/** イベントフィルタ */
	AppFilter *m_pFilter;

	/** 前回指定された点の座標 */
	QgsPointXY m_pntLastPos;

	/** スプラッシュウィンドウオブジェクト */
	QSplashScreen *m_pSplash;

	/** カスタムアシスタントオブジェクト */
	MyAssistant *m_pAssistant;

	/** 指定地点に配置するアイテム */
	QgsSvgAnnotation *m_pPointAnnotation;

	/** 座標変換オブジェクト */
	QgsCoordinateTransform *m_pCoordinateTrans;

	/** マップキャンバスブリッジオブジェクト */
	QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge;

	/** basinレイヤインデックス */
	int m_nBasinLayerIdx;

	/** lumeshレイヤインデックス */
	int m_nLumeshLayerIdx;

	/** networkレイヤインデックス */
	int m_nNetworkLayerIdx;

	/** 土地利用画像レイヤインデックス */
	int m_nRasterLayerIdx;

	
private slots:

	/** キャンバスを移動ボタンが押されたとき */
	void toolPan( bool bChecked );

	/** キャンバスを拡大ボタンが押されたとき */
	void toolZoomIn( bool bChecked );

	/** キャンバスを縮小ボタンが押されたとき */
	void toolZoomOut( bool bChecked );

	/** 全体表示ボタンが押されたとき */
	void toolZoomReset();

	/** 要素を選択ボタンが押されたとき */
	void toolSelect( bool bChecked );

	/** クリック地点から上流のメッシュを集計する */
	void aggregateMesh( QgsPointXY& pntPos );

	/** クエリが終了したとき */
	void onQueryFinished();

	/** クエリエラーを表示する */
	void showError(QString strError);

	/** ツールバーの「ファイルを開く」ボタンが押されたとき */
	void onExportResult();

	/** 座標リストファイルを開くとき */
	void onFileOpen();

	/** このプログラムについてのダイアログを開く */
	void onAppAbout();

	/** ヘルプボタンが押されたとき */
	void showDocumentation();

	/** 凡例アイテムがクリックされたとき */
	void enableRasterOpacitySlider( QgsMapLayer *pLayer );

	/** 透過率スライダの値が変更されたとき */
	void changeRasterOpacity( int nOpacity );

	/** 検索開始地点のannotationが作成されたとき */
	void annotationCreated( QgsAnnotation *annotation );
};

