/***************************************************************************
						  QueryManage.h  -  description
							 -------------------
	begin                : Nov. 02, 2019
	copyright            : (C) 2013 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#pragma once
#include <qgsmapcanvas.h>

 /**
  * カスタムマップキャンバスクラス
  */
class MyMapCanvas : public QgsMapCanvas
{
	Q_OBJECT

public:

	/** コンストラクタ */
	MyMapCanvas( QWidget *parent = nullptr );

	/** デストラクタ */
	~MyMapCanvas();

protected:
	/** マウスクリックイベント */
	void mousePressEvent( QMouseEvent *event ) override;

signals:
	/** 右クリックイベント発生時のシグナル */
	void rightButtonClicked();
};

