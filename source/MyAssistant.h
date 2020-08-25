/***************************************************************************
                          MyAssistant.h  -  description
                             -------------------
    begin                : Dec 04, 2015
    copyright            : (C) 2015 by niaes
	author               : Yamate, N
 ***************************************************************************/


#pragma once

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

/**
 * カスタムアシスタント表示クラス
 */
class MyAssistant
{
public:

	/** コンストラクタ */
	MyAssistant(void);

	/** デストラクタ */
	~MyAssistant(void);

	/** 表示するページを指定する */
	void showDocumentation(const QString &file);

private:

	/** アシスタントを起動する */
    bool startAssistant();

	/** アシスタントプロセス */
    QProcess *m_pProc;
};

