EpgDataCap_Bon
==============
**BonDriver based multifunctional EPG software**

Documents are stored in the 'Document' directory.  
Configuration files are stored in the 'ini' directory.

**このForkについて**

ビルド方法は[Document/HowToBuild.txt](https://github.com/xtne6f/EDCB/blob/work-plus/Document/HowToBuild.txt)を参照。  
各々のコミットを大まかに理解したうえで自由にマージやcherry-pickしてください。  
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

[branch:fix-etc](https://github.com/xtne6f/EDCB/tree/fix-etc)
* 個々のバグ修正ブランチ

[branch:fix-rsvman](https://github.com/xtne6f/EDCB/tree/fix-rsvman)
* EpgTimerSrvの予約管理まわりを修正するブランチ
* [80801db](https://github.com/xtne6f/EDCB/commit/80801db6e892071517e0f9578441c2c3ca61e17e) ～ [cbf5ac5](https://github.com/xtne6f/EDCB/commit/cbf5ac5c87620415bc860c44cece94a2007bfaf3) は必須でない
* [3df54ed](https://github.com/xtne6f/EDCB/commit/3df54ed72d55f8fa3d77792f77922d110885f71e) は絶対値の大きな録画マージンでおきる諸々の不具合を修正するもの
  * 負のマージンをつかって番組の一部だけ録画したりしても問題なくなるはず

[branch:cherry-picks](https://github.com/xtne6f/EDCB/tree/cherry-picks)
* 他Forkから安定していて特に良さそうなコミットを集めてくるブランチ
* バグ修正系のコミットが中心

[branch:misc-ui](https://github.com/xtne6f/EDCB/tree/misc-ui)
* おもにEpgTimer(NW)のUI周辺を弄るブランチ

[branch:cherry-picks-niisaka-epg](https://github.com/xtne6f/EDCB/tree/cherry-picks-niisaka-epg)
* [niisaka/EDCB](https://github.com/niisaka/EDCB)のEPG番組表を取り込むブランチ。番組表がかっこよくなる

[branch:fix-recname-macro](https://github.com/xtne6f/EDCB/tree/fix-recname-macro)
* RecName_Macroの修正と機能追加のブランチ
