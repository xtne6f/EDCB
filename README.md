EpgDataCap_Bon
==============
**BonDriver based multifunctional EPG software**

Documents are stored in the 'Document' directory.  
Configuration files are stored in the 'ini' directory.

**このForkについて**

ビルド方法は[Document/HowToBuild.txt](https://github.com/xtne6f/EDCB/blob/work-plus/Document/HowToBuild.txt)を参照。  
各々のコミットを大まかに理解したうえで自由にマージやcherry-pickしてください。大体[こんな方針](https://github.com/xtne6f/EDCB/pull/1)で改造しています。  
[branch:work](https://github.com/xtne6f/EDCB/tree/work)をベースに、以下をマージしたものが[branch:work-plus](https://github.com/xtne6f/EDCB/tree/work-plus)です。

[branch:misc](https://github.com/xtne6f/EDCB/tree/misc)
* 細かな重箱つつきのブランチ。機能追加やバグ修正はほとんどない
* EDCBをすべてVC++/# 2010 Expressでビルドできるようになる
* [64722ca](https://github.com/xtne6f/EDCB/commit/64722ca79b8c02cb6337504dc64a5c976ea8145b) ～ [fbb0f5c](https://github.com/xtne6f/EDCB/commit/fbb0f5c93cef731de44c00991f54cecd9eed2390) はVisualStudio製品版の人には不要(でも多分副作用はない)
* [accb255](https://github.com/xtne6f/EDCB/commit/accb255c34d5f2005b3ea5797f5671746b9ccbce) は人柱版10.69mod4k7までに変更された最適化オプションを元に戻すもの
* branch:misc2 → miscに吸収
  * EpgTimerTaskを機能追加するブランチ
  * [eaac62a](https://github.com/xtne6f/EDCB/commit/eaac62a083c2167f5829b996e5e46b255a1c3431) は開くアプリを選択できるようにした方がいいかも
  * [3972df2](https://github.com/xtne6f/EDCB/commit/3972df2c6ea2dbc8ebb1522d0704bbb32ae65729) は"EpgTimerTask.exe /StartSrv"のように起動することでサービス登録なしにEpgTimer.exeを常駐不要にするもの
* branch:misc3 → miscに吸収
  * EpgDataCap_Bon.exeを修正するブランチ(謎の無駄作業)
* [96dcd9b](https://github.com/xtne6f/EDCB/commit/96dcd9baea83e0293aa8e15a1993558df061fc50) は"ConvertText.txt"によるEPG検索文字列の置換機能を削除するので注意
* [9251d18](https://github.com/xtne6f/EDCB/commit/9251d18aada3ddc498cc1af10e30b5274fd04c6a) は"Reserve.txt"にID=0で項目を直接追加する機能を削除するので注意
* [a37f398](https://github.com/xtne6f/EDCB/commit/a37f398199f76222e7c354d39a0cef67fa2028b2) "Reserve.txt"はID順にソートするようになったが、連携ツール等が予約日時順でないとダメな場合は、このファイルの「;;NextID=」という行を消すと予約日時順に戻る

[branch:fix-etc](https://github.com/xtne6f/EDCB/tree/fix-etc)
* 個々のバグ修正ブランチ

[branch:fix-rsvman](https://github.com/xtne6f/EDCB/tree/fix-rsvman)
* EpgTimerSrvの予約管理まわりを修正するブランチ
* [80801db](https://github.com/xtne6f/EDCB/commit/80801db6e892071517e0f9578441c2c3ca61e17e) ～ [cbf5ac5](https://github.com/xtne6f/EDCB/commit/cbf5ac5c87620415bc860c44cece94a2007bfaf3) は必須でない
* [3df54ed](https://github.com/xtne6f/EDCB/commit/3df54ed72d55f8fa3d77792f77922d110885f71e) は絶対値の大きな録画マージンでおきる諸々の不具合を修正するもの
  * 負のマージンをつかって番組の一部だけ録画したりしても問題なくなるはず
* [7dd8b64](https://github.com/xtne6f/EDCB/commit/7dd8b643ed7e5ca21c435f1cdba1c329de34c399) で"RecInfo2Data.bin"はテキスト形式になって"RecInfo2.txt"に移動。「同一番組無効登録」機能のための情報が一度リセットされるので注意

[branch:cherry-picks](https://github.com/xtne6f/EDCB/tree/cherry-picks)
* 他Forkから安定していて特に良さそうなコミットを集めてくるブランチ
* バグ修正系のコミットが中心

[branch:misc-ui](https://github.com/xtne6f/EDCB/tree/misc-ui)
* おもにEpgTimer(NW)のUI周辺を弄るブランチ

[branch:cherry-picks-niisaka-epg](https://github.com/xtne6f/EDCB/tree/cherry-picks-niisaka-epg)
* [niisaka/EDCB](https://github.com/niisaka/EDCB)のEPG番組表を取り込むブランチ。番組表がかっこよくなる

[branch:fix-recname-macro](https://github.com/xtne6f/EDCB/tree/fix-recname-macro)
* RecName_Macroの修正と機能追加のブランチ
* [db9ac0c](https://github.com/xtne6f/EDCB/commit/db9ac0c08210374d3d7b1bfb88071d5d9bf38a6c) はマクロに以下の関数機能を追加する
  * ※関数部に$,&,(を含めるときは数値文字参照(&文字コード;)を使う
  * 【文字置換】Tr/置換文字リスト/置換後/  
  ・例: 番組名のA→a、$→B: $Tr/A&36;/aB/(Title)$
  * 【半角⇔全角】HtoZ,ZtoH
  * 【英数半角⇔全角】HtoZ\<alnum>,ZtoH\<alnum>
  * 【文字削除】Rm/削除文字リスト/  
  ・例: 番組内容から/を削除: $Rm!/!(SubTitle)$
  * 【足切り】Head文字数  
  ・例: 番組内容を半角にして最長15文字に: $Head15(ZtoH(SubTitle))$
