#include "MyMapCanvas.h"



MyMapCanvas::MyMapCanvas( QWidget *parent )
	: QgsMapCanvas( parent )
{
}


MyMapCanvas::~MyMapCanvas()
{
}


void MyMapCanvas::mousePressEvent(QMouseEvent *event)
{
	QgsMapCanvas::mousePressEvent( event );

	if ( event->button() == Qt::RightButton )
	{
		emit rightButtonClicked();
	}
}

