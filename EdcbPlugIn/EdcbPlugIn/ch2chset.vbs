' ch2chset.vbs: �����ŗ^����ꂽTVTest�`�����l���ݒ�t�@�C��(ch2)����EDCB�`�����l�����t�@�C��(ChSet4.txt,ChSet5.txt)���쐬����

' �l�b�g���[�N���̍��ڂ��T�[�r�X���Ɠ����ɂ��邩�ǂ���
setNetworkName = True
' ������M(�����Z�O)�T�[�r�X���o�͂��邩�ǂ���
savePartial = False

Set objFSO = CreateObject("Scripting.FileSystemObject")
objCh5 = Null
strCh5IdList = ""

For i = 0 To WScript.Arguments.Count - 1
  strPath = WScript.Arguments(i)
  ' �g���q��.ch2�ł����
  If LCase(Right(strPath, 4)) = ".ch2" Then
    Set objIn = objFSO.OpenTextFile(strPath, 1)
    ' "(�`).ChSet4.txt"�́`�ɂ͖{���`���[�i�������邪�Ach2�ɂ��̏��͂Ȃ��̂ŋ󊇌ʂɂ���
    ' EpgTimerSrv.exe�������������Ȃ��Ȃ�̂Ŋ��ʂ͕K�v
    ' EpgDataCap_Bon.exe���ǂݍ��߂�悤�ɂ���ɂ̓`���[�i�����K�v
    Set objOut = objFSO.OpenTextFile(Left(strPath, Len(strPath) - 4) & "().ChSet4.txt", 2, True)
    If IsNull(objCh5) Then
      Set objCh5 = objFSO.OpenTextFile(Left(strPath, InStrRev(strPath, "\")) & "ChSet5.txt", 2, True)
    End If

    Do Until objIn.AtEndOfStream
      ' ch2��9���ڂ̃J���}��؂�
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
          ' ���ڂ���т����ă^�u��؂��ChSet4.txt�ɒǉ�
          ' ChSet4�̃`�����l�����ƃl�b�g���[�N���͗��p����Ȃ�����"!"��OK
          ' �y�`�����l����(ch2�ɏ��Ȃ�)|�T�[�r�X��|�l�b�g���[�N��(ch2�ɏ��Ȃ�)|�`���[�i���|�����`�����l��|ONID|TSID|SID|�T�[�r�X�^�C�v|������M��|�ꗗ�\���Ɏg�p���邩|�����R��ID�z
          aryOut = Array("!", Trim(aryIn(0)), "!", Trim(aryIn(1)), Trim(aryIn(2)), Trim(aryIn(6)), Trim(aryIn(7)), Trim(aryIn(5)), strServiceType, strPartial, "1", Trim(aryIn(3)))
          objOut.WriteLine(Join(aryOut, vbTab))
          strId = Trim(aryIn(6)) & "-" & Trim(aryIn(7)) & "-" & Trim(aryIn(5)) & ","
          If InStr(strCh5IdList, strId) = 0 Then
            ' �܂��ǉ����Ă��Ȃ��T�[�r�X�Ȃ̂�ChSet5.txt�ɂ��ǉ�
            ' �y�T�[�r�X��|�l�b�g���[�N��|ONID|TSID|SID|�T�[�r�X�^�C�v|������M��|EPG�f�[�^�擾�Ώۂ�|�f�t�H���g�����Ώۂ��z
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
