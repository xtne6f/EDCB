EpgDataCap_Bon
==============
**BonDriver based multifunctional EPG software**

Documents are stored in the 'Document' directory.  
Configuration files are stored in the 'ini' directory.

**このフォークについて**

人柱版10.69からの改変部分は[Document/Readme_Mod.txt](Document/Readme_Mod.txt)を参照。  
ビルド方法は[Document/HowToBuild.txt](Document/HowToBuild.txt)を参照。(自ビルド派の追加メモ[build_memo.txt](https://gist.github.com/xtne6f/f9b6f19c10cd146fe580))  
[0ac7692](https://github.com/xtne6f/EDCB/commits/0ac7692afe7cbe615534577facda15f57b5e5af9)の履歴確認のため、[コミットをバラしたタグ](https://github.com/xtne6f/EDCB/commits/log-mod4k7)を作りました。  
[01aff08](https://github.com/xtne6f/EDCB/commits/01aff08a5df4c7e63c86ea7136c20b259c08229e)にかけて改行コードの混乱があったようです。[改行のみ調整したタグ](https://github.com/xtne6f/EDCB/commits/log-to-crlf)も作りました(※改行以外の調整は一切無し)。

各々のコミットを大まかに理解したうえで自由にマージやcherry-pickしてください。大体[こんな方針](https://github.com/xtne6f/EDCB/pull/1)で改造しています。  
[branch:work](https://github.com/xtne6f/EDCB/tree/work)をベースに、以下をマージしたものが[branch:work-plus-s](https://github.com/xtne6f/EDCB/tree/work-plus-s)です。

[branch:misc](https://github.com/xtne6f/EDCB/tree/misc)
* 細かな重箱つつきのブランチ。たまに機能追加もある

[branch:misc-ui](https://github.com/xtne6f/EDCB/tree/misc-ui)
* おもにEpgTimer(NW)のUI周辺を弄るブランチ

[branch:edcb-plug-in](https://github.com/xtne6f/EDCB/tree/edcb-plug-in)
* EdcbPlugIn(TVTestプラグイン)のブランチ。説明は[releases](https://github.com/xtne6f/EDCB/releases)に添付
