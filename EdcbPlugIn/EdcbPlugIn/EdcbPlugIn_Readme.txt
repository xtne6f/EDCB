EdcbPlugInについて (2016-04-17)

EdcbPlugInはTVTestをEpgDataCap_Bonのように振舞わせるための、TVTestのプラグインで
す。つまりEDCB(以下、EpgTimer等のプロジェクトの総称として使います)からTVTest.exe
をEpgDataCap_Bon.exeの代わりに使えるようになります。
TVTestはEpgDataCap_Bonとは別のソフトです。エラー時の動作など挙動の違いに注意して
ください。EdcbPlugIn側での機能のすり合わせがまだ不十分なところもあると思います。

[使い方]
EdcbPlugInはTVTestの保存プラグイン機能の応用で実現しているため、TVTest ver.0.9.0
以降が必須です。混乱を避けるため、なるべくクリーンな(初期設定の)TVTestとEDCBを使
ってください。TVTestのチャンネルスキャンは済ませてください。

まず、PluginsフォルダにEdcbPlugIn.tvtpとEdcbPlugIn.iniを、TVTestのフォルダに
Write_Multi.dllとWrite_OneService.dllを、それぞれ入れます。EdcbPlugIn.iniを開い
てEdcbFolderPathキーを設定してください。
TVTestとEDCBのアーキテクチャ(x86/x64)が異なる場合は、TVTestのフォルダにTVTestの
アーキテクチャに合わせたEpgDataCap3.dllを置いてください。
TVTestを起動し、録画設定の「保存プラグイン」にWrite_Multi.dllを指定します。
EdcbPlugInに有効/無効の区別はありません。なにかあればTVTestのログで通知するので
ログに注意してください。TVTestの設定はこれで完了です。

EdcbPlugInにチャンネルスキャン機能はありません。あらかじめEpgDataCap_Bonでスキャ
ンしてもよいですが、TVTestの.ch2ファイルを添付のch2chset.vbsスクリプトにドロップ
して必要なファイル(ChSet4/ChSet5.txt)を生成できます。生成されたファイルはEDCBの
"Setting"フォルダに置いてください。
EpgTimer.exeを起動し、基本設定の「録画用アプリのexe」をTVTest.exeに変更します。
コマンドライン引数はデフォルトでも動きますが、-silentを追加したほうがよいです(
-nodshowもお勧め)。EDCBのその他の設定方法は従来と同じです。EpgTimerを再起動し、
EPG取得ボタンを押して正しく取得できるか確認してください。

[注意]
EpgTimerPlugin.tvtpとは併用できません。
予約の以下の設定項目は無視されます:
・ぴったり(？)録画は常に「しない」で動作
・指定サービス対象データ(字幕/データカルーセル)
・録画フォルダ設定は通常のものと部分受信のもの、それぞれ1つだけ有効
・録画フォルダの出力PlugIn設定
・録画モードのデコード処理の有無
また、録画は常に「全サービス」相当になりますが、Write_Multi.dllのプラグイン設定
でWritePluginキーを"Write_OneService.dll"に設定し、EdcbPlugIn.iniのRecNamePrefix
キーを"#$SID16$#"に設定することで「指定サービス」録画もできます。
Write_Multi.dllのソースは https://github.com/xtne6f/Write_Multi です。
