/***************************************************************************
                          DlgPointCsv.cpp  -  description
                             -------------------
    begin                : Nov. 24, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/

#include "DlgPointCsv.h"
#include <QFileDialog>


DlgPointCsv::DlgPointCsv( QWidget *parent )
	: QDialog( parent )
{
	setupUi( this );
}


DlgPointCsv::~DlgPointCsv()
{
}


void DlgPointCsv::onBrowseInput()
{
	QString strFilter = tr("Comma Separated Value(*.csv)");
	QString strSelected("");
	QString strFName = QFileDialog::getOpenFileName( this, QString::fromLocal8Bit( "座標ファイルを指定" ), ".", strFilter, &strSelected );

	if ( !strFName.isEmpty() )
	{
		lineEdit_input->setText( strFName );
	}
}


void DlgPointCsv::onBrowseOutput()
{
	QString strFilter = tr("Comma Separated Value(*.csv)");
	QString strSelected("");
	QString strFName = QFileDialog::getSaveFileName( this, QString::fromLocal8Bit( "結果をエクスポート" ), ".", strFilter, &strSelected );

	if ( !strFName.isEmpty() )
	{
		lineEdit_output->setText( strFName );
	}
}
