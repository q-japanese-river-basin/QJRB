/***************************************************************************
                          QueryManage.cpp  -  description
                             -------------------
    begin                : Nov. 11, 2015
    copyright            : (C) 2013 by NIAES
	author               : Yamate, N
 ***************************************************************************/


#include "QueryManage.h"

#include <QtGui>
#include <QtWidgets>
#include <QtWidgets/QMessageBox>

QueryManage *QueryManage::m_QueryManage = 0;
QueryManage* QueryManage::instance()
{
	if ( m_QueryManage == 0 )
	{
		m_QueryManage = new QueryManage();
	}

	return m_QueryManage;
}


QueryManage::QueryManage() :
	m_bQueryed( false ),
	m_bOpen( false ),
	m_bCheckDist( true )
{

	sqlite3_config( SQLITE_CONFIG_SINGLETHREAD );

	// load DB
	QString strAppDirPath = QFileInfo( QCoreApplication::applicationDirPath() ).absoluteFilePath();
//#ifdef _DEBUG
//	m_strDbName = "E:/niaes_lrs/data_2017/lrs.sqlite";
//#else
	m_strDbName =  strAppDirPath + "/../../data/lrs.sqlite";
//#endif
	if ( !QFile::exists( m_strDbName ) )
	{
		return;
	}

	int nRet = sqlite3_open_v2( m_strDbName.toUtf8().constData(), &m_pSQLiteHandle, SQLITE_OPEN_READWRITE, NULL );
	if ( nRet )
	{
		QString strError( sqlite3_errmsg( m_pSQLiteHandle ) );
		sqlite3_close( m_pSQLiteHandle );
		QMessageBox::warning( NULL, "ERROR", strError );
		m_pSQLiteHandle = NULL;
		return;
	}

	m_pCache = spatialite_alloc_connection();
	spatialite_init_ex( m_pSQLiteHandle, m_pCache, 0 );

	char *pErrMsg = NULL;
	nRet = sqlite3_exec( m_pSQLiteHandle,
		"PRAGMA foreign_keys = 1; PRAGMA case_sensitive_like = ON; PRAGMA journal_mode = Wal; PRAGMA SyncMode = Off;",
		NULL, 0, &pErrMsg );
	if ( nRet != SQLITE_OK )
	{
		QMessageBox::warning( NULL, "ERROR", QString( pErrMsg ) );
		sqlite3_free( pErrMsg );
		sqlite3_close( m_pSQLiteHandle );
		m_pSQLiteHandle = NULL;
		return;
	}

	// read SRID
	sqlite3_stmt *pStmt;
	const char szSql[] = "select srid from projection_info;";
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, szSql, strlen( szSql ), &pStmt, NULL );
	if ( nRet != SQLITE_OK )
	{
		QMessageBox::warning( QApplication::activeWindow(), "ERROR", tr("SRID情報を取得できません、UTM54Nを仮定します") );
		m_nSrid = 3100;
	}
	nRet = sqlite3_step( pStmt );
	m_nSrid = sqlite3_column_int( pStmt, 0 );
//	qDebug() << m_nSrid;
	sqlite3_finalize( pStmt );

	// create lu map
	m_bOpen = createLuTable();
}


QueryManage::~QueryManage(void)
{
	clearResult();
	sqlite3_close( m_pSQLiteHandle );

    spatialite_cleanup_ex( m_pCache );
    spatialite_shutdown();
}


void QueryManage::clearResult()
{
	m_pnt = QgsPointXY();
	m_strRivName = QString();
	m_bQueryed = false;

	initializeResult();
}


bool QueryManage::createLuTable()
{
	sqlite3_stmt *pStmt;
	QString strLuName;

	try
	{
		// check lucode table;
		QString strSql = QString( "select count(type) from sqlite_master where name = 'lucode';" );
		int nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			qDebug() << (char *)sqlite3_errmsg( m_pSQLiteHandle );
//			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
//			return false;
		}
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			QMessageBox::critical( qApp->activeWindow(), "CRITICAL", tr("lucodeテーブルがありません") );
			sqlite3_finalize( pStmt );
			QCoreApplication::instance()->exit( 1 );
		}
		else if ( nRet == SQLITE_ROW )
		{
			if ( sqlite3_column_int( pStmt, 0 ) == 0 )
			{
				QMessageBox::critical( qApp->activeWindow(), "CRITICAL", tr("lucodeテーブルがありません") );
				sqlite3_finalize( pStmt );
//				QCoreApplication::instance()->exit( 1 );
//				throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
				return false;
			}
		}
		sqlite3_finalize( pStmt );

		strSql = QString( "select id,lucode,luname,subitem from lucode order by id" );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			qDebug() << (char *)sqlite3_errmsg( m_pSQLiteHandle );
//			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
			return false;
		}

		int nMapCount = 0;
		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE )
			{
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				LUINFO info;
				info.nID = sqlite3_column_int( pStmt, 0 );
				info.strLuCode =
					QString::fromUtf8((const char *)sqlite3_column_text(pStmt, 1));
				info.strLuName =
					QString::fromUtf8((const char *)sqlite3_column_text(pStmt, 2));
				info.nParentItem = sqlite3_column_int( pStmt, 3 );
				m_vLuFields.push_back( info );

				// initialize result table
//				m_mapResult[info.nID] = 0;
			}
		}
		sqlite3_finalize( pStmt );
	}
	catch ( SLException &e )
	{
		sqlite3_finalize( pStmt );
		emit sqlError( QString(e.errorMessage()) );
	}

	return true;
}


QVector<QueryManage::LUINFO>& QueryManage::getLuList()
{
/*
	sqlite3_stmt *pStmt;
	QString strLuName;
	QVector<LUINFO> vstrLuName;

	try
	{
		QString strSql = QString( "select luname,PK_UID,subitem from lucode order by PK_UID" );
		int nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			qDebug() << (char *)sqlite3_errmsg( m_pSQLiteHandle );
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}

		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE )
			{
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				LUINFO info;
				info.strLuName = 
					QString::fromUtf8((const char *)sqlite3_column_text( pStmt, 0 ));
				info.nID = sqlite3_column_int( pStmt, 1 );
				info.nParentItem = sqlite3_column_int( pStmt, 2 );
//				qDebug() << strLu;
				vstrLuName.push_back( info );
			}
		}
		sqlite3_finalize( pStmt );
	}
	catch ( SLException &e )
	{
		sqlite3_finalize( pStmt );
		emit sqlError( e.errorMessage() );
	}

	return std::move( vstrLuName );
*/
	return m_vLuFields;
}


int QueryManage::getLuId( QString strLuCode )
{
	sqlite3_stmt *pStmt;
	int nId;

	try
	{
		QString strSql = QString( "select id from lucode where lucode = '%1';" ).arg( strLuCode );
		qDebug() << strSql;
		int nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}

		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_ROW )
		{
			nId = sqlite3_column_int( pStmt, 0 );
		}
		else
		{
			nId = -1;
		}
		sqlite3_finalize( pStmt );
	}
	catch ( SLException &e )
	{
		sqlite3_finalize( pStmt );
		emit sqlError( e.errorMessage() );
	}

	return nId;
}


void QueryManage::initializeResult()
{
	m_bQueryed = false;

	// clear previous result
//	std::for_each(m_mapResult.begin(), m_mapResult.end(),
//				  [&](double d){ d = 0.0; });
	m_mapResult.clear();

	// clear searched basins;
	m_vSearchedBasins.clear();

	// clear searched meshes;
	m_vSearchedMeshes.clear();

	// clear initial river info
	m_vInitRivInfo.clear();

	// drop previous mesh view
	char *pErrMsg = NULL;
	int nRet = sqlite3_exec( m_pSQLiteHandle, "drop view tempmesh;", NULL, 0, &pErrMsg );

	// reset rivname
	m_strRivName = "";

	sqlite3_free( pErrMsg );
}


void QueryManage::run()
{
	sqlite3_stmt *pStmt;
	int nRet;
	bool bCommit = false;
	char *pErrMsg = NULL;

	initializeResult();

	// initial basin
	try
	{
		// find target river.
//		QString strSql = 
//			QString( "select gid,basinid from network where st_distance(GEOMETRY, st_geomfromtext('POINT(%1 %2)', %3))		\
//			= (select min(st_distance(GEOMETRY, st_geomfromtext('POINT(%1 %2)', %3))) from network);")
//			.arg( m_pnt.x() ).arg( m_pnt.y() ).arg( m_nSrid );
		QString strSql = QString( " select R.gid,B.gid from network as R, basin as B where  \
					  			  st_within( st_geomfromtext('POINT(%1 %2)' , %3), B.GEOMETRY ) \
								  and R.basinid = B.gid" )
								.arg( m_pnt.x(), 11, 'f', 8 ).arg( m_pnt.y(), 11, 'f', 8 ).arg( m_nSrid );
		qDebug() << strSql;
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}

		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			sqlite3_finalize( pStmt );
			return;
		}
		int nRivCode = sqlite3_column_int64( pStmt, 0 );
		int nBasinId = sqlite3_column_int64( pStmt, 1 );
		qDebug() << nRivCode;
		sqlite3_finalize( pStmt );

		// calc distance between point and riv.
		if ( m_bCheckDist )
		{
			strSql =
				QString( "select st_length(st_transform(shortestline(st_geomfromtext('POINT(%1 %2)', 4612), GEOMETRY), %3)) from network where gid = %4" )
				.arg( m_pnt.x(), 11, 'f', 8 ).arg( m_pnt.y(), 11, 'f', 8 ).arg( m_nSrid ).arg( nRivCode );
			qDebug() << strSql;
			nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
			if ( nRet != SQLITE_OK )
			{
				throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
			}

			nRet = sqlite3_step( pStmt );
			double dDist = sqlite3_column_double( pStmt, 0 );
			qDebug() << dDist;

			sqlite3_finalize( pStmt );

			if ( dDist > RIV_SEARCH_DIST )
			{
				return;
			}
		}

//		m_vSearchedBasins.push_back( nBasinId );

		// start with initial point
		initialBasinSearch( m_pnt, nRivCode );

		// search split flow
/*
		strSql = 
			QString( "select x(startpoint(n.geometry)),y(startpoint(n.geometry)),n.splitrivid \
				from network as n, basin as b where n.basinid = b.gid and n.splitrivid > 0 and b.gid in (" );
		// add current basin
		strSql += QString::number( nBasinId, 10 );
		if ( m_vSearchedBasins.size() )
		{
			strSql += ",";
			for ( int i = 0; i < m_vSearchedBasins.size(); i++ )
			{
				strSql += QString::number( m_vSearchedBasins[i] );
				if ( i != m_vSearchedBasins.size() - 1 )
					strSql += ",";
				else
					strSql += ");";
			}
		}
		else
		{
			strSql += ")";
		}
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );

		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE || nRet == SQLITE_MISUSE )
			{
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				QgsPoint pntEndPos = QgsPoint(
					sqlite3_column_double( pStmt, 0 ),
					sqlite3_column_double( pStmt, 1 ) );
				nRivCode = sqlite3_column_int( pStmt, 2 );
				InitRivPos rpos;
				rpos.pntPos = pntEndPos;
				rpos.nRivCode= nRivCode;
				m_qInitRivPos.enqueue(rpos);
			}
		}
		sqlite3_finalize( pStmt );
*/
		// search upstream from split point while Queue is present.
		while ( !m_qInitRivPos.isEmpty() )
		{
#ifdef  _DEBUG
			for ( int i = 0; i < m_qInitRivPos.size(); i++ )
			{
				qDebug() << m_qInitRivPos[i].nRivCode;
			}
#endif //  _DEBUG

			InitRivPos rpos = m_qInitRivPos.dequeue();
			initialBasinSearch( rpos.pntPos, rpos.nRivCode );
		}

		// finally aggregate all meshes.
		aggregateAllLu();

	}
	catch ( SLException &e )
	{
		if ( bCommit )
		{
			sqlite3_exec( m_pSQLiteHandle, "rollback", NULL, NULL, NULL );
		}
		sqlite3_finalize( pStmt );
		emit sqlError( e.errorMessage() );
		return;
	}
}

#ifndef OLD_VERSION


void QueryManage::initialBasinSearch( QgsPointXY &pntStartPos, int nRivCode )
{
	sqlite3_stmt *pStmt;
	char *pErrMsg = NULL;

	// calc lrs distance of target river.
	QString strSql = 
		QString( "select st_length(st_transform(GEOMETRY, %4)) *												\
		(1 - line_locate_point(GEOMETRY, st_geomfromtext('POINT(%1 %2)', %4))) from network where gid = %3;" )
		.arg( pntStartPos.x(), 11, 'f', 8 ).arg( pntStartPos.y(), 11, 'f', 8 ).arg( nRivCode ).arg( m_nSrid );
		qDebug() << strSql;
	int nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
	if ( nRet != SQLITE_OK )
	{
		throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
	}

	nRet = sqlite3_step( pStmt );
	double dLrsLength = sqlite3_column_double( pStmt, 0 );
	sqlite3_finalize( pStmt );
//	qDebug() << " dLrsLength = " << dLrsLength;

	// collect information of target river
//		strSql = QString( "select rivcode,basinid,diststartpoint,st_length(st_transform(GEOMETRY, %2)) from network where gid = %1" )
//			.arg( nRivCode ).arg( m_nSrid );
	strSql = QString( "select gid,basinid,diststartpoint,streamcode,st_length(st_transform(GEOMETRY, %2)) from network where gid = %1" )
		.arg( nRivCode ).arg( m_nSrid );
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
	if ( nRet != SQLITE_OK )
	{
		throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
	}

	nRet = sqlite3_step( pStmt );
	int nRivGid = sqlite3_column_int64( pStmt, 0 );
	int nBasinid = sqlite3_column_int64( pStmt, 1 );
	double dSdist = sqlite3_column_double( pStmt, 2 );
	QString strStrmCode = QString::fromLocal8Bit( (const char *)sqlite3_column_text( pStmt, 3 ) );
	double dRivLength = sqlite3_column_double( pStmt, 4 );
	sqlite3_finalize( pStmt );
//		qDebug() << nBasinid << dSdist << dRivLength;


	// check if the upstream was already queried.
	if ( m_vSearchedBasins.contains( nBasinid ) )
	{
		return;
	}

	// register initial riv info
	InitialRivInfo info;
	info.nRivCode = nRivCode;
	info.nBasinId = nBasinid;
	info.dLrsLength = dLrsLength;
	m_vInitRivInfo.push_back( info );

	// search upstream
#ifndef OLD_VERSION
	getUpstreamBasins( nRivGid, strStrmCode, dLrsLength );
	m_bQueryed = true;
#else
	QVector<QPair<int, int>> vnUpstreamRivCode;
//		strSql = QString( "select gid,basinid from network where downstreamid = %1" ).arg( nRivCode );
	strSql = QString( "select gid,basinid from network where downstreamid = %1 and diststartpoint > %2" )
		.arg( nRivCode ).arg( dLrsLength );
	qDebug() << strSql;
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );

	while ( true )
	{
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			break;
		}
		else if ( nRet == SQLITE_ROW )
		{
			vnUpstreamRivCode.push_back( qMakePair( sqlite3_column_int( pStmt, 0 ), sqlite3_column_int( pStmt, 1 ) ) );
		}
	}
	sqlite3_finalize( pStmt );

	for ( int i = 0; i < vnUpstreamRivCode.size(); i++ )
	{
		getUpstreamBasins( vnUpstreamRivCode[i] );
	}

	vnUpstreamRivCode.clear();

	m_bQueryed = true;

/*
	cur.execute("select rivcode,basinid,diststartpoint,st_length(st_transform(geom, " + srid + ")) from "
 + target_table + " where gid = " + str(rivgid) + ";")
*/

#endif
}


void QueryManage::getUpstreamBasins( int nCurrentRivGid, QString &strStreamCode, double dDistStartPoint )
{
	int nRet;
	sqlite3_stmt *pStmt;
	bool bCommit = false;
	char *pErrMsg = NULL;
	QMap<int, QString> mapFieldName;
	QString strSql;
	//QStringList strlInitStreamCode;
	QVector<int> vnInitStreamCode;

	/*
	上流河川の検索は、まず
	①　現河川に流入し、かつdiststartpointが指定地点より大きい河川の抽出を行い、
	②　次に①で抽出された河川のstreamcodeに後方一致する河川をすべて抽出する
	という二段階で検索する必要がある。
	単にstreamcodeが後方一致するものだけを抽出すると、
	diststartpointが指定地点より小さいところから分岐した河川の上流にあたるものが検索対象となってしまう
	②の検索では自分自身も検索対象に含める必要があるため、like句以降の条件は
	[ 'streamcode'% ] となる
	*/

	// search all upstreams
	try
	{
		// search rivers flow into current river
		strSql = QString( "select gid from network where downstreamid = %1 and diststartpoint > %2" )
			.arg( nCurrentRivGid ).arg( dDistStartPoint, 0, 'f', 4 );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}
		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE )
			{
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				vnInitStreamCode.append( sqlite3_column_int( pStmt, 0 ) );
			}
		}
		sqlite3_finalize( pStmt );

		if ( vnInitStreamCode.size() )
		{
			// search rivers 
			//strSql = QString( "select basinid,streamcode,diststartpoint from network where streamcode " );
			//for ( int i = 0; i < strlInitStreamCode.size(); i++ )
			//{
			//	strSql += "like '" + strlInitStreamCode[i] + "%'";
			//	if ( i != strlInitStreamCode.size() - 1 )
			//	{
			//		strSql += " or streamcode ";
			//	}
			//}
			//qDebug() << strSql;

			for ( auto nGid : vnInitStreamCode )
			{
				strSql = QString( "with recursive R as (select * from network where gid = %1 union all select network.* from network,r where network.downstreamid = r.gid) select basinid,streamcode,diststartpoint from r" )
					.arg( nGid );
				nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
				if ( nRet != SQLITE_OK )
				{
					throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
				}

				while ( true )
				{
					nRet = sqlite3_step( pStmt );
					if ( nRet == SQLITE_DONE )
					{
						break;
					}
					else if ( nRet == SQLITE_ROW )
					{
						int nBasinid = sqlite3_column_int( pStmt, 0 );
						if ( (strlen( (const char *)sqlite3_column_text( pStmt, 1 ) ) > strStreamCode.length() + 1 ||  // ommit current basin.
							sqlite3_column_double( pStmt, 2 ) > dDistStartPoint) &&  // check if the join point with current basin is the upstream side.
							!m_vSearchedBasins.contains( nBasinid ) )  // check if the basin was already queried.
						{
							m_vSearchedBasins.push_back( nBasinid );
						}
						//				vnUpstreamRivCode.push_back( qMakePair( sqlite3_column_int( pStmt, 0 ), sqlite3_column_int( pStmt, 1 ) ) );
					}
				}
				sqlite3_finalize( pStmt );
			}
		}

		// search more split flow
		if ( vnInitStreamCode.size() )
		{
			for ( auto nGid : vnInitStreamCode )
			{
				//strSql =
				//	QString( "select x(startpoint(n.geometry)),y(startpoint(n.geometry)),n.splitrivid \
				//		from network as n where n.splitrivid > 0 and (streamcode " );
				//for ( int i = 0; i < strlInitStreamCode.size(); i++ )
				//{
				//	strSql += "like '" + strlInitStreamCode[i] + "%'";
				//	if ( i != strlInitStreamCode.size() - 1 )
				//	{
				//		strSql += " or streamcode ";
				//	}
				//	else
				//	{
				//		strSql += ")";
				//	}
				//}
				strSql =
					QString( "with recursive r as (select * from network where gid = %1 union all select network.* from network,r where network.downstreamid = r.gid) select x(startpoint(r.geometry)),y(startpoint(r.geometry)),r.splitrivid from r where splitrivid > 0" )
					.arg( nGid );
				qDebug() << strSql;
				nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );

				while ( true )
				{
					nRet = sqlite3_step( pStmt );
					if ( nRet == SQLITE_DONE || nRet == SQLITE_MISUSE )
					{
						break;
					}
					else if ( nRet == SQLITE_ROW )
					{
						QgsPoint pntEndPos = QgsPoint(
							sqlite3_column_double( pStmt, 0 ),
							sqlite3_column_double( pStmt, 1 ) );
						int nRivCode = sqlite3_column_int( pStmt, 2 );
						InitRivPos rpos;
						rpos.pntPos = pntEndPos;
						rpos.nRivCode = nRivCode;
						if ( qFind( m_qInitRivPos, rpos.nRivCode ) == m_qInitRivPos.end() )
							m_qInitRivPos.enqueue( rpos );
					}
				}
				sqlite3_finalize( pStmt );
			}
		}
		// and current riv splitrivid if present.
		strSql = 
			QString( "select x(startpoint(n.geometry)),y(startpoint(n.geometry)),n.splitrivid \
				from network as n where n.gid = %1" ).arg( nCurrentRivGid );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_ROW )
		{
			if ( sqlite3_column_blob(pStmt, 2) != NULL )
			{
				QgsPoint pntEndPos = QgsPoint(
					sqlite3_column_double(pStmt, 0),
					sqlite3_column_double(pStmt, 1));
				int nRivCode = sqlite3_column_int(pStmt, 2);
				InitRivPos rpos;
				rpos.pntPos = pntEndPos;
				rpos.nRivCode = nRivCode;
				if ( qFind(m_qInitRivPos, rpos.nRivCode) == m_qInitRivPos.end() )
					m_qInitRivPos.enqueue(rpos);
			}
		}
		sqlite3_finalize( pStmt );
	}
	catch ( SLException &e )
	{
		throw e;
	}
}


#else
void QueryManage::getUpstreamBasins( int nCurrentRivGid, QString &strStreamCode, double dDistStartPoint )
{
	int nRet;
	sqlite3_stmt *pStmt;
	bool bCommit = false;
	char *pErrMsg = NULL;
	QMap<int, QString> mapFieldName;
	QString strSql;
	QStringList strlInitStreamCode;

	/*
	上流河川の検索は、まず
	①　現河川に流入し、かつdiststartpointが指定地点より大きい河川の抽出を行い、
	②　次に①で抽出された河川のstreamcodeに後方一致する河川をすべて抽出する
	という二段階で検索する必要がある。
	単にstreamcodeが後方一致するものだけを抽出すると、
	diststartpointが指定地点より小さいところから分岐した河川の上流にあたるものが検索対象となってしまう
	②の検索では自分自身も検索対象に含める必要があるため、like句以降の条件は
	[ 'streamcode'% ] となる
	*/

	// search all upstreams
	try
	{
		// search rivers flow into current river
		strSql = QString( "select streamcode from network where downstreamid = %1 and diststartpoint > %2" )
			.arg( nCurrentRivGid ).arg( dDistStartPoint, 0, 'f', 4 );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}
		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE )
			{
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				strlInitStreamCode.append( (const char *)sqlite3_column_text( pStmt, 0 ) );
			}
		}
		sqlite3_finalize( pStmt );

		if ( strlInitStreamCode.size() )
		{
			// search rivers 
			strSql = QString( "select basinid,streamcode,diststartpoint from network where streamcode " );
			for ( int i = 0; i < strlInitStreamCode.size(); i++ )
			{
				strSql += "like '" + strlInitStreamCode[i] + "%'";
				if ( i != strlInitStreamCode.size() - 1 )
				{
					strSql += " or streamcode ";
				}
			}
			qDebug() << strSql;
			nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
			if ( nRet != SQLITE_OK )
			{
				throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
			}

			while ( true )
			{
				nRet = sqlite3_step( pStmt );
				if ( nRet == SQLITE_DONE )
				{
					break;
				}
				else if ( nRet == SQLITE_ROW )
				{
					int nBasinid = sqlite3_column_int( pStmt, 0 );
					if ( (strlen( (const char *)sqlite3_column_text( pStmt, 1 ) ) > strStreamCode.length() + 1 ||  // ommit current basin.
						sqlite3_column_double( pStmt, 2 ) > dDistStartPoint) &&  // check if the join point with current basin is the upstream side.
						!m_vSearchedBasins.contains( nBasinid ) )  // check if the basin was already queried.
					{
						m_vSearchedBasins.push_back( nBasinid );
					}
					//				vnUpstreamRivCode.push_back( qMakePair( sqlite3_column_int( pStmt, 0 ), sqlite3_column_int( pStmt, 1 ) ) );
				}
			}
			sqlite3_finalize( pStmt );
		}

		// search more split flow
		if ( strlInitStreamCode.size() )
		{
			strSql =
				QString( "select x(startpoint(n.geometry)),y(startpoint(n.geometry)),n.splitrivid \
					from network as n where n.splitrivid > 0 and (streamcode " );
			for ( int i = 0; i < strlInitStreamCode.size(); i++ )
			{
				strSql += "like '" + strlInitStreamCode[i] + "%'";
				if ( i != strlInitStreamCode.size() - 1 )
				{
					strSql += " or streamcode ";
				}
				else
				{
					strSql += ")";
				}
			}
			qDebug() << strSql;
			nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );

			while ( true )
			{
				nRet = sqlite3_step( pStmt );
				if ( nRet == SQLITE_DONE || nRet == SQLITE_MISUSE )
				{
					break;
				}
				else if ( nRet == SQLITE_ROW )
				{
					QgsPoint pntEndPos = QgsPoint(
						sqlite3_column_double( pStmt, 0 ),
						sqlite3_column_double( pStmt, 1 ) );
					int nRivCode = sqlite3_column_int( pStmt, 2 );
					InitRivPos rpos;
					rpos.pntPos = pntEndPos;
					rpos.nRivCode = nRivCode;
					if ( qFind( m_qInitRivPos, rpos.nRivCode ) == m_qInitRivPos.end() )
						m_qInitRivPos.enqueue( rpos );
				}
			}
			sqlite3_finalize( pStmt );
		}
		// and current riv splitrivid if present.
		strSql =
			QString( "select x(startpoint(n.geometry)),y(startpoint(n.geometry)),n.splitrivid \
				from network as n where n.gid = %1" ).arg( nCurrentRivGid );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_ROW )
		{
			QgsPoint pntEndPos = QgsPoint(
				sqlite3_column_double( pStmt, 0 ),
				sqlite3_column_double( pStmt, 1 ) );
			int nRivCode = sqlite3_column_int( pStmt, 2 );
			InitRivPos rpos;
			rpos.pntPos = pntEndPos;
			rpos.nRivCode = nRivCode;
			if ( qFind( m_qInitRivPos, rpos.nRivCode ) == m_qInitRivPos.end() )
				m_qInitRivPos.enqueue( rpos );
		}
		sqlite3_finalize( pStmt );
	}
	catch ( SLException &e )
	{
		throw e;
	}
}



void QueryManage::getUpstreamBasins( QPair<int, int>& nUpstream )
{
	int nRet;
	sqlite3_stmt *pStmt;
	bool bCommit = false;
	char *pErrMsg = NULL;
	QMap<int, QString> mapFieldName;
	QString strSql;

	qDebug() << nUpstream.first;
	m_vSearchedBasins.push_back( nUpstream.second );
	
	// search upstream
	QVector<QPair<int, int>> vnUpstreamRivCode;
	strSql = QString( "select gid,basinid from network where downstreamid = %1" ).arg( nUpstream.first );
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
	if ( nRet != SQLITE_OK )
	{
		throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
	}

	while ( true )
	{
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			break;
		}
		else if ( nRet == SQLITE_ROW )
		{
			vnUpstreamRivCode.push_back( qMakePair( sqlite3_column_int( pStmt, 0 ), sqlite3_column_int( pStmt, 1 ) ) );
		}
	}
	sqlite3_finalize( pStmt );

	for ( int i = 0; i < vnUpstreamRivCode.size(); i++ )
	{
		getUpstreamBasins( vnUpstreamRivCode[i] );
	}
	vnUpstreamRivCode.clear();
}



void QueryManage::getUpstreamLu( QPair<int, int>& nUpstream )
{
	int nRet;
	sqlite3_stmt *pStmt;
	bool bCommit = false;
	char *pErrMsg = NULL;
	QMap<int, QString> mapFieldName;
	QString strSql;

	qDebug() << nUpstream.first;
	m_vSearchedBasins.push_back( nUpstream.second );

	try
	{
		strSql = 
			QString( "select * from lusum where rivid = %1" ).arg( nUpstream.first );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}


		int n = 0;
		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE )
			{
				QString str = "";
				for ( int i = 0; i < m_vLuSum.size(); i++ )
					str += QString::number(m_vLuSum[i]) + "|";
				qDebug() << str;
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				if ( n == 0 )
				{
					for ( int i = 2; i < sqlite3_column_count( pStmt ); i++ )
					{
						QString strField = QString::fromUtf8( sqlite3_column_name( pStmt, i ) );
						strField.remove( "lu_" );
						mapFieldName[i] = strField;
					}
				}
				for ( int i = 2; i < sqlite3_column_count( pStmt ); i++ )
				{
					int nCount = sqlite3_column_int( pStmt, i );
					if ( nCount != 0 )
					{
						m_vLuSum[m_mapLuFields[mapFieldName[i]]] += nCount;
					}
				}
			}

			n++;
		}
//		qDebug() << m_mapLuFields.size();
	}
	catch ( SLException &e )
	{
		if ( bCommit )
		{
			sqlite3_exec( m_pSQLiteHandle, "rollback", NULL, NULL, NULL );
		}
		emit sqlError( e.errorMessage() );
		return;
	}
	
	// search upstream
	QVector<QPair<int, int>> vnUpstreamRivCode;
	strSql = QString( "select gid,basinid from network where downstreamid = %1" ).arg( nUpstream.first );
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
	if ( nRet != SQLITE_OK )
	{
		throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
	}

	while ( true )
	{
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			break;
		}
		else if ( nRet == SQLITE_ROW )
		{
			vnUpstreamRivCode.push_back( qMakePair( sqlite3_column_int( pStmt, 0 ), sqlite3_column_int( pStmt, 1 ) ) );
		}
	}
	sqlite3_finalize( pStmt );

	for ( int i = 0; i < vnUpstreamRivCode.size(); i++ )
	{
		getUpstreamLu( vnUpstreamRivCode[i] );
	}
	vnUpstreamRivCode.clear();

/*
	for ( int i = 0; i < sqlite3_column_count( pStmt ); i++ )
		m_lstFields.push_back( QString::fromUtf8( sqlite3_column_name( pStmt, i ) ) );
*/
}
#endif

#ifndef OLD_VERSION

void QueryManage::aggregateAllLu()
{
	int nRet;

	// first aggregate all upstream LU
	aggregateUpstreamLu();

	// aggregate current mesh lu
	for ( int i = 0; i < m_vInitRivInfo.size(); i++ )
	{
		if ( !m_vSearchedBasins.contains( m_vInitRivInfo[i].nBasinId ) )
		{
			aggregateCurrentLu( m_vInitRivInfo[i].nRivCode, m_vInitRivInfo[i].dLrsLength, i == 0 ? true : false );
		}
	}
}


void QueryManage::aggregateCurrentLu( int nCurrentRivCode, double dLrsLength, bool bGetName )
{
	sqlite3_stmt *pStmt;
	char *pErrMsg = NULL;

	// create temp view of lumesh.
	QString strSql = QString( "create view tempmesh as select * from lumesh where rivid = %1" )
		.arg( nCurrentRivCode );
	qDebug() << strSql;
	int nRet = sqlite3_exec( m_pSQLiteHandle, strSql.toLocal8Bit(), NULL, NULL, &pErrMsg );
	if ( nRet != SQLITE_OK )
	{
		QString strn = QString::number( nRet );
		throw SLException( pErrMsg );
	}
	sqlite3_free( pErrMsg );

	// count target mesh
/*
with tempmesh as
(
select L.lu,count(L.lu),city_code
from lumesh_all L
where basinid = 1879
group by city_code,lu
)
select tempmesh.*,crop_propotion.*
from tempmesh,crop_propotion
where
tempmesh.city_code = crop_propotion.city_code
*/

	strSql = QString( 
		"with lutot as (select lu,count(lu),city_code from tempmesh where distance > %1 \
		 group by lu,city_code) select lutot.*, \
		P.rice,P.wheat,P.millet,P.potato,P.bean,P.craft_crop,P.veg_oa,P.veg_gh,P.flower_oa, \
		P.flower_gh,P.other_oa,P.other_gh,P.orange_u,P.orange_o,P.apple,P.grape,P.pear_j,\
		P.pear_w,P.peach,P.cherry,P.biwa,P.persimmon,P.chestnut,P.ume,P.plum,P.kiwi, \
		P.pineapple,P.other_fruit from lutot,crop_propotion P where P.city_code = lutot.city_code"
	).arg( dLrsLength );
	qDebug() << strSql;
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );

	while ( true )
	{
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			QString str = "";
			for ( QMap<int, double>::iterator p = m_mapResult.begin();
				 p != m_mapResult.end(); p++ )
			{
				str += QString::number( p.value() ) + "|";
			}
			qDebug() << str;
			break;
		}
		else if ( nRet == SQLITE_ROW )
		{
			QString strLu = QString::fromLocal8Bit( (const char *)sqlite3_column_text(pStmt, 0) );
			QVector<LUINFO>::const_iterator p = qFind( m_vLuFields, strLu );
			int nLuNumber = p->nID;
			qDebug() << strLu << nLuNumber;
			double dCount = (double)sqlite3_column_int(pStmt, 1);
			m_mapResult[nLuNumber] += dCount;
			if ( nLuNumber == 1 )
			{
				QString strCName = QString::fromLocal8Bit( sqlite3_column_name( pStmt, 3 ) );
				double dProp = sqlite3_column_double( pStmt, 3 );
				QVector<LUINFO>::const_iterator pp = qFind( m_vLuFields, strCName );
				int nSubLuNum = pp->nID;
				m_mapResult[nSubLuNum] += dCount * dProp;
			}
			else if ( nLuNumber == 2 )
			{
				for ( int i = 4; i < sqlite3_column_count(pStmt); i++ )
				{
					QString strCName = QString::fromLocal8Bit( sqlite3_column_name( pStmt, i ) );
					double dProp = sqlite3_column_double( pStmt, i );
					QVector<LUINFO>::const_iterator pp = qFind( m_vLuFields, strCName );
					int nSubLuNum = pp->nID;
					m_mapResult[nSubLuNum] += dCount * dProp;
				}
			}
		}

	}
	sqlite3_finalize( pStmt );

#ifdef _DEBUG
	for ( QMap<int, double>::iterator p = m_mapResult.begin();
		 p != m_mapResult.end(); p++ )
	{
		qDebug() << p.key() << p.value();
	}
#endif

	// query searched mesh
	strSql = QString( "select OGC_FID from tempmesh where distance > %1" ).arg( dLrsLength );
	qDebug() << strSql;
	nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
	while ( true )
	{
		nRet = sqlite3_step( pStmt );
		if ( nRet == SQLITE_DONE )
		{
			break;
		}
		else if ( nRet == SQLITE_ROW )
		{
			m_vSearchedMeshes.push_back( sqlite3_column_int( pStmt, 0 ) );
		}
	}
	sqlite3_finalize( pStmt );

	// get river name before drop temp view.
	if ( bGetName )
	{
		strSql = QString( "select w07_005 from tempmesh limit 1" );
		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		nRet = sqlite3_step( pStmt );
		m_strRivName = QString::fromUtf8((const char *)sqlite3_column_text( pStmt, 0 ));
		qDebug() << m_strRivName;
		sqlite3_finalize( pStmt );
	}

	// clear temp view
	strSql = QString( "drop view tempmesh;" );
	nRet = sqlite3_exec( m_pSQLiteHandle, strSql.toLocal8Bit(), NULL, NULL, &pErrMsg );
	sqlite3_free( pErrMsg );
	sqlite3_finalize( pStmt );
}


#endif // OLD_VERSION



void QueryManage::aggregateUpstreamLu()
{
	int nRet;
	sqlite3_stmt *pStmt;
	bool bCommit = false;
	char *pErrMsg = NULL;
//	QMap<int, QString> mapFieldName;
	QString strSql;

	if ( !m_vSearchedBasins.size() )
		return;

	QString strSubsetString = "basinid in (";
	for ( int i = 0; i < m_vSearchedBasins.size(); i++ )
	{
		strSubsetString += QString::number( m_vSearchedBasins[i] );
		if ( i != m_vSearchedBasins.size() - 1 )
			strSubsetString += ",";
		else
			strSubsetString += ")";
	}

	try
	{
		strSql = 
			QString( "select sum(lu_0100),sum(lu_0200),sum(lu_0500),sum(lu_0600), \
				sum(lu_0700),sum(lu_0901),sum(lu_0902),sum(lu_1000),sum(lu_1400), \
				sum(lu_1500),sum(lu_1600),sum(lu_0000), \
				sum(rice),sum(wheat),sum(millet),sum(potato),sum(bean),sum(craft_crop), \
				sum(veg_oa),sum(veg_gh),sum(flower_oa),sum(flower_gh),sum(other_oa), \
				sum(other_gh),sum(orange_u),sum(orange_o),sum(apple),sum(grape), \
				sum(pear_j),sum(pear_w),sum(peach),sum(cherry),sum(biwa),sum(persimmon), \
				sum(chestnut),sum(ume),sum(plum),sum(kiwi),sum(pineapple),sum(other_fruit) \
				from lusum where %1" ).arg( strSubsetString );
		qDebug() << strSql;

		nRet = sqlite3_prepare_v2( m_pSQLiteHandle, strSql.toLocal8Bit(), strSql.length(), &pStmt, NULL );
		if ( nRet != SQLITE_OK )
		{
			throw SLException( (char *)sqlite3_errmsg( m_pSQLiteHandle ) );
		}

		while ( true )
		{
			nRet = sqlite3_step( pStmt );
			if ( nRet == SQLITE_DONE )
			{
				QString str = "";
				for ( QMap<int, double>::iterator p = m_mapResult.begin();
					 p != m_mapResult.end(); p++ )
				{
					str += QString::number( p.value() ) + "|";
				}
				qDebug() << str;
				break;
			}
			else if ( nRet == SQLITE_ROW )
			{
				for ( int i = 0; i < sqlite3_column_count( pStmt ); i++ )
				{
					QString strField = QString::fromUtf8( sqlite3_column_name( pStmt, i ) );
					strField.remove( "sum(" );
					strField.remove( "lu_" );
					strField.remove( ")" );

					QVector<LUINFO>::const_iterator p = qFind( m_vLuFields, strField );

					double dCount = sqlite3_column_double( pStmt, i );
					m_mapResult[p->nID] += dCount;
				}
			}
		}
		sqlite3_finalize( pStmt );
	}
	catch ( SLException &e )
	{
		if ( bCommit )
		{
			sqlite3_exec( m_pSQLiteHandle, "rollback", NULL, NULL, NULL );
		}
		emit sqlError( e.errorMessage() );
		return;
	}

}
