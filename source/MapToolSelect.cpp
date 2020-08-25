/***************************************************************************
                          MapToolSelect.h  -  description
                             -------------------
    begin                : Oct 09, 2015
    copyright            : (C) 2015 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#include "MapToolSelect.h"
#include <qgsvectorlayer.h>
//#include <qgsmapcanvas.h>
#include "MyMapCanvas.h"
#include <qgsmaptopixel.h>
#include <qgsvectordataprovider.h>
#include <qgsmapmouseevent.h>
#include <QtGui>
#include <QMessageBox>


MapToolSelect::MapToolSelect( MyMapCanvas *canvas )
	: QgsMapTool( canvas )
{
	mCursor = Qt::ArrowCursor;

	qDebug() << "MapToolSelect";
	qDebug() << this->canvas();
}


MapToolSelect::~MapToolSelect(void)
{
}


void MapToolSelect::canvasPressEvent(QgsMapMouseEvent *event)
{
	qDebug() << "MapToolSelect press event";

	if ( event->button() == Qt::LeftButton )
	{
		const QgsMapToPixel *pTrans = mCanvas->getCoordinateTransform();
		QgsPointXY pntPos = pTrans->toMapCoordinates(QPoint(event->pos().x(), event->pos().y()));

		emit mousePressed(pntPos);
	}
}

