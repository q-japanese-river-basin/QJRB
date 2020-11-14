# 河川流域土地利用算出プログラム

## はじめに
本プログラムは，国土数値情報と農林水産統計，および世界農業センサスを元に構築したデータベースを用いて，河川上任意の地点の上流域の面積，土地利用割合，水稲作付け率，畑地の地目・作目別面積を推定するものです．プログラム開発の詳細については，以下の論文を参照ください．
 
岩崎亘典・稲生圭哉・上田紘司（2020）河川上流域土地利用算出プログラムの開発と公開，地理情報システム学会第29回学術研究発表大会論文集

Nobusuke IWASAKI, Keiya INAO and Koji UEDA (2020) Development of tool and database for estimating river basin land use. Proceeding of 29th academic research presentation meeting of GIS Association of Japan (CD-ROM). 

## 動作確認環境
Windows 10 Pro 64bit版にて動作確認を行いました．また，背景画像表示のために，インターネットへの接続が必要です．

## 使用準備および起動
1. 本レポジトリの[programフォルダ](https://github.com/q-japanese-river-basin/QJRB/tree/master/program)をダウンロードして，PCに保存してください．その際，ファイルパスに日本語等の2バイト文字が含まれないようにしてください．
1. [データ配布用サイト](https://forms.gle/YSczUQqzAqxVPrgKA)から，背景画像として用いている国土数値情報土地利用細分メッシュラスタ版と，利用したい都道府県のデータファイルをダウンロードしてください．都道府県別のデータファイルは，**lrsX.7z**の様な名前になっています．**X**の部分が，[都道府県JISコード](https://nlftp.mlit.go.jp/ksj/gml/codelist/PrefCd.html)に対応しています．ただし，一桁の場合，0は付されません．
1. 都道府県別データファイルは，[7-zip形式](https://www.7-zip.org/)で圧縮されています．[LhaForge](https://forest.watch.impress.co.jp/library/software/lhaforge/)等のアプリケーションを使って解凍してください．
1. 解凍してできた**lrsX.sqlite**ファイルを**lrs.sqlite**とファイル名を変更し，[programフォルダ](https://github.com/q-japanese-river-basin/QJRB/tree/master/program)内のdataフォルダにコピーしてください．また，国土数値情報土地利用細分メッシュラスタ版（luraster.tif）についても，同じくdataフォルダにコピーしてください．
1. programフォルダ内のqgis_lrs.batダブルクリックすることで，河川流域土地利用算出プログラムが起動します．

## 使用方法
プログラムの起動画面は，下の図の通りです．
マップウィンドウには、背景として国土地理院の地図が表示され、その上に支流域界、河川流路およびその流下方向が表示されます。また、河川モニタリングの採水地点を選定する際の参考情報として、水質環境基準点の位置や土地利用図を表示可能です。凡例ウィンドでは、各データの表示・非表示の切り替えができます。なお、地理院地図はインターネット経由で取得されるので、ネットワークに接続されたパソコンでプログラムを実行する必要があります．

<img alt="プログラムの起動画面" src="https://github.com/q-japanese-river-basin/QJRB/blob/master/images/fig1.png" width="600" />


流域面積および流域内の土地利用面積の算出は、ユーザーがマップウィンドウで表示されている河川上の任意の地点をカーソルでクリックする方法と、対象とする複数地点の位置情報（緯度、経度）を入力したCSV形式のファイルを読み込んで、複数の地点について一括で算出する方法があります。地図上で対象地点を指定する場合には、ツールバー上の解析対象指定ボタン（下図）をクリックしてから、地図上で対象地点をクリックします。

<img alt="解析対象地点指定ボタン" src="https://github.com/q-japanese-river-basin/QJRB/blob/master/images/fig2.png" />

地図上で対象地点を指定した場合の解析結果を以下にに示します。マップウィンドウ上に対象地点と、解析結果である上流域が表示されます。集計結果ウィンドには、対象地点の座標、河川名、各土地利用面積およびその流域内での比率（%）が表示されます。

<img alt="地図上で対象地点を指定した場合の結果表示例" src="https://github.com/q-japanese-river-basin/QJRB/blob/master/images/fig3.png"  width="600"/>

また、土地利用の「田」および「その他農用地」の左にある＋記号をクリックすると、田では「水稲作付面積」が、その他農用地では「普通畑、その他畑」と「果樹」の地目別面積が表示されます（下図 (A)）。さらに、「普通畑、その他畑」と「果樹」の左にある＋記号をクリックすると、それぞれ主要品目の作付面積が表示されます（下図(B)）

<img alt="土地利用に係る地目・作目別結果の表示例" src="https://github.com/q-japanese-river-basin/QJRB/blob/master/images/fig4.png" />

解析結果保存ボタンをクリックすることで、集計結果をCSV形式のファイルとして保存することができ、エクセル等の表計算ソフトで利用可能です。集計結果の一部を下図に示します。一行目には指定した座標と選択された河川名が出力されます。土地利用の集計結果は、3行目以降にです。なお、プログラム上で集計する際には、「普通畑、その他畑」と「果樹」の地目別の集計面積も表示されますが、CSVでの出力では作目別の集計結果のみが表示されます。

<img alt="河川流域土地利用集計出力結果（CSV形式）" src="https://github.com/q-japanese-river-basin/QJRB/blob/master/images/fig5.png" />

## 注意事項
本プログラムで算出される面積は、国土数値情報の細分土地利用メッシュ1つ分の面積を1 haとして計算したものであり、実際の面積とは異なります。  
また作付面積は，統計情報を市町村単位で集計し，それを按分して求めたものである．そのため，実際の作付面積と一致しないことに注意が必要です．

## ライセンス
プログラムのライセンスは[GNU General Public License v2.0](https://github.com/q-japanese-river-basin/QJRB/blob/master/LICENSE)です．
データのライセンスは[国土数値情報の非商用](https://nlftp.mlit.go.jp/ksj/other/agreement.html#agree-02)です．
