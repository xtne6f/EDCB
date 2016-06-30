EpgDataCap_Bon
==============
**BonDriver based multifunctional EPG software**

Documents are stored in the 'Document' directory.  
Configuration files are stored in the 'ini' directory.

**このForkについて**

[0ac7692](https://github.com/xtne6f/EDCB/commit/0ac7692afe7cbe615534577facda15f57b5e5af9)の履歴確認のため、[コミットをバラしたタグ](https://github.com/xtne6f/EDCB/tree/log-mod4k7)を作りました。おもに自分用ですが参考にどうぞ。  
[01aff08](https://github.com/xtne6f/EDCB/commit/01aff08a5df4c7e63c86ea7136c20b259c08229e)にかけて改行コードの混乱があったようで、履歴確認しにくくなっています。[改行のみ調整したタグ](https://github.com/xtne6f/EDCB/tree/log-to-crlf)も作りました(※改行以外の調整は一切無し)。

人柱版10.69からの改変部分は[Document/Readme_Mod.txt](https://github.com/xtne6f/EDCB/blob/work-plus-s/Document/Readme_Mod.txt)を参照。  
ビルド方法は[Document/HowToBuild.txt](https://github.com/xtne6f/EDCB/blob/work-plus-s/Document/HowToBuild.txt)を参照。  
自ビルド派の追加メモ[build_memo.txt](https://gist.github.com/xtne6f/f9b6f19c10cd146fe580)  
CtrlCmdCLI.dllは不要になりましたが[branch:for-ctrlcmdcli](https://github.com/xtne6f/EDCB/tree/for-ctrlcmdcli)でビルドできます。  
各々のコミットを大まかに理解したうえで自由にマージやcherry-pickしてください。大体[こんな方針](https://github.com/xtne6f/EDCB/pull/1)で改造しています。  
[branch:work](https://github.com/xtne6f/EDCB/tree/work)をベースに、以下をマージしたものが[branch:work-plus-s](https://github.com/xtne6f/EDCB/tree/work-plus-s)です。

[branch:misc](https://github.com/xtne6f/EDCB/tree/misc)
* 細かな重箱つつきのブランチ。たまに機能追加もある
* [a37f398](https://github.com/xtne6f/EDCB/commit/a37f398199f76222e7c354d39a0cef67fa2028b2) "Reserve.txt"はID順にソートするようになったが、連携ツール等が予約日時順でないとダメな場合は、このファイルの「;;NextID=」という行を消すと予約日時順に戻る

[branch:fix-etc](https://github.com/xtne6f/EDCB/tree/fix-etc)
* 個々のバグ修正ブランチ

[branch:cherry-picks](https://github.com/xtne6f/EDCB/tree/cherry-picks)
* 他Forkから安定していて特に良さそうなコミットを集めてくるブランチ
* バグ修正系のコミットが中心

[branch:misc-ui](https://github.com/xtne6f/EDCB/tree/misc-ui)
* おもにEpgTimer(NW)のUI周辺を弄るブランチ

[branch:cherry-picks-niisaka-epg](https://github.com/xtne6f/EDCB/tree/cherry-picks-niisaka-epg)
* [niisaka/EDCB](https://github.com/niisaka/EDCB)のEPG番組表を取り込むブランチ。番組表がかっこよくなる

[branch:fix-recname-macro](https://github.com/xtne6f/EDCB/tree/fix-recname-macro)
* RecName_Macroの修正と機能追加のブランチ
