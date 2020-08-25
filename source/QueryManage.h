/***************************************************************************
                          QueryManage.h  -  description
                             -------------------
    begin                : Nov. 11, 2015
    copyright            : (C) 2013 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#pragma once

#include <QThread>
#include <QtCore>
//#include <QtSql>
#include <sqlite3.h>
#include <qgsrectangle.h>
#include <qgspoint.h>
#include <spatialite.h>



#define _tr(s) QString::fromLocal8Bit(s)



// 旧バージョン
//#define OLD_VERSION


/** クエリ結果テーブルを格納する */
typedef QList<QVariant> MyRowData;

/** 検索対象河川の探索範囲 */
#define RIV_SEARCH_DIST 100.0



/**
 * SQLiteデータベース管理クラス
 * シングルトンオブジェクトとして構築する
 */
class QueryManage :	public QThread
{
	Q_OBJECT

public:

	/** 初期検索河川クラス */
	typedef struct InitialRivInfo
	{
		int nRivCode;
		int nBasinId;
		double dLrsLength;
	} 
	InitialRivInfo;

	/** クエリ実行時の例外クラス */
	struct SLException
	{
		SLException( char *pMsg ) : errMsg( pMsg ){}
		SLException( const SLException &e ) : errMsg( e.errMsg ){}
		~SLException(){ if ( errMsg ) sqlite3_free( errMsg ); }

		QString errorMessage() const { return errMsg ? QString::fromUtf8( errMsg ) : ""; }
	private:
		char *errMsg;
	};

	/** 土地利用情報 */
	typedef struct _LUINFO
	{
		int nID;
		QString strLuCode;
		QString strLuName;
		int nParentItem;

		bool operator == (int n) const { return n == nID; }
		bool operator == (const QString &str) const { return str == strLuCode; }
	} LUINFO;


	/** 初期検索河川地点情報 */
	class InitRivPos
	{
	public:
		int nRivCode;
		QgsPointXY pntPos;

		bool operator == (const int n) const { return n == nRivCode; }
	};


	/** デストラクタ */
	~QueryManage(void);

	/** このクラスのシングルトンを返す */
	static QueryManage* instance();

	/** データベースに接続されているかどうか */
	inline bool isOpened() const { return m_bOpen; }

	/** データベースファイル名を取得する */
	inline QString& getDbName() { return m_strDbName; }

	/** 前回のクエリ結果を消去する */
	void clearResult();

	/** 
	 * ポイントを指定してその地点から上流の土地利用メッシュを集計する条件をセットする
	 * @param pnt 指定した地点
	 */
	inline void setCondition( QgsPointXY &pnt ){ m_pnt = pnt; }

	/** クエリ結果を取得する */
	inline QMap<int, double>& getQueryResult(){ return m_mapResult; }

	/** 土地利用名称リストを取得する */
	QVector<QueryManage::LUINFO>& getLuList();

	/** 土地利用コードの番号を取得する */
	int getLuId( QString strLuCode );

	/** 検索された支流域のIDを取得する */
	inline QVector<int>& getSearchedBasins(){ return m_vSearchedBasins; }

	/** 検索された河川名を取得する */
	inline QString getRivName(){ return m_strRivName; }

	/** 検索された土地利用メッシュを取得する */
	inline QVector<int>& getSearchedMeshes(){ return m_vSearchedMeshes; }

	/** クエリが実行されたかどうかを取得する */
	inline bool isQueryed(){ return m_bQueryed; }

	/** Spatialiteのバージョンを返す */
	inline const char * _spatialite_version() { return spatialite_version(); }

	/** 検索開始位置に対して河川までの距離範囲を限定するかどうかを設定する */
	inline void setCheckDist( bool bCheck ){ m_bCheckDist = bCheck; }

signals:

	/** クエリエラー時に発行するシグナル */
	void sqlError( QString );

protected:

	/** コンストラクタ */
	QueryManage();

	/** 土地利用テーブルを作成する */
	bool createLuTable();

	/** 前回の結果をクリアする */
	void initializeResult();

	/** クエリを実行する */
	void run();

#ifdef OLD_VERSION
	/** 上流の土地利用を集計する */
	void getUpstreamLu( QPair<int, int>& nUpstream );
#endif

#ifdef OLD_VERSION
	/** 上流を検索する */
	void getUpstreamBasins( QPair<int, int>& nUpstream );
#else
	/**
	 上流を検索する
	 @param nCurrentRivGid 対象河川のgid
	 @param strStreamCode 対象河川のstream code
	 @param dDistStartPoint 指定地点の、対象河川下流端からのLRS距離
	 */
	void getUpstreamBasins( int nCurrentRivGid, QString &strStreamCode, double dDistStartPoint );

	/**
	 分流の分岐点を含む、最初の河川を検索する
	 @param pntStartPos 検索開始地点
	 @param nRivCode 対象河川のgid
	 */
	void initialBasinSearch( QgsPointXY &pntStartPos, int nRivCode );
#endif

	/** 全ての土地利用を集計する */
	void aggregateAllLu();

	/** 
	 現在の支流域の土地利用を集計する
	 @param nCurrentRivCode 対象河川のgid
	 @param dLrsLength 対象河川下流端からのLRS距離
	 @param bGetName lumeshから河川名称を取得するかどうか
	 */
	void aggregateCurrentLu( int nCurrentRivCode, double dLrsLength, bool bGetName = false );

	/** 上流の土地利用を集計する */
	void aggregateUpstreamLu();

	/** SQLiteハンドル */
	sqlite3 *m_pSQLiteHandle;

	/** データべファイル名 */
	QString m_strDbName;

	/** データベースが接続されているかどうか */
	bool m_bOpen;

	/** 集計起点座標 */
	QgsPointXY m_pnt;

	/** 投影座標系SRID */
	int m_nSrid;

	/** このクラスのシングルトン */
	static QueryManage *m_QueryManage;

	/** 土地利用テーブル */
	QVector<LUINFO> m_vLuFields;

	/** クエリ結果 */
	QMap<int, double> m_mapResult;

	/** 検索された支流域のID */
	QVector<int> m_vSearchedBasins;

	/** 検索された土地利用メッシュのID */
	QVector<int> m_vSearchedMeshes;

	/** クリックした地点の河川名 */
	QString m_strRivName;

	/** 検索が実行されたかどうか */
	bool m_bQueryed;

	/** Spatialiteキャッシュオブジェクト */	
	void *m_pCache;

	/** スタート地点を含む河川情報の配列 */
	QVector<InitialRivInfo> m_vInitRivInfo;

	/** スタート地点をを含む未検索の河川 */
	QQueue<InitRivPos> m_qInitRivPos;

	/** 検索開始位置に対して河川までの距離範囲を限定するかどうか */
	bool m_bCheckDist;

};

