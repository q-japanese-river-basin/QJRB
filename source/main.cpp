/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Nov. 04, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/



//
// QGIS Includes
//
#include <qgsapplication.h>
#include <qgsproviderregistry.h>
//
// Qt Includes
//
#include <QString>
#include <QApplication>
#include <QWidget>


#include "MyMainWindow.h"

int main(int argc, char ** argv)
{
//	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);

	QgsApplication app(argc, argv, true);

	QString strAppDirPath = QFileInfo( QCoreApplication::applicationDirPath() ).absoluteFilePath();
#ifdef ENABLE_BGMAP
	QString myPluginsDir        = strAppDirPath + "/../plugins";
#else
	QString myPluginsDir		= "D:/dev/qgis3.4.3/plugins";
#endif
	qDebug() << myPluginsDir;
	QgsProviderRegistry::instance(myPluginsDir);

	app.setPkgDataPath( strAppDirPath + "/../" );

	//QTextCodec::setCodecForCStrings( QTextCodec::codecForLocale() );
	//QTextCodec::setCodecForTr( QTextCodec::codecForLocale() );

	QSplashScreen *pSplash = new QSplashScreen( QPixmap(":/image/splash.png") );
	pSplash->show();
	app.processEvents();

	MyMainWindow window( pSplash );
	window.show();
	pSplash->finish( &window );

	return app.exec();
}
