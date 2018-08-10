' ch2chset.vbs: 引数で与えられたTVTestチャンネル設定ファイル(ch2)からEDCBチャンネル情報ファイル(ChSet4.txt,ChSet5.txt)を作成する

' ネットワーク名の項目をサービス名と同じにするかどうか
setNetworkName = True
' 部分受信(ワンセグ)サービスを出力するかどうか
savePartial = False

Set objFSO = CreateObject("Scripting.FileSystemObject")
objCh5 = Null
strCh5IdList = ""

For i = 0 To WScript.Arguments.Count - 1
  strPath = WScript.Arguments(i)
  ' 拡張子が.ch2であれば
  If LCase(Right(strPath, 4)) = ".ch2" Then
    Set objIn = objFSO.OpenTextFile(strPath, 1)
    ' "(～).ChSet4.txt"の～には本来チューナ名が入るが、ch2にその情報はないので空括弧にする
    ' EpgTimerSrv.exeが正しく扱えなくなるので括弧は必要
    ' EpgDataCap_Bon.exeが読み込めるようにするにはチューナ名も必要
    Set objOut = objFSO.OpenTextFile(Left(strPath, Len(strPath) - 4) & "().ChSet4.txt", 2, True)
    If IsNull(objCh5) Then
      Set objCh5 = objFSO.OpenTextFile(Left(strPath, InStrRev(strPath, "\")) & "ChSet5.txt", 2, True)
    End If

    Do Until objIn.AtEndOfStream
      ' ch2は9項目のカンマ区切り
      aryIn = Split(objIn.ReadLine, ",")
      If UBound(aryIn) = 8 And Left(Trim(aryIn(0)), 1) <> ";" Then
        strNetwork = "!"
        If setNetworkName Then
          strNetwork = Trim(aryIn(0))
        End If
        strServiceType = Trim(aryIn(4))
        If strServiceType = "" Then
          strServiceType = "0"
        End If
        strPartial = "0"
        If strServiceType = "192" Then
          strPartial = "1"
        End If
        If strPartial = "0" Or savePartial Then
          ' 項目を並びかえてタブ区切りでChSet4.txtに追加
          ' ChSet4のチャンネル名とネットワーク名は利用されないため"!"でOK
          ' 【チャンネル名(ch2に情報なし)|サービス名|ネットワーク名(ch2に情報なし)|チューナ空間|物理チャンネル|ONID|TSID|SID|サービスタイプ|部分受信か|一覧表示に使用するか|リモコンID】
          aryOut = Array("!", Trim(aryIn(0)), "!", Trim(aryIn(1)), Trim(aryIn(2)), Trim(aryIn(6)), Trim(aryIn(7)), Trim(aryIn(5)), strServiceType, strPartial, "1", Trim(aryIn(3)))
          objOut.WriteLine(Join(aryOut, vbTab))
          strId = Trim(aryIn(6)) & "-" & Trim(aryIn(7)) & "-" & Trim(aryIn(5)) & ","
          If InStr(strCh5IdList, strId) = 0 Then
            ' まだ追加していないサービスなのでChSet5.txtにも追加
            ' 【サービス名|ネットワーク名|ONID|TSID|SID|サービスタイプ|部分受信か|EPGデータ取得対象か|デフォルト検索対象か】
            aryCh5 = Array(Trim(aryIn(0)), strNetwork, Trim(aryIn(6)), Trim(aryIn(7)), Trim(aryIn(5)), strServiceType, strPartial, "1", "1")
            objCh5.WriteLine(Join(aryCh5, vbTab))
            strCh5IdList = strCh5IdList & strId
          End If
        End If
      End If
    Loop

    objOut.Close
    objIn.Close
  End If
Next

If Not IsNull(objCh5) Then
  objCh5.Close
End If
