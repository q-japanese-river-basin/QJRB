/***************************************************************************
                          MyAssistant.cpp  -  description
                             -------------------
    begin                : Dec 04, 2015
    copyright            : (C) 2015 by niaes
	author               : Yamate, N
 ***************************************************************************/


#include "MyAssistant.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>
#include <QtCore/QProcess>
#include <QtWidgets/QMessageBox>
#include <QtCore/QCoreapplication>

#include <QDebug>

MyAssistant::MyAssistant(void)
    : m_pProc(0)
{
}


MyAssistant::~MyAssistant(void)
{
	if ( m_pProc && m_pProc->state() == QProcess::Running )
	{	
		m_pProc->terminate();
		m_pProc->waitForFinished(3000);
	}
	delete m_pProc;
}


void MyAssistant::showDocumentation(const QString &page)
{
	if (!startAssistant())
		return;
	
	QByteArray ba("SetSource ");
	ba.append("qthelp://gov.niaes.qgis.lrs/doc/");
	m_pProc->write(ba + page.toLocal8Bit() + '\n');
}


bool MyAssistant::startAssistant()
{
    if ( !m_pProc )
		m_pProc = new QProcess();

    if ( m_pProc->state() != QProcess::Running )
	{
//		QString app = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QDir::separator();
		QString app = QLatin1String("assistant");

		QStringList strlArgs;
		QString strAppDirPath = QFileInfo( QCoreApplication::applicationDirPath() ).absoluteFilePath();
		qDebug() << strAppDirPath;

		strlArgs << QLatin1String("-collectionFile")
			<< strAppDirPath + QLatin1String( "/../../doc/qgis_lrs_help.qhc")
			<< QLatin1String("-enableRemoteControl");

		m_pProc->start( app, strlArgs );
		qDebug() << strlArgs;

        if ( !m_pProc->waitForStarted() )
		{
			QMessageBox::critical(0, QObject::tr("Simple Text Viewer"),
				QObject::tr("Unable to launch Qt Assistant (%1)").arg(app));
			return false;
		}
	}
	
	return true;
}