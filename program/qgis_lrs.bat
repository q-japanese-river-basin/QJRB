@echo off

set OSGEO4W_ROOT=%~dp0
set PATH=%OSGEO4W_ROOT%libs;%OSGEO4W_ROOT%qgis\bin;%WINDIR%system32;%WINDIR%;%WINDIR%WBem
set GDAL_DATA=%OSGEO4W_ROOT%share\gdal
set GEOTIFF_CSV=%OSGEO4W_ROOT%share\epsg_csv
set PROJ_LIB=%OSGEO4W_ROOT%share\proj
set QGIS_PREFIX_PATH=%OSGEO4W_ROOT%qgis
set QT_RASTER_CLIP_LIMIT=4096
set GDAL_FILENAME_IS_UTF8=YES
set VSI_CACHE=TRUE
set VSI_CACHE_SIZE=1000000
set QT_PLUGIN_PATH=%OSGEO4W_ROOT%qgis\bin\qtplugins;%OSGEO4W_ROOT%\libs\qtplugins

start "QGIS_LRS" /B qgis_lrs.exe
