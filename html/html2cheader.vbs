' html2cheader.vbs

Option Explicit

' コマンドライン引数の処理
Dim args, fso, inputFile, outputFile
Set args = WScript.Arguments
Set fso = CreateObject("Scripting.FileSystemObject")

If args.Count < 2 Then
    WScript.Echo "Usage: cscript html2cheader.vbs -i input.html -o output.h"
    WScript.Quit 1
End If

If LCase(args(0)) <> "-i" Or LCase(args(2)) <> "-o" Then
    WScript.Echo "Invalid arguments. Use -i for input and -o for output."
    WScript.Quit 1
End If

inputFile = args(1)
outputFile = args(3)

If Not fso.FileExists(inputFile) Then
    WScript.Echo "Input file does not exist: " & inputFile
    WScript.Quit 1
End If

' HTMLからCヘッダーへの変換処理
Call Html2CHeader(inputFile, outputFile)

Sub Html2CHeader(inputPath, outputPath)
    Dim inputStream, line, lines, basename, content, outputStream
    Set inputStream = CreateObject("ADODB.Stream")
    
    ' 入力ファイルをUTF-8で読み込み、改行コードをLFに設定
    inputStream.Type = 2 ' テキストモード
    inputStream.Charset = "utf-8"
    inputStream.LineSeparator = 10 ' LF (\n) を改行コードとして設定
    inputStream.Open
    inputStream.LoadFromFile inputPath
    lines = Array()
    Do Until inputStream.EOS
        line = inputStream.ReadText(-2) ' 1行読み込み
        If Not IsNull(line) Then
            line = """" & Replace(Trim(line), """", "\""") & "\n"""
            ReDim Preserve lines(UBound(lines) + 1)
            lines(UBound(lines)) = line
        End If
    Loop
    inputStream.Close
    
    ' ファイル名からベース名を取得
    basename = fso.GetBaseName(inputPath) & "_" & Replace(fso.GetExtensionName(inputPath), ".", "_")
    
    ' 出力用の文字列を構築
    content = "#pragma once" & vbCrLf & _
              "String " & basename & " = " & vbCrLf & _
              Join(lines, vbCrLf) & ";"
    
    ' BOMなしUTF-8で書き込み
    Set outputStream = CreateObject("ADODB.Stream")
    outputStream.Type = 2 ' テキストモード
    outputStream.Charset = "utf-8"
    outputStream.Open
    outputStream.WriteText content
    
    ' BOMを除去してバイナリ保存
    outputStream.Position = 0
    outputStream.Type = 1 ' バイナリモード
    outputStream.Position = 3 ' BOM（3バイト）をスキップ
    Dim binaryData
    binaryData = outputStream.Read ' 残りのデータを読み込み
    outputStream.Close
    
    ' BOMなしでファイルに保存
    Set outputStream = CreateObject("ADODB.Stream")
    outputStream.Type = 1 ' バイナリモード
    outputStream.Open
    outputStream.Write binaryData
    outputStream.SaveToFile outputPath, 2 ' 2 = 上書き
    outputStream.Close
End Sub

WScript.Quit 0