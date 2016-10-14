EpgDataCap_Bon
==============
**BonDriver based multifunctional EPG software**

Documents are stored in the 'Document' directory.  
Configuration files are stored in the 'ini' directory.

**このフォークについて**

このフォークは、xtne6fさんのフォーク([xtne6f@work-plus-s](https://github.com/xtne6f/EDCB/tree/work-plus-s))にちょびっとだけパッチを追加するブランチ(フォーク)です。機能に関する説明やビルド方法などはフォーク元の[xtne6f@work-plus-s](https://github.com/xtne6f/EDCB/tree/work-plus-s)を参照してください。フォーク元との関係は[xtne6f@NetworkGraph](https://github.com/xtne6f/EDCB/network)で確認することが出来ます。

なお、フォーク元([xtne6f@work-plus-s](https://github.com/xtne6f/EDCB/tree/work-plus-s))のビルドでも、[設定]→[動作設定]→[その他]の「EpgTimerSrvの応答をtkntrec版互換にする」をチェック(またはEpgTimerSrv.iniの[SET]セクションに CompatFlags=4095 を追加)してEpgTimerSrvを再起動すると、このフォークのEpgTimerが動作します。xtne6f氏には感謝。

**注意点**

[2016/9/13までのビルド](https://github.com/tkntrec/EDCB/releases/tag/my-build-s-160913)からそれより新しいビルドへ変更すると、キーワード予約で「番組長で絞り込み」及び「同一番組無効予約で同一サービス確認を省略するオプション」を使用している場合、それらの設定が一度クリアされるので注意してください。  
(オプションを使用していなければ影響無ありません)。  

* 関連コミット → [1aa479b](https://github.com/tkntrec/EDCB/commit/1aa479b05a7f52a1de339c4098f846e72b0ac7ec)
* データ移行方法(非推奨) → [キーワード予約移行方法.txt](https://github.com/tkntrec/EDCB/files/491007/default.txt)

**主な変更点について**

このフォークでは主にEpgTimerへの変更を行っています。  
参考【[各画面キャプチャ](https://tkntrec.github.io/EDCB_PrtSc)】

* 自動予約登録に合わせて予約を変更するオプションを追加した。
* 右クリックメニュー項目を追加し、表示/非表示を選択出来るようにした。【[設定画面](https://tkntrec.github.io/EDCB_PrtSc/#i44)】  
ショートカットキーを変更したい場合は、設定ファイル(XML)で直接指定してください。
* 検索(キーワード予約)ダイアログに、キーワード予約操作用のボタンを追加した。【[画面イメージ](https://tkntrec.github.io/EDCB_PrtSc/#i16)】
* 予約などの情報を簡易に検索できるダイアログを追加した。【[画面イメージ](https://tkntrec.github.io/EDCB_PrtSc/#i161)】
* 各ダイアログなどをESCで閉じられるようにした。
* 設定画面がなく設定ファイル(XML)で直接指定するオプションなどの説明を含むコミット
  * [1c22086](https://github.com/tkntrec/EDCB/commit/1c220862bc75b84465d1c524227dbac1c8ee3e3b) 各設定画面でのフォルダ選択時に「ファイル選択ダイアログ」を使用するオプション
  * [52c598b](https://github.com/tkntrec/EDCB/commit/52c598b17a660fdbe090fcea7c937b3acfc464d8) 録画結果のドロップにカウントしないPIDを設定するオプション
  * [ff60480](https://github.com/tkntrec/EDCB/commit/ff6048074a4a609fb22c78361682a3cb4cf4a593) 予約情報等の強制更新(F5)を追加(仮)

**ブランチついて**

このフォークのブランチは再作成(リベース)などで構成が変わることがあります。  
[branch:my-build-s](https://github.com/tkntrec/EDCB/tree/my-build-s)以外、特にビルドする意味はありません。  
[branch:my-ui-s](https://github.com/tkntrec/EDCB/tree/my-ui-s)はフォーク元[xtne6f@work-plus-s](https://github.com/xtne6f/EDCB/tree/work-plus-s)とのEpgTimer側の差分、[branch:my-work-s](https://github.com/tkntrec/EDCB/tree/my-work-s8)はEpgTimerSrv側の差分です。