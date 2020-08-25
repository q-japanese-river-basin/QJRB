/***************************************************************************
                          DlgPointCsv.h  -  description
                             -------------------
    begin                : Nov. 24, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#pragma once

#include "generated/ui_dlgPointCsv.h"

/** 点を指定して集計するときのファイル指定ダイアログクラス */
class DlgPointCsv :
	public QDialog, public Ui::ui_PointCsv
{
	Q_OBJECT

public:
	DlgPointCsv( QWidget *parent = NULL );
	~DlgPointCsv();


private slots:

	/** 入力ファイルの「参照」ボタンが押されたとき */
	void onBrowseInput();

	/** 出力ファイルの「参照」ボタンが押されたとき */
	void onBrowseOutput();
};

