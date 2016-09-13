人柱版10.69からの改変説明

■概要■
Readme.txt、Readme_EpgDataCap_Bon.txt、Readme_EpgTimer.txtは基本的に人柱版10.69
のままです。更新履歴はHistory.txtに移されていますが、すでに内容を更新していませ
ん。正確な履歴は以下を参照してください:
https://github.com/xtne6f/EDCB/commits/work
https://github.com/xtne6f/EDCB/commits/log-mod4k7 (mod4k7までの履歴)
https://github.com/xtne6f/EDCB/commits/log-to-crlf (改行コード安定までの履歴)

このファイルでは上述のReadmeから改変された部分だけを説明します(他者の改変部分も
原則能動態で説明します)。なお、仕様に影響しないバグ修正や細かいデザイン改変は省
略します。追加機能については【追加】マークを付けています。

サービスとして使用しない場合、EpgTimer.exeやEpgTimerTask.exeの常駐を不要にできま
す(EpgTimerの動作設定タブ→その他)。この場合、EpgTimerSrv.exeがタスクトレイアイ
コンを表示し、スリープ確認ダイアログやチューナ・バッチを直接起動します。

HTTPサーバ機能はフル機能のWebサーバを組み込み、ページ生成をLuaに任せました。詳細
は 後述「CivetWebの組み込みについて」を参照してください。デフォルトではlocalhost
以外からのアクセスを拒否するので注意してください。

■Readme.txt■
◇動作環境
  必要なランタイムはビルド環境次第になります。
  OSは、おそらくXP SP3以降なら動くでしょう(XPはすでに終了しているので未確認)。

◇twitter.dllの取り扱いについて
  twitter.dllは削除しました。無視してください。

■Readme_EpgDataCap_Bon.txt■
◇設定
  ●基本設定タブ
    Readmeで触れられていませんが、相対パスは使用不可です(仕様変更ではない)。うま
    く動いているように動作する可能性もありますが誤動作です。安定性に問題が出るの
    で絶対パスを使用してください。
  ●動作設定タブ
    ・デバッグ出力をファイルに保存する【追加】
      デバッグ出力(OutputDebugStringW)をEpgDataCap_Bon.exeの起動数に応じて
      EpgDataCap_Bon_DebugLog-{番号}.txtに保存します。
    ・TS入力/ファイル出力バッファ上限【追加】
      EpgDataCap_Bon.iniのTsBuffMaxCount/WriteBuffMaxCountを設定します。
  ●EPG取得設定タブ
    ・基本情報のみ取得するネットワーク(視聴・録画中)【追加】
      動作設定タブにある、視聴中、録画中のEPGデータ取得について、基本情報のみ取
      得するかどうか設定します。
  ●ネットワーク設定タブ
    ・TCP送信
      ポート番号が22000～22999の範囲では送信形式がplainになります【追加】。

◇TSデータ/ファイル出力データのバッファリング最大値を変更する
設定値は「回数」ではなく「48128バイトのn倍」になります。
デフォルト値は従来通りです。

◇EPG取得のタイムアウト値を変更する
デフォルトは10分ではなく15分です(仕様変更でなく誤表記)

◇例外発生時のスタックトレース出力について【追加】
EpgDataCap_Bon.exeがなんらかの不具合で異常終了するとき、スタックトレースを
EpgDataCap_Bon.exe.errというテキストファイルに出力するようにしました(スタックト
レースは不具合特定の有用なヒントになることがあります)。また、ビルド時に生成され
るEpgDataCap_Bon.pdbが同じフォルダにあれば出力内容が詳細になります。

■Readme_EpgTimer.txt■
"EpgTimer.exe"を"EpgTimerNW～.exe"にファイル名をリネームすることで、EpgTimerNW相
当の動作になります。～には任意の文字列を指定可能で、この文字列が異なるEpgTimerNW
は多重起動できます。EpgTimer.exeはniisaka氏(https://github.com/niisaka)デザイン
のEPG番組表をベースにしたniisaka版と、従来に近いlegacy版の2種類あります。気分で
使い分けてください。録画済み番組情報"RecInfo2Data.bin"はテキスト形式になって
"RecInfo2.txt"に移動しました。移行する場合は「同一番組無効登録」機能のための情報
がリセットされるので注意してください。

◇主な機能
  サーバー連携機能は廃止しました。

◇使い方
  ◇各タブ
    タブの移動はショートカットキー(Ctrl+1～5)でも可能です。ちなみに(Ctrl+F)は検
    索ダイアログを開きます。【追加】
    ●自動予約登録
      ・予約ごと削除ボタン【追加】
        選択されている項目を、その検索条件にマッチする予約ごと削除します。

◇検索条件
  ・正規表現モード
    ConvertText.txtは廃止しました(後述)
  ・大小文字区別【追加】
    検索キーワードやNOTキーワードの大文字小文字(Aとaなど)を区別して検索します。
  ・自動登録を無効にする【追加】
    自動予約登録条件に追加するとき、その条件を無効にする(予約されなくなる)かどう
    かです。通常の検索で使用する意味はありません。

◇録画設定
  ・追従
    プログラム予約に切り替えるのと実質的な違いがほとんど無いため、「イベントリレ
    ー追従」に意味を変更しました。
  ・録画フォルダ
    録画フォルダを空欄のままにすると既定の録画保存フォルダになります【追加】。
    ファイル名PlugInにオプションの文字列を指定できます【追加】。オプションの意味
    はPlugIn次第ですが、RecName_Macro.dllではマクロを指定します。オプションを指
    定しなければ従来動作です。対応していないPlugInではオプションは無視されます。

◇設定
  ●基本設定タブ
    ●保存フォルダ
      ・コマンドライン引数【追加】
        録画用アプリの引数をカスタマイズします。EpgDataCap_Bon.exeの場合は弄る必
        要ありません。
        ・最小化: EPG取得や「最小化で起動する」設定の録画時に付加される
        ・非視聴時: 視聴時以外や「視聴時はViewを起動する」にチェックしていないと
                    きに付加される
      ・録画情報保存フォルダ【追加】
        .program.txt/.errの保存先を指定します(Common.iniのRecInfoFolderに相当)。
    ●チューナー
      利用可能なチューナー数のうちEPG取得に使用するチューナー数を設定できるよう
      になりました。【追加】
    ●EPG取得設定タブ
      EPG取得時間に曜日と取得種別を指定できます【追加】。種別はすぐ上にある「基
      本情報のみ取得するネットワーク」のチェックボックスで指定してください。
  ●動作設定タブ
    ●録画動作
      ・bat実行条件
        廃止しました(バッチごとに指定)。
      ・録画ファイルの容量確保を行う【追加】
        Bitrate.iniをもとに録画容量を予想してファイルにあらかじめブランク領域を
        確保します(EpgTimerSrv.iniのKeepDiskに相当)。並列録画時の断片化を抑制で
        きる可能性が高いですが、このようなファイルを追っかけ再生できるソフトは比
        較的少ないです。
    ●予約情報管理処理
      ・イベントリレーによる追従を行う
        廃止しました(予約ごとに指定)。
      ・EPGデータ読み込み時、予約時と番組名が変わっていれば番組名を変更する
        廃止しました(常にオン)。
      ・EPGデータ読み込み時、EventIDの変更を開始、終了時間のみで処理する
        廃止しました(常にオフ)。
      ・同一物理チャンネルで連続となるチューナーの使用を優先する
        廃止しました(常にオン)。
      ・ファイル名の禁則文字の変換対象から「\」を除外する【追加】
        PlugInが返すファイル名の禁則文字の変換対象から「\」を除外して、フォルダ
        階層を表現できるようにします(EpgTimerSrv.iniのNoChkYenに相当)。
      ・録画中の予約削除を【追加】
        ・削除のみ(従来動作)
        ・録画済みに追加
          「録画終了」として録画済み一覧に追加します。録画後実行batが実行され、
          成功として扱われる点に注意してください。
        ・キャンセルとして録画済みに追加
          「録画中にキャンセルされた可能性があります」として追加します。
    ●ボタン表示
      ・タブの位置に表示【追加】
        上部表示ボタンを上部タブと並列に配置します。
    ●その他
      ・ネットワーク接続を許可する
        ・アクセス制御【追加】
          EpgTimerSrv.exeが接続を許可するクライアントのIPアドレスを
            {許可+|拒否-}{ネットワーク}/{ネットマスク},...
          の形式で複数指定します。指定は左から順にクライアントと比較され、最後に
          マッチした指定の+-で可否が決まります(HTTPサーバ機能と同じルール)。
        ・無通信タイムアウト(秒)【追加】
          EpgTimerNWのネットワーク接続で「クライアント側に待ち受けポートを作る」
          をオフにしたときに使われるロングポーリングの再接続の間隔を指定します。
      ・EPG取得時に放送波時間でPC時計を同期する
        同期の信頼性を確保するため、150秒の放送波時間の観測を行います。EPG取得時
        間が延べ150(複数時は150÷チューナ数)秒に満たない場合は同期しません。
        仕様変更ではありませんが、厳密には管理者権限ではなくSE_SYSTEMTIME_NAME特
        権(コントロールパネル→ローカルセキュリティポリシー→システム時刻の変更)
        が必要です。判断は任せますが管理者起動させるよりもユーザにこの特権を与え
        るほうがマシな気がします。
      ・EpgTimerSrvを常駐させる【追加】
        ・タスクトレイアイコンを表示する【追加】
        ・バルーンチップでの動作通知を抑制する【追加】
          ※これらはEpgTimerSrv.exeが直接表示するものについての設定です
      ・情報通知ログをファイルに保存する【追加】
        情報通知ログをEpgTimerSrvのあるフォルダのEpgTimerSrvNotifyLog.txtに保存
        します。
      ・デバッグ出力をファイルに保存する【追加】
        EpgTimerSrvのデバッグ出力(OutputDebugStringW)をEpgTimerSrvのあるフォルダ
        のEpgTimerSrvDebugLog.txtに保存します。
      ・サーバー間連携
        廃止しました。
      ・タスクトレイアイコンを表示する【追加】
        タスクトレイアイコンの表示・非表示を切り替えます。
    ●Windowsサービス
      このタブは廃止しました。サービス登録、解除はiniフォルダにある以下のバッチ
      ファイルを管理者権限で起動してください。予めEpgTimerは閉じてください。
        ・EpgTimerSrv_Install.bat : サービス登録と開始
        ・EpgTimerSrv_Remove.bat : サービス停止と解除
      EpgTimerを管理者権限で起動する必要はありません(むしろ避けてください)。
  ●番組表タブ
    ●基本
      ・表示
        ・最低表示行数【追加】
          短時間の番組でも最低この行数だけ高さを確保します。短時間の番組が続くと
          下にずれるので、実用的には0.8程度をお勧めします。
        その他、niisaka版とlegacy版とで項目に若干違いがありますが、特に説明不要
        だと思うので省略します。
    ●表示項目
      ・カスタマイズ表示
        表示条件→表示サービスで、同一TSのサービス(全サービス録画でまとめて録画
        されるもの)を逆順に並べると、これらを結合表示できます【追加】。例：
          アフリカ中央テレビ3
          アフリカ中央テレビ2
          アフリカ中央テレビ1、の順に追加すると番組表上は1サービス分の幅で表示
  ●外部アプリケーション
    ●ファイル再生
      ・再生アプリのexeパス
        指定するとEpgTimerNWのファイル再生にもこのアプリを使います【追加】。
      ・追っかけ再生にも使用する【追加】
        追っかけ再生にも(NetworkTVモードではなく)このアプリを使います。
    ●Twitter設定タブ
      廃止しました。

◇スタンバイ、休止状態への移行
  次の予約録画またはEPG取得に対して、動作設定で指定した"復帰処理開始時間"+8分、
  かつ抑制条件で指定した時間以上の開きがある場合に移行します。

◇録画後のバッチファイル実行の仕様
  バッチのプロセス優先度は"通常以下"(BELOW_NORMAL_PRIORITY_CLASS)で実行します。
  以下の拡張命令を利用できます【追加】。拡張命令はバッチファイル内のどこかに直接
  記述してください(remコメント等どんな形式でもOK)。
  _EDCBX_BATMARGIN_={bat実行条件(分)}
    このマージン以上録画予定がないときに実行開始します。デフォルトは0です。
  _EDCBX_HIDE_
    ウィンドウを非表示にします。
  _EDCBX_NORMAL_
    ウィンドウを最小化しません。
  _EDCBX_DIRECT_
    マクロを置換ではなく環境変数で渡して直接実行します。マクロを囲う$が%になるだ
    けですが、start /waitを使って別のスクリプトに処理を引き継ぐときに便利です。
    また、以下を保証します:
      ・EpgTimerSrv.exeのあるフォルダに"EpgTimer_Bon_RecEnd.bat"を作らない
      ・EpgTimer.exeを経由する間接実行はしない
      ・バッチのカレントディレクトリはそのバッチのあるフォルダになる
  取得できるマクロについては以下の3行のコマンドで確認すると手っ取り早いです。
    >rem _EDCBX_DIRECT_
    >set
    >pause

◇マクロ
  RecName_Macro.dllに限り、以下の関数機能を利用できます。【追加】
  ※関数部に$,&,(を含めるときは数値文字参照(&文字コード;)を使う
  ・【文字置換】Tr/置換文字リスト/置換後/
    ・例：番組名のA→a、$→B: $Tr/A&36;/aB/(Title)$
  ・【半角⇔全角】HtoZ,ZtoH
  ・【英数半角⇔全角】HtoZ<alnum>,ZtoH<alnum>
  ・【文字列置換】S/置換文字列/置換後/
  ・【文字削除】Rm/削除文字リスト/
    ・例：番組内容から/を削除: $Rm!/!(SubTitle)$
  ・【足切り】Head文字数[省略記号]
    ・例：番組内容を半角にして最長15文字に: $Head15(ZtoH(SubTitle))$
    ・例：番組名を省略記号つき最長15文字に: $Head15~(Title)$

◇追従の仕様(この項上書き)
  ※あいまい検索処理によるEventID変更に対する追従処理は一切行いません。
  追従処理には大きく2種類あります。
  1.EPGデータ読み込み時に、読み込んだEPGデータから追従を行う
    ※このEPGデータはEpgTimerの番組表と同じものです
    プログラム予約、および2.で一度でも変更された予約は対象外です。追従するパラメ
    ータは開始時間、終了時間、およびイベント名です。

  2.起動中のEpgDataCap_Bon.exeの蓄積しているEPGデータから追従を行う
    起動中のチャンネルと異なるチャンネルの予約、6時間以上先の予約、プログラム予
    約、および無効予約は対象外です。追従するパラメータは開始時間、終了時間、イベ
    ント名、およびイベントリレーです。イベントリレーは現在番組(present)の情報の
    みを利用して行います。
    現在番組の終了時間が未定になった場合：
      現在番組の予約が録画終了(マージン含む)まで5分を切るタイミングで、5分ずつ予
      約を延長していきます。終了時間未定のまま番組が終わった場合、番組終了から5
      ～10分後に録画が終わることになります。終了時間が再度決定すれば、決定時間に
      変更します。

    次番組(following)の終了時間が未定になった場合：
      次番組の予約が録画終了(マージン含む)まで5分を切るタイミングで、5分ずつ予約
      を延長していきます。終了時間未定のまま次番組が切り替わった場合、後述の「現
      在でも次でもない番組」として扱います。終了時間が再度決定すれば、決定時間に
      変更します。
      また、このとき現在でも次でもない番組についても時間が定まらないので、これら
      の予約が録画終了(マージン含む)まで5分を切るタイミングで、録画総時間が
      TuijyuHourに達するまで、5分ずつ予約を延長していきます。次番組の終了時間が
      再度決定すれば、延長を停止します。この番組が現在または次番組になれば、その
      時間に変更します。
      一部の放送局で、未定解消直後の番組のイベントIDが変更されることがあります。
      これに対処するため、現在または次番組に未定追従中の予約とイベント名が完全一
      致するイベントが存在する場合に限り、その予約を番組終了時間まで延長します。

◇Twitter機能
  Twitter機能は廃止しました。代わりにEpgTimerSrv.exeのあるフォルダに置かれた以下
  のバッチファイルを実行します【追加】。取得できるマクロは従来とだいたい同じです
  (NEW系マクロは名前からNEWを取り除いています)。PostRecEnd.bat以外は$ReserveID$(
  予約ID)、$RecMode$(録画モード0=全サービス～4=視聴)、$ReserveComment$(コメント)
  も取得できます。
  ・PostAddReserve.bat : 予約を追加したとき(無効を除く)
    ・EPG自動予約のとき$ReserveComment$は"EPG自動予約"という文字列で始まります
  ・PostChgReserve.bat : 予約を変更したとき(無効を除く)
    ・$SYMDHMNEW$～$SEYMDHM28NEW$は取得できません(必要かどうか検討中…)
  ・PostRecStart.bat : 録画を開始したとき
  ・PostRecEnd.bat : 録画を終了したとき
    ・取得できるマクロは録画後バッチと完全に同じです
  また、イベント発生時に以下のバッチファイルを実行します。
  ・PostNotify.bat : 更新通知が送られたとき
    ・取得できるマクロは$NotifyID$のみです
      ・$NotifyID$(1=EPGデータ更新, 2=予約情報更新, 3=録画結果情報更新)
  バッチ仕様は録画後バッチと同じですが、_EDCBX_BATMARGIN_は無効です。また、
  _EDCBX_DIRECT_でないときの一時ファイル名は"EpgTimer_Bon_Post.bat"です。
  実行は直列に行います(互いに並列実行しない)。また、実行時点で各々の動作が完了し
  ている(予約ファイル等更新済みである)ことを保証します。
  iniフォルダに簡単な参考用バッチファイルを用意しました。

◇Q&A
  ・予約割り振りの仕方を詳しく
    全予約を開始時間でソートし、予約のない時間帯ごとに組分け、組ごとの予約に対し
    "予約優先度>開始時間(終ろ優先時逆順)>予約ID"の一意な優先度をつけ、優先度順に
    予約をチューナに割り当てます。ここで、優先度をもとに実際の録画時間を計算し、
    最長となるチューナを選びます(同じ長さならBonDriverの優先度順)。チューナ強制
    指定時はここで選べるチューナが限定されることになります。録画時間が0分ならチ
    ューナ不足となります。
  ・開始と終了時間重なっているときの動作を詳しく
    別チャンネルの場合、前番組の優先度が高いときはそれが終わるまで後番組の録画は
    始まりません。後番組の優先度が高いときは、その開始20秒前に前番組の録画を終了
    します。マージンを含めて録画時間です。マージン無視等の仕様は廃止しました。
  ・プログラム的に登録する方法は？
    Reserve.txtにID 0で項目を直接追加する機能は廃止しました。
  ・EPG取得でBS、CS1、CS2の基本情報のみ取得て何？
    CS3(NetworkID=10)を追加しました【追加】。
  ・追従できるだけ失敗しないための対策とかある？
    仕様変更ではなく注意ですが、デジタル放送の仕様上、午前0時を跨いでのEPG取得は
    避けるのが無難です。

◇キーワード検索の置き換え文字を変更する
  ConvertText.txtは廃止しました。常に以下のテーブルを使います。
  <置換前>
  ０１２３４５６７８９
  ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ
  ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ
  ’”{全角空白}！＃＄％＆（）＊＋，－．／：；＜＝＞？＠［］＾＿｀｛｜｝～｡｢｣､･
  ｦｧｨｩｪｫｬｭｮｯｰｱｲｳｴｵ
  ｶﾞ ｷﾞ ｸﾞ ｹﾞ ｺﾞ ｶｷｸｹｺ
  ｻﾞ ｼﾞ ｽﾞ ｾﾞ ｿﾞ ｻｼｽｾｿ
  ﾀﾞ ﾁﾞ ﾂﾞ ﾃﾞ ﾄﾞ ﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉ
  ﾊﾞ ﾋﾞ ﾌﾞ ﾍﾞ ﾎﾞ ﾊﾟ ﾋﾟ ﾌﾟ ﾍﾟ ﾎﾟ ﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝﾞﾟ￥
  <置換後>
  0123456789
  ABCDEFGHIJKLMNOPQRSTUVWXYZ
  abcdefghijklmnopqrstuvwxyz
  '"{半角空白}!#$%&()*+,-./:;<=>?@[]^_`{|}~。「」、・
  ヲァィゥェォャュョッーアイウエオ
  ガギグゲゴカキクケコ
  ザジズゼゾサシスセソ
  ダヂヅデドタチツテトナニヌネノ
  バビブベボパピプペポハヒフヘホマミムメモヤユヨラリルレロワン゛゜\

◇追従動作のカスタマイズ
  ・NoEpgTuijyuMinは廃止しました(常に0)
  ・DuraChgMarginMinは廃止しました(常に0)
  ・TuijyuHourのデフォルトは3時間になりました

◇予約割り振りのアルゴリズムの変更
  廃止しました。

◇正常に録画を行えた番組情報として判断するためのドロップ数を変更する
  ドロップカウントの仕様変更により、RecInfo2DropChkのデフォルトは2になりました。

◇ブラウザから表示できるようにする
  「CivetWebの組み込みについて」を参照。

◇録画後bat起動時の形式を変える
  非サービス時は常に最小化で起動します。

◇デザインをロードしない
  NoStyleが0(デフォルト)のとき、EpgTimerのあるフォルダにEpgTimer.exe.rd.xamlがあ
  れば、そこに定義されたリソースを適用します【追加】。iniフォルダに簡単なサンプ
  ルを用意したので参考にしてください。

◇DLNAのDMSぽい機能を使う
  「CivetWebの組み込みについて」を参照。

◇同一番組無効登録で番組名の比較の際に無視する文字列を指定する【追加】
  [無]や[生]などのついた番組名も同一番組として扱いたい場合に利用することを想定し
  たもの。EpgTimerSrv.iniのSETにRecInfo2RegExpを追加することで指定可能です。文字
  列は正規表現として扱われ、動作としては、番組名から正規表現にマッチする部分を削
  除したものが同一番組名判定に使われます。
  例：RecInfo2RegExp=\[[再無生]\]
      （[再]と[無]と[生]に対応）

◇スリープ抑止拡張【追加】
  設定→動作設定→録画動作→抑止条件→[PCを使用中の場合]にチェックを入れ、録画終
  了時や番組表取得完了時にPCを使用(マウスやキーボード操作)していた場合、スリープ
  へのカウントダウンが表示されなくなります。また、その下にある[X分以内にユーザー
  操作があれば使用中とみなす]で使用中かどうかの判定時間を調整できます。ここを0に
  した場合は常に使用中であるとみなします(スリープしなくなります）。

◇バルーンチップを強制的に閉じるまでの時間(秒数)を指定する【追加】
  (Vista以降？の)バルーンチップはマウス操作等がなければ表示されつづけますが、
  EpgTimer.exe.xmlの<ForceHideBalloonTipSec>で表示タイムアウトを指定できます。

◇EpgTimerSrvのタスクトレイ左クリック動作を変更する【追加】
  デフォルトではEpgTimerSrv.exeと同じ場所のEpgTimer.exeを実行しますが、EpgTimer
  という名前のショートカットファイルがあればこちらを実行します。

◇Write_DefaultのTeeコマンド機能について【追加】
Write_Defaultの通常のファイル出力に平行して、出力と同じデータをPlugIn設定で指定
されたコマンドの標準入力に渡します。コマンドには$FilePath$(出力ファイルパス)を指
定できます。コマンドのカレントディレクトリは親プロセス(EpgDataCap_Bon.exeなど)の
あるフォルダになります。
録画終了後、標準入力は速やかに閉じられます(入力完了まで待機することはない)。コマ
ンドの処理速度が録画速度を下回る場合は終了後の処理について(引数で受け取った出力
ファイルパスをもとに処理を継続するなど)コマンド側で工夫してください。
・Teeバッファサイズ(byte) : コマンドにデータを入力する単位
・Tee読み込み遅延(byte) : コマンドへの入力をファイル出力からこの値だけ遅らせる

■CivetWebの組み込みについて■
HTTPサーバ機能の簡単化とディレクトリトラバーサル等々のバグ修正を目的に、EpgTimerSrv.exeにCivetWebを組み込みました。
HTTPサーバ機能は従来通りEpgTimerSrv.iniのEnableHttpSrvキーを1にすると有効になります(2にするとEpgTimerSrv.exeと同じ場所にログファイルも出力)。
有効にする場合はEpgTimerSrv.exeと同じ場所にlua52.dllが必要です。対応するものをDLしてください。
https://sourceforge.net/projects/luabinaries/files/5.2.4/Windows%20Libraries/Dynamic/
CivetWebについては本家のドキュメント↓を参照してください(英語) ※組み込みバージョンはv1.8
https://github.com/civetweb/civetweb/blob/master/docs/UserManual.md
SSL/TLSを利用する場合はEpgTimerSrv.exeと同じ場所にssleay32.dllとlibeay32.dllが必要です。自ビルド(推奨)するか信頼できるどこかから入手してください。
とりあえず https://www.openssl.org/community/binaries.html にある https://indy.fulgan.com/SSL/ で動作を確認しています。

「DLNAのDMSぽい機能」はHTTPサーバに統合しました。
iniフォルダにあるdlna以下を公開フォルダに置いてEpgTimerSrv.iniのEnableDMSキーを1にすると有効になります。

EpgTimerSrv.iniのSETに以下のキー[=デフォルト]を指定できます:
HttpAccessControlList[=+127.0.0.1]
  アクセス制御
  # CivetWebのaccess_control_listに相当(ただし"deny all accesses"からスタート)
  # 従来通りすべてのアクセスを許可する場合は+0.0.0.0/0とする
  # ※+0.0.0.0/0は最終手段。キャリアの技術情報やプロキシを活用して接続元をできるだけ限定すべき
HttpPort[=5510]
  ポート番号
  # CivetWebのlistening_portsに相当
  # SSLや複数ポート指定方法などは本家ドキュメント参照
HttpPublicFolder[=EpgTimerSrv.exeと同じ場所の"HttpPublic"]
  公開フォルダ
  # CivetWebのdocument_rootに相当
  # フォルダパスに日本語(マルチバイト)文字を含まないこと
HttpAuthenticationDomain[=CivetWebのデフォルト]
  認証領域
  # CivetWebのauthentication_domainに相当
  # パスワード確認画面の文字列ぐらいの役割しかない
HttpNumThreads[=5]
  ワーカスレッド数
  # CivetWebのnum_threadsに相当
  # 最大50
HttpRequestTimeoutSec[=120]
  リクエストタイムアウト(秒)
  # CivetWebのrequest_timeout_msに相当
HttpSslCipherList[=HIGH:!aNULL:!MD5]
  使用するSSL/TLSの暗号スイートのリスト
  # CivetWebのssl_cipher_listに相当
HttpSslProtocolVersion[=2]
  受け入れるSSL/TLSプロトコルのバージョン
  # CivetWebのssl_protocol_versionに相当
  # 値が大きいほど安全。ガラケーなどでSSL3.0が必要な場合は1にする
HttpKeepAlive[=0]
  Keep-Aliveを有効にするかどうか
  # CivetWebのenable_keep_aliveに相当
  # 有効にする[=1]ときは以下に注意:
  # ・本家ドキュメントによると"Experimental feature"(実験的な機能)
  # ・HttpNumThreadsを大きめ(同時閲覧ブラウザ数×6)にすべき
  # ・最大HttpRequestTimeoutSecまでEpgTimerSrv.exeの終了処理が滞るかもしれない
  # ・mg.keep_alive(true)メソッドを呼んだLuaスクリプトは持続的接続になるかもしれない。
  #   このメソッドがtrueを返したときは"Content-Length"を必ず送り、"Connection: close"しない

加えて、以下の設定をCivetWebのデフォルトから変更しています:
  ssi_pattern: "" (SSIは無効)
  extra_mime_types: "ContentTypeText.txt"に追加したMIMEタイプ(従来通り)
  lua_script_pattern: "**.lua$|**.html$|*/api/*$" (つまり.htmlファイルはLuaスクリプト扱いになる)
    ※REST APIとの互換のため、公開フォルダ直下のapiフォルダにあるファイルもLuaスクリプト扱いになる
  ssl_certificate: HttpPortに文字's'を含むとき、EpgTimerSrv.exeと同じ場所の"ssl_cert.pem"
  ssl_ca_file: EpgTimerSrv.exeと同じ場所の"ssl_peer.pem"
  ssl_verify_peer: "ssl_peer.pem"が存在するとき"yes"
  global_auth_file: EpgTimerSrv.exeと同じ場所の"glpasswd"

公開フォルダ以下のフォルダやファイルが公開対象です(色々遊べる)。
iniフォルダに原作っぽい動作をするLuaスクリプトを追加したので参考にしてください。

外部公開は推奨しませんが、もしも行う場合は以下を参考に"ssl_peer.pem"または"glpasswd"を作成し(フォルダごとの.htpasswdは不確実)、
SSL/TLSを利用してください。さらに重要なデータから隔離してください。
CivetWebでセキュリティが確保されているだろうと判断できたのは認証処理までの不正アクセス耐性のみです。
DoS耐性は期待できませんし、パス無しの公開サーバとしての利用もお勧めできません。

"ssl_cert.pem"(秘密鍵+自己署名証明書)の作成手順は本家ドキュメントに従わないほうが良いです。暗号強度が
「SSL/TLS暗号設定ガイドライン」(https://www.ipa.go.jp/security/vuln/ssl_crypt_config.html)を満たしていません。
(作成手順例、鵜呑みにしないこと)
> openssl genrsa -out server.key 2048
> openssl req -new -key server.key -out server.csr
> openssl x509 -req -days 3650 -sha256 -in server.csr -signkey server.key -out server.crt
> copy server.crt ssl_cert.pem
> type server.key >> ssl_cert.pem

"ssl_peer.pem"(信頼済みクライアント証明書リスト)の作成手順例
> openssl genrsa -out client.key 2048
> openssl req -new -key client.key -out client.csr
> openssl x509 -req -days 3650 -sha256 -in client.csr -signkey client.key -out client.crt
> copy client.crt > ssl_peer.pem
> openssl pkcs12 -export -inkey client.key -in client.crt -out edcb_key.p12 -name "edcb_key"
> (↑"edcb_key.p12"(クライアントの秘密鍵)はブラウザ等にインポートする)

"glpasswd"(ダイジェスト認証ファイル)の簡単な作り方(ユーザ名root、認証領域mydomain.com、パスワードtest)
> set <nul /p "x=root:mydomain.com:test" | openssl md5
  (出力される32文字のハッシュ値でパスワードを上書き↓)
> set <nul /p "x=root:mydomain.com:351eee77bbb11db9fef4870b0d78b061" >glpasswd

Luaのmg.write()について、成否のブーリアンを返すよう拡張しています(本家に取り込み予定)。

■Lua edcbグローバル変数の仕様■
機能はEpgTimerSrv本体にあるメソッドとほぼ同じなので
C++を読める人はEpgTimerSrvMain.cppにある実装を眺めると良いかもしれない。

[略語の定義]
B:ブーリアン
I:整数
S:文字列
TIME:標準ライブラリos.date('*t')が返すテーブルと同じ
<テーブル名>:後述で定義するテーブル
～のリスト:添え字1からNまでN個の～を添え字順に格納したテーブル

htmlEscape:I
  文字列返却値の実体参照変換を指示するフラグ(+1=amp,+2=lt,+4=gt,+8=quot,+16=apos)
  初期値は0。
  例えばedcb.htmlEscape=15とすると'<&"テスト>'は'&lt;&amp;&quot;テスト&gt;'のように変換される。

serverRandom:S
  EpgTimerSrv.exeの起動毎に変化する256bitの暗号論的乱数

S GetGenreName( 大分類*256+中分類:I )
  STD-B10のジャンル指定の文字列を取得する
  中分類を0xFFとすると大分類の文字列が返る。
  存在しないとき空文字列。
  例えばedcb.GetGenreName(0x0205)は'グルメ・料理'が返る。

S GetComponentTypeName( コンポーネント内容*256+コンポーネント種別:I )
  STD-B10のコンポーネント記述子の文字列を取得する
  存在しないとき空文字列。
  例えばedcb.GetComponentTypeName(0x01B1)は'映像1080i(1125i)、アスペクト比4:3'が返る。

S|nil Convert( to文字コード:S, from文字コード:S, 変換対象:S )
  文字コード変換する
  利用できる文字コードは'utf-8'または'cp932'のみ。
  変換に失敗すると空文字列、利用できない文字コードを指定するとnilが返る。
  例：os.execute(edcb.Convert('cp932','utf-8','echo 表が怖い & pause'))

Sleep( ミリ秒:I )
  スレッドの実行を中断する

S GetPrivateProfile( セクション:S, キー:S, 既定値:S|I|B, ファイル名:S )
  Win32APIのGetPrivateProfileStringを呼ぶ
  既定値がBのときはtrue=1、false=0に変換される。
  ファイル名はEDCBフォルダ配下に置かれたiniファイルを相対指定する。
  例：v=0+edcb.GetPrivateProfile('SET','HttpPort',5510,'EpgTimerSrv.ini')
  ファイル名が'Setting\\'で始まるときは「設定関係保存フォルダ」にリダイレクトされる。
  特例的に以下の引数でEDCBフォルダのパスが返る。
  edcb.GetPrivateProfile('SET','ModulePath','','Common.ini')

B WritePrivateProfile( セクション:S, キー:S|nil, 値:S|I|B|nil, ファイル名:S )
  Win32APIのWritePrivateProfileStringを呼ぶ
  値がBのときはtrue=1、false=0に変換される。
  キーまたは値がnilのときの挙動はWritePrivateProfileStringと同じ。
  ファイル名についてはGetPrivateProfile()と同じ。
  書き込めたときtrueが返る。

B ReloadEpg()
  EPG再読み込みを開始する
  開始できたときtrueが返る。

ReloadSetting( ネットワーク設定を読み込むか:B )
  設定を再読み込みする
  引数をtrueにするとEpgTimerSrv.iniの以下のキーも読み込まれる(HTTPサーバは再起動する)。
  EnableTCPSrv, TCP*, EnableHttpSrv, Http*, EnableDMS

B EpgCapNow()
  EPG取得開始を要求する
  要求が受け入れられたときtrueが返る。

<チャンネル情報>のリスト GetChDataList()
  チャンネルスキャンで得たチャンネル情報のリストを取得する(onid>tsid>sidソート)
  つまり"Setting\ChSet5.txt"の内容。

<サービス情報>のリスト|nil GetServiceList()
  EPG取得で得た全サービス情報を取得する(onid>tsid>sidソート)
  プロセス起動直後はnil(失敗)。

{minTime:TIME, maxTime:TIME}|nil GetEventMinMaxTime( ネットワークID:I, TSID:I, サービスID:I )
  指定サービスの全イベントについて最小開始時間と最大開始時間を取得する
  開始時間未定でないイベントが1つもなければnil。

<イベント情報>のリスト|nil EnumEventInfo( {onid:I|nil, tsid:I|nil, sid:I|nil}のリスト [, {startTime:TIME|nil, durationSecond:I|nil} ] )
  指定サービスの全イベント情報を取得する(onid>tsid>sid>eidソート)
  リストのいずれかにマッチしたサービスについて取得する。
  onid,tsid,sidフィールドを各々nilとすると、各々すべてのIDにマッチする。
  プロセス起動直後はnil(失敗)。
  第2引数にイベントの開始時間の範囲を指定できる。
  ・開始時間がstartTime以上～startTime+durationSecond未満のイベントにマッチ
  ・空テーブルのときは開始時間未定のイベントにマッチ

<イベント情報>のリスト|nil SearchEpg( <自動予約検索条件> )
<イベント情報>|nil SearchEpg( ネットワークID:I, TSID:I, サービスID:I, イベントID:I )
  イベント情報を検索する
  1引数のときは<自動予約検索条件>にマッチしたイベントを取得する。
  4引数のときは指定イベントを取得する。なければnilが返る。
  プロセス起動直後はnil(失敗)。

B AddReserveData( <予約情報> )
  予約を追加する
  失敗時はfalse。

B ChgReserveData( <予約情報> )
  予約を変更する
  失敗時はfalse。

DelReserveData( 予約ID:I )
  予約を削除する

<予約情報>のリスト GetReserveData()
<予約情報>|nil GetReserveData( 予約ID:I )
  予約を取得する(reserveIDソート)
  無引数のときは全予約を取得する。
  1引数のときは指定予約を取得する。なければnilが返る。

S|nil GetRecFilePath( 予約ID:I )
  予約の録画ファイルパスを取得する
  録画中でなかったり視聴予約などで取得できなければnilが返る。

<録画済み情報>のリスト GetRecFileInfo()
<録画済み情報>|nil GetRecFileInfo( 情報ID:I )
  録画済み情報を取得する(idソート)
  無引数のときは全情報を取得する。
  1引数のときは指定情報を取得する。なければnilが返る。

<録画済み情報>のリスト GetRecFileInfoBasic()
<録画済み情報>|nil GetRecFileInfoBasic( 情報ID:I )
  基本的な録画済み情報を取得する(idソート)
  programInfoとerrInfoが常に空文字列になる以外はGetRecFileInfo()と同じ。
  programInfoとerrInfoを取得するためのファイルアクセスのコストが無い。
  例：このメソッドが存在するならこれを使ってリストを取得する
      a=edcb.GetRecFileInfoBasic and edcb.GetRecFileInfoBasic() or edcb.GetRecFileInfo()

ChgProtectRecFileInfo( 情報ID:I, プロテクト:B )
  録画済み情報のプロテクトを変更する

DelRecFileInfo( 情報ID:I )
  録画済み情報を削除する

<チューナ予約情報>のリスト GetTunerReserveAll()
  チューナごとの予約の割り当て情報を取得する(tunerIDソート)
  ただし、リストの最終要素はチューナ不足の予約を表す。

<録画プリセット>のリスト EnumRecPresetInfo()
  録画プリセット情報を取得する(idソート)
  少なくともデフォルトプリセット(id==0)は必ず返る。

<自動予約登録情報>のリスト EnumAutoAdd()
  自動予約登録情報を取得する(dataIDソート)

<自動予約(プログラム)登録情報>のリスト EnumManuAdd()
  自動予約(プログラム)登録情報を取得する(dataIDソート)

DelAutoAdd( 登録ID:I )
  自動予約登録情報を削除する

DelManuAdd( 登録ID:I )
  自動予約(プログラム)登録情報を削除する

B AddOrChgAutoAdd( <自動予約登録情報> )
  自動予約登録情報を追加/変更する
  dataIDフィールドが0のとき追加の動作になる。
  失敗時はfalse。

B AddOrChgManuAdd( <自動予約(プログラム)登録情報> )
  自動予約(プログラム)登録情報を追加/変更する
  dataIDフィールドが0のとき追加の動作になる。
  失敗時はfalse。

I GetNotifyUpdateCount( 通知ID:I )
  更新通知のカウンタを取得する
  0から始まってイベントが発生するたびに1だけ増える数値が返る。
  取得できない通知IDを指定すると-1が返る。
  通知ID:
    1=EPGデータが更新された
    2=予約情報が更新された
    3=録画済み情報が更新された
    4=自動予約登録情報が更新された
    5=自動予約(プログラム)登録情報が更新された

<ファイル情報>のリスト ListDmsPublicFile()
  (公開フォルダ)/dlna/dms/PublicFileにあるファイルをリストする
  「DLNAのDMSぽい機能」用。

■テーブル定義■
<チャンネル情報>={
  onid:I=ネットワークID
  tsid:I=TSID
  sid:I=サービスID
  serviceType:I=サービスタイプ
  partialFlag:B=限定受信サービス(ワンセグ)かどうか
  serviceName:S=サービス名
  networkName:S=ネットワーク名
  epgCapFlag:B=EPGデータ取得対象かどうか
  searchFlag:B=検索時のデフォルト検索対象サービスかどうか
}

<サービス情報>={
  onid:I=ネットワークID
  tsid:I=TSID
  sid:I=サービスID
  service_type:I=サービス形式種別
  partialReceptionFlag:B=限定受信サービス(ワンセグ)かどうか
  service_provider_name:S=事業者名
  service_name:S=サービス名
  network_name:S=ネットワーク名
  ts_name:S=TS名
  remote_control_key_id:I=リモコンキー識別
}

<イベント情報>={
  onid:I=ネットワークID
  tsid:I=TSID
  sid:I=サービスID
  eid:I=イベントID
  startTime:TIME|nil=開始時間(不明のときnil)
  durationSecond:I|nil=総時間(不明のときnil)
  freeCAFlag:B=ノンスクランブルフラグ
  shortInfo:{
    event_name:S=イベント名
    text_char:S=情報
  }|nil=EPG基本情報(ないときnil、以下同様)
  extInfo:{
    text_char:S=詳細情報
  }|nil=EPG拡張情報
  contentInfoList:{
    content_nibble:I=大分類*256+中分類
    user_nibble:I=独自ジャンル大分類*256+独自ジャンル中分類
  }のリスト|nil=EPGジャンル情報
  componentInfo:{
    stream_content:I=コンポーネント内容
    component_type:I=コンポーネント種別
    component_tag:I=コンポーネントタグ
    text_char:S=コンポーネント記述
  }|nil=EPG映像情報
  audioInfoList:{
    stream_content:I=以下、STD-B10音声コンポーネント記述子を参照
    component_type:I=*
    component_tag:I=*
    stream_type:I=*
    simulcast_group_tag:I=*
    ES_multi_lingual_flag:B=*
    main_component_flag:B=*
    quality_indicator:I=*
    sampling_rate:I=*
    text_char:S=*
  }のリスト|nil=EPG音声情報
  eventGroupInfo:{
    group_type:I=グループ種別。必ず1(イベント共有)
    eventDataList:{onid:I, tsid:I, sid:I, eid:I}のリスト
  }|nil=EPGイベントグループ情報
  eventRelayInfo:{
    group_type:I=グループ種別
    eventDataList:{onid:I, tsid:I, sid:I, eid:I}のリスト
  }|nil=EPGイベントグループ(リレー)情報
}

<予約情報>={
  title:S=番組名
  startTime:TIME=録画開始時間
  durationSecond:I=録画総時間
  stationName:S=サービス名
  onid:I=ネットワークID
  tsid:I=TSID
  sid:I=サービスID
  eid:I=イベントID
  comment:S=コメント
  reserveID:I=予約ID
  overlapMode:I=かぶり状態
  startTimeEpg:TIME=予約時の開始時間
  recSetting:<録画設定>=録画設定
}

<録画設定>={
  recMode:I=録画モード
  priority:I=優先度
  tuijyuuFlag:B=イベントリレー追従するかどうか
  serviceMode:I=処理対象データモード
  pittariFlag:B=ぴったり?録画
  batFilePath:S=録画後BATファイルパス
  suspendMode:I=休止モード
  rebootFlag:B=録画後再起動する
  startMargin:I|nil=録画開始時のマージン(デフォルトのときnil)
  endMargin:I|nil=録画終了時のマージン(デフォルトのときnil)
  continueRecFlag:B=後続同一サービス時、同一ファイルで録画
  partialRecFlag:I=物理CHに部分受信サービスがある場合、同時録画する(=1)か否(=0)か
  tunerID:I=強制的に使用Tunerを固定
  recFolderList={
    recFolder:S=録画フォルダ
    writePlugIn:S=出力PlugIn
    recNamePlugIn:S=ファイル名変換PlugIn
  }のリスト=録画フォルダパス
  partialRecFolder={
    (recFolderListと同じ)
  }のリスト=部分受信サービス録画のフォルダ
}

<録画済み情報>={
  id:I=情報ID
  recFilePath:S=録画ファイルパス
  title:S=番組名
  startTime:T=開始時間
  durationSecond:I=録画時間
  serviceName:S=サービス名
  onid:I=ネットワークID
  tsid:I=TSID
  sid:I=サービスID
  eid:I=イベントID
  drops:I=ドロップ数
  scrambles:I=スクランブル数
  recStatus:I=録画結果のステータス
  startTimeEpg:T=予約時の開始時間
  comment:S=コメント
  programInfo:S=.program.txtファイルの内容
  errInfo:S=.errファイルの内容
  protectFlag:B=プロテクト
}

<チューナ予約情報>={
  tunerID:I=チューナID
  tunerName:S=BonDriverファイル名
  reserveList:予約ID:Iのリスト=そのチューナに割り当てられた予約
}

<録画プリセット>={
  id:I=プリセットID
  name:S=プリセット名
  recSetting:<録画設定>=プリセット録画設定
}

<自動予約登録情報>={
  dataID:I=登録ID
  addCount:I=予約追加カウント(参考程度)。登録時はnilで良い
  searchInfo:<自動予約検索条件>=検索条件
  recSetting:<録画設定>=録画設定
}

<自動予約検索条件>={
  ### 以下のプロパティはEnumAutoAddなどの取得系メソッドでnilになることはない
  andKey:S=キーワード
  notKey:S=NOTキーワード
  regExpFlag:B|nil=正規表現モード(省略時false)
  titleOnlyFlag:B|nil=番組名のみ検索対象にする(省略時false)
  aimaiFlag:B|nil=あいまい検索モード(省略時false)
  notContetFlag:B|nil=対象ジャンルNOT扱い(省略時false)
  notDateFlag:B|nil=対象期間NOT扱い(省略時false)
  freeCAFlag:I|nil=スクランブル放送(0=限定なし(省略時),1=無料のみ,2=有料のみ)
  chkRecEnd:B|nil=録画済かのチェックあり(省略時false)
  chkRecDay:I|nil=録画済かのチェック対象期間(省略時0)
  chkDurationMin:I|nil=番組最小長(分)(省略時0)
  chkDurationMax:I|nil=番組最大長(分)(省略時0)
  contentList:{
    content_nibble:I=大分類*256+中分類
  }のリスト=対象ジャンル
  dateList:{
    startDayOfWeek:I=検索開始曜日(0=日,1=月...)
    startHour:I=開始時
    startMin:I=開始分
    endDayOfWeek:I=検索終了曜日(0=日,1=月...)
    endHour:I=終了時
    endMin:I=終了分
  }のリスト|nil=対象期間(省略時空リスト)
  serviceList:{
    onid:I=ネットワークID
    tsid:I=TSID
    sid:I=サービスID
  }のリスト|nil=対象サービス(省略時空リスト)
  ### 以下のプロパティは取得系メソッドで常にnil
  network:I|nil=対象ネットワーク(+1=地デジ,+2=BS,+4=CS,+8=その他。該当サービスがserviceListに追加される)
  days:I|nil=対象期間(現時刻からdays*24時間以内。SearchEpg以外では無視される)
  days29:I|nil=対象期間(現時刻からdays*29時間以内。互換用なので使用しないこと。SearchEpg以外では無視される)
}

<自動予約(プログラム)登録情報>={
  dataID:I=登録ID
  dayOfWeekFlag:I=対象曜日(+1=日,+2=月...)
  startTime:I=録画開始時間(00:00を0として秒単位)
  durationSecond:I=録画総時間
  title:S=番組名
  stationName:S=サービス名
  onid:I=ネットワークID
  tsid:I=TSID
  sid:I=サービスID
  recSetting:<録画設定>=録画設定
}

<ファイル情報>={
  id:I=このファイルがリストされた順番
  name:S=ファイル名
  size:I=ファイルサイズ
}
