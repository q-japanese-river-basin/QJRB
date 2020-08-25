/***************************************************************************
                          MapToolSelect.h  -  description
                             -------------------
    begin                : Oct 09, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/

#pragma once

#include <qgsmaptool.h>

class MyMapCanvas;

/**
 *  データ選択ツールクラス
 */
class MapToolSelect :	public QgsMapTool
{
	Q_OBJECT

public:

	/**
	 * コンストラクタ
	 * @param canvas マップキャンバスへのポインタ
	 */
	MapToolSelect( MyMapCanvas *canvas );

	/** デストラクタ */
	~MapToolSelect(void);

	/** キャンバス上をクリックしたときのイベント */
	virtual void canvasPressEvent( QgsMapMouseEvent *event );

signals:

	/** 押されたポイントの経緯度 */
	void mousePressed( QgsPointXY& );
};

