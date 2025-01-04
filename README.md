# mysh

独自開発のシェルスクリプト環境(Ubuntu版)

ここ十年近くWindows環境でばかり開発をしてきたので、最近のLinuxでの開発環境を知るべく、これに取り組んでみました。

## ビルド方法

#### ファイル構成
```
mysh/
├── .vscode
|   ├── launch.json
|   └── settings.json
├── CMakeLists.txt
├── README.md
└── src
    ├── mysh.cpp
    ├── mysh.h
    ├── operators.cpp
    └── token.cpp
```
#### ターミナルでビルドする場合
1. ターミナルでmysh/buildに移動
2. `cmake ..`
3. `make`

#### VSCodeでビルドする場合
1. VSCodeのOpen Folderでmyshを開いてロードする
2. VSCode画面下のステータスバーにあるBuildボタンを押す

## 起動

* myshを起動すると、すぐにコマンド入力モードに移行するが画面には何も表示しない。
* 下記のようにコマンドを入力して改行すると
  ```
  (hello world\n) print ↵
  hello world
  ```
* インタラクティブに操作する場合は、最初に`prompt`コマンドを実行して、プロンプト`msh> `を表示するのが良い
  * インタラクティブモードではエラーが発生してもエラーを表示するだけでmyshは終了しない
  ```
  prompt
  msh> 
  msh> abc
  ##[undefined - abc]##
  0| /abc
  ```
  * 通常モードではコマンドエラー時にエラーを表示してmysh終了

## 言語仕様

> Adobeが開発したPostScriptの言語仕様にほぼ準拠

### 基本

* オペランドスタックにデータ[オブジェクト](#オブジェクトの種類)を載せて、コマンド[オペレータ](#オペレータの種類)を実行する形式
* データ[オブジェクト](#オブジェクトの種類)には数値、名前、文字列、配列、手続き、ファイル、辞書がある
* 変数の管理は辞書で行う
* 辞書は複数作成でき、同じ変数名を複数辞書に違う値で登録することを許す
* 辞書の検索順は辞書オブジェクトを載せる辞書スタックの順番に依存する

#### トークンのセパレータ

* トークンは以下の文字をセパレータにして切り出す
 `{` `}` `[` `]` `(` `)` `<` `>` `\n` `\t` `/` `%` `;` `\\`

* `%`または`;`以降の文字はコメントと見なす

#### 足し算
* 足し算１＋２を実行する場合
  * １と２を順番にオペランドスタックに積んだあとにaddというオペレータを実行するようにスクリプトを作成する。
  `1 2 add`
  * これを実行するとオペレータ`add`は必要なパラメータ`1`と`2`をスタックから取得して計算結果`3`をオペランドスタックに積む。
  * スタックに積まれた`3`は必要に応じて`==`や`pstack`などの指示で画面表示
   ```
   msh> 1 2 add
   msh> ==
   3
   ```

### 変数と変数値
* Key=変数、Value=変数値という考え方で、辞書に登録する
* Key-Valueペアの登録は`/key value def`
  * Keyは任意の文字列
  * すべての[オブジェクト](#オブジェクトの種類)がValueになり得る
  * `def` は Key-Valueペアを登録するオペレータ
  ```
  /param#1 10 def                                            % 整数を代入
  /param#2 3.14 def                                          % 実数を代入
  /param#3 (sample string) def                               % 文字列を代入
  /param#4 {param#1 param#2 add} def                         % 手続き(実行配列)を代入
  /param#5 [1 2 param#2 (a) (b) (cdef) {(OK\n) print}] def   % 配列を代入
  /param#6 10 dict def                                       % 辞書を代入
  /param#7 (/temp/sample.txt) readfile def                   % ファイルを代入
  ```
* `/`を付けずに変数名を入力するとその値がスタックに載る
  ```
  msh> /param#1 10 def       % param#1というkeyに10を代入
  msh> param#1               % param#1を呼び出すと10がスタックに積まれる
  mah> ==                    % == はオペランドスタック最上位の値を表示してpopするコマンド
  10
  msh> /param#1 -9 def
  msh> param#1 ==
  -9
  ```
* 手続き(実行配列)型`{}`のオブジェクトは、スタックに載る瞬間にその手続きの内容を実行
  ```
  msh> /param#1 10 def
  msh> /param#2 3.14 def
  msh> /param#4 {param#1 param#2 mul} def   % param#4に手続きを代入
  msh> param#4   % param#4を呼び出すと手続きがスタックに積まれる同時に実行。結果はスタックに。
  mash ==        % == でオペランドスタック最上位の値を表示してpop
  31.4
  ```
### 辞書

* Key-Valueペアの登録先
* 複数の辞書をハンドリングする機能を持つ
  * [辞書スタック](#辞書スタック)
  * 辞書をValueとして登録可

#### 辞書スタック

* 複数辞書を辞書スタックに積み、スタックの順番で検索順を決めている。最下層にはmyshの定義を格納した[システム辞書](#システム辞書)が位置している。
* `dict`で作成した辞書オブジェクトに対して`begin`する事で、その辞書が辞書スタックに積まれる。
* `end`によって辞書スタック最上位の辞書をpopする。これによってpopされた辞書は検索スコープから外れる。

#### システム辞書

* myshがデフォルトで持っているコマンドや手続きを格納している辞書で`end`できない

#### ユーザ定義辞書

* `dict`で生成し，`begin`で辞書スタックに載せ、`end`で辞書スタックから外す
* 検索は辞書スタックの上から下に向けて行い、最初に見つかった値を採用する
  * 例えば辞書スタック最上位の辞書にKey=Param, Value=0の登録があり、その下の辞書にKey=Param, Value=100の登録がある場合、Paramの検索結果は0になる。
  * 以下はそのサンプル
  ```
  %% デフォルトで10個のKey-Valueペアを格納できる辞書を2つ作成
  10 dict /MyDict#1 exch def  %% exch は先頭スタック入れ替えるオペレータ
  10 dict /MyDict#2 exch def 

  %% MyDict#1を辞書スタックに載せる
  MyDict#1 begin
    /year 2025 def
    /month (jan) def
    %% この時点で year = 2025, month = (jan) を記憶
    %% 記憶情報を表示
    (**MyDict\n) print        %% print は文字列を表示するオペレータ
    year cvs print (\n) print %% cvs は数値を文字列に変換するオペレータ
    month print (\n) print

      %% MyDict#2を辞書スタックに載せる
      MyDict#2 begin
        /year 3000 def
        /month (dec) def
        %% この時点で year = 3000, month = (dec) を記憶
        %% 記憶情報を表示
        (**MyDict\n) print
        year cvs print (\n) print
        month print (\n) print
      end %% MyDict#2を辞書スタックから外す

    %% この時点で year = 2025, month = (jan) を思い出す
    %% 記憶情報を表示
    (**MyDict\n) print
    year cvs print (\n) print
    month print (\n) print

  end %% MyDict#1を辞書スタックから外す

  %% MyDict#2を辞書スタックに載せる
  MyDict#2 begin
    /year 3000 def
    /month (dec) def
    %% この時点で year = 3000, month = (dec) を記憶
    %% 記憶情報を表示
    (**MyDict\n) print
    year cvs print (\n) print
    month print (\n) print
  end %% MyDict#2を辞書スタックから外す  

  %% この時点で year, monthが未定義に
  %% 記憶情報を表示
  (**\n) print
  year cvs print (\n) print
  month print (\n) print
  ```
  上記スクリプトを実行すると、printコマンドによって下記の出力が得られる。

  ```
  **MyDict         <== MyDict#1が辞書スタックの最上位
  2025
  jan
  **MyDict         <== MyDict#2をbeginした事で最上位に
  3000
  dec
  **MyDict         <== MyDict#2をendした事でMyDict#1が最上位に
  2025
  jan
  **MyDict         <== MyDict#2をbeginした事でMyDict#2が最上位に
  3000
  dec
  **               <== MyDict#2をendした事でシステム辞書が最上位に
   ##[undefined - year]##   <== システム辞書にはyear変数の登録はないので未定義エラー
    0| /year                <== エラーが発生したらオペランドスタックのリストを表示する仕様
  ```

### ファイル

* 外部ファイルデータの取り込み、標準入力からのデータ取り込みの際に、ファイル名などを指定して`file`オブジェクトを作成し、それに対して`read`指示を行う。
* readlineやファイル書き込み機能を追加する予定あり[TBD]

#### ファイル読み込み

* 読み込み対象のファイルのファイルパスを`readfile`に渡して`--file--`オブジェクトを取得
* `readfile`によってファイルは開いた状態になるので、不要になったら`closefile`で開放する
  ```
  (sample.txt) readfile/file exch def     %% カレントフォルダにある sample.txt を開く
  1 string/buffer exch def                %% 文字読み込み用バッファを確保
  {                                       %% ファイル終端までのloop
	  file read                             %% ファイルから1文字読み込む(終端に達したらfalseを返す)
	  {buffer 0 3 -1 roll put buffer print} %% 取得した文字コードをバッファに格納
	  {(\nEOF\n) print exit}ifelse          %% readがfalseを返したらファイル終端
  } loop
  file closefile                          %% ファイルを閉じて開放
  ```

#### キー入力読み込み

* `null`を`readfile`に渡して、標準入力用の`file`オブジェクトを生成
  * 標準入力用の`file`は`closefile`を実行する必要なし
* `file`が標準入力の場合、`file read`でキー入力を1文字ずつ検出
  ```
  %% キー入力検出した後のアクションを定義
  %% 下記では単純に文字列を画面表示しているのみ
  /finish {(\nfinished\n) print exit} def
  /forward {(forward\n) print } def
  /backward {(backward\n) print } def
  /stop {(stop\n) print } def
  /left {(left\n) print } def
  /right {(right\n) print } def

  null readfile/file exch def %% null readfile によって標準入力をファイルオブジェクトに関連付け
  1 string/buffer exch def
  {
    %% 標準入力からのキー入力を検出して文字コードをスタックに載せる
    %% readは文字入力を正常に検出したら、その「文字コード」と「true」をスタックにpushする
    %% 失敗したら「false」をスタックにpushする
    file read
    {
      buffer 0 3 -1 roll put 
      %% 下記で押されたキーに応じた動作をコール
      %% ifelseを使って記述すると処理が読みにくくなるので、ここでは敢えてifだけを使用
      %% この場合、毎回全てのキーとの一致を試みる事になる。
      buffer (q) eq {finish} if     %% q が押されたらループを抜ける
      buffer (w) eq {forward} if    %% w が押されたら 定義したforward手続きをコール
      buffer (s) eq {stop} if       %% s が押されたら 定義したstop手続きをコール
      buffer (x) eq {backward} if   %% x が押されたら 定義したbackwward手続きをコール
      buffer (a) eq {left} if       %% a が押されたら 定義したleft手続きをコール
      buffer (d) eq {right} if      %% d が押されたら 定義したright手続きをコール
    }
    {exit}ifelse 
  } loop
  ```
### 外部コマンド(アプリ)との連携
* 外部コマンドとの連携には、外部コマンドを呼び出す`async`と、実行した外部コマンドの終了を確認する`finished`を使用
* 下記は[車輪ロボットを動かす3](https://github.com/shimooku/ros2_cpp_ws?tab=readme-ov-file#車輪ロボットを動かす3) で作成したロボットを`async`と`finished`で制御するスクリプト
  ```
  %% 外部向けコマンドをモディファイしやすいように複数に分割して配列に入れる
  %% 3番目の要素 (0.0) と5番目の要素 (0.0)を他の値に書き換えて、ロボットを前進、後退、左右回転させるようという目論見
  /command
  [
    (ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist )  %% 固定にする
    ('{linear: {x: )                                           %% 固定にする
    (0.0)                                                      %% 入れ替える
    (, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: )         %% 固定にする
    (0.0)                                                      %% 入れ替える
    (}}')                                                      %% 固定にする
  ]def

  %% 外部コマンドを呼び出し、レスポンスが来るまで待つコマンドを作成
  /exec_command
  {
    async/pid exch def                          %% asyncでコマンド実行
    {pid finished {exit}{2 sleep} ifelse }loop  %% finished で終了チェック, sleepでチェック頻度を調整
  } def

  %% キー入力検出した後のアクションを定義
  /finish {(\nfinished\n) print exit} def
  /forward { command 2 (1.0) put command exec_command} def
  /backward {command 2 (-1.0) put command exec_command} def
  /stop {command 2 (0.0) put command 4 (0.0) put command exec_command} def
  /left {command 4 (1.0) put command exec_command} def
  /right {command 4 (-1.0) put command exec_command} def
  
  null readfile/file exch def %% null readfile によって標準入力をファイルオブジェクトに関連付け
  1 string/buffer exch def    %% 1文字コードを格納するバッファを準備

  %% キー入力を検出し、文字コードによって動作を振り分け実行するループ
  {
    file read  %% キー入力を検出し、成功すれば文字コードとtrueを、失敗すればfalseをスタックに載せる
    {
      buffer 0 3 -1 roll put %% 文字コードをバッファに格納
  
      %% 下記で押されたキーに応じた動作をコール
      %% ifelseを使って記述すると処理がわかりにくなるので、ここでは敢えてifだけ使用
      buffer (q) eq {finish} if     %% q が押されたらループを抜ける
      buffer (w) eq {forward} if    %% w が押されたら 定義したforward手続きをコール
      buffer (s) eq {stop} if       %% s が押されたら 定義したstop手続きをコール
      buffer (x) eq {backward} if   %% x が押されたら 定義したbackwward手続きをコール
      buffer (a) eq {left} if       %% a が押されたら 定義したleft手続きをコール
      buffer (d) eq {right} if      %% d が押されたら 定義したright手続きをコール
    }
    {exit}ifelse 
  } loop
  ```

## オブジェクトの種類

|オブジェクト|説明|
|---|---|
|Boolean|true, false|
|整数|-2, -1, 0, 1, 2, ...|
|浮動小数|-1.0, -20., 0.0, 1.0, 2.0, ...|
|配列開始マーク|オペランドスタックに配列要素を積む直前の配列開始マーク`[`|
|[リテラル名](#リテラル配列と実行配列手続き)|/ABC, /abc, /Abc, ....|
|[検索名](# リテラル名と検索名)|ABC, abc, Abc, ....|
|文字列|(ABC), (abc), (Abc), ...|
|[リテラル配列](# リテラル配列と実行配列)|[1 2 3], [(a) (b) (cd)], [(abc) [10 0.2] /cc], ...|
|[実行配列](# リテラル配列と実行配列)|{1 2 3}, {(a) (b) (cd)}, {(abc) [10 0.2] /cc}, ...|
|空オブジェクト|`null`でスタックに積む。表示は`--nulll--`。|
|辞書|Key-Valueのペアの集合。複数作成可。辞書をValueにする事も可。|
|ファイル|`readfile`で開いたファイルを識別するオブジェクト|

### リテラル名と検索名

* リテラル名はスタック上にリテラル名オブジェクトとして積まれる一方で，検索名は辞書検索結果がスタック上に積まれる。
* 例えば /ABC (abc) def の登録がある場合，/ABC ⇨ /ABC となり ABC ⇨ (abc) となる。
* 検索名による検索結果が実行配列の場合，スタックに積まれた実行配列は即実行される。

### リテラル配列と実行配列(手続き)

#### リテラル配列を作成する操作
* `[ 1 2 add]` の入力結果は `[ 3 ]` となり，`[`と`]`の間は直ぐに実行
#### 実行配列(手続き)を作成する操作
* `{ 1 2 add }` の入力結果は `{ 1 2 add }` となり，`{`と`}`の間は実行されずそのまま記録
* 実行配列は，検索結果のValueとしてスタックに積まれる即実行

## オペレータの種類

### ヘルプ
|オペレータ|説明|パラメータ|結果|
|--:|:-|:--|:--|
|**help**|コマンド仕様表示|コマンドのリテラル名 /dup **help**|標準出力にコマンド仕様を表示|

### 演算
|オペレータ|説明|パラメータ|結果|
|--:|:-|--:|:--|
|`add`|加算|1 2 **add** *3*|3|
|`sub`|減算|5 2 `sub`|3|
|`mul`|積算|5 2 `mul`|10|
|`div`|除算|5 2 `div`|2|
|||5.0 2 `div`|2.5|
|`mod`|余り|5 2 `mod`|1|
|`ceil`|切り上げ|2.4 `ceil`|3.0|
|`round`|四捨五入|2.4 `round`|2.0|
|`tuncate`|切り捨て|2.4 `truncate`|2.0|
|`abs`|絶対値|-1. `abs`|1.|
|||-1 `abs`|1|
|`and`|AND算|3 5 `and`|1|
|`or`|OR算|3 5 `or`|7|
|`xor`|XOR算|3 5 `xor`|6|


### 比較
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`eq`|等しければtrueを積む|(abc) (abc) `eq`| true |
|||(abc) (ABC) `eq`| false |
|||3 3 `eq`| true |
|`ne`|等しくなければtrueを積む|(abc) (abc) `ne`| false |
|||(abc) (ABC) `ne`| true |
|`ge`|Greater equal|2 1 `ge`|true|
|||2 2 `ge`|true|
|`gt`|Greater than|2 1 `gt`|true |
|||2 2 `gt`|false |
|`le`|Less equal|3 3 `le`|true|
|||3 4 `le`|true|
|`lt`|Less than|3 3 `lt`|false|
|||3 4 `lt`|true|

### スタック操作
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`pop`|先頭スタックを捨てる|3 4 `pop`|3|
|`exch`|先頭スタック入れ替え|3 5 `exch`| 5 3 |
|`dup`|先頭スタック複製|3 `dup`| 3 3 |
|||(abc) `dup`| (abc) (abc) |
|`roll`| .... *--num1--* *--num2--* `roll` <br>スタックの*num1* 個の要素を*num2* 回ロールする | 10 20 30 40 4 1 `roll` |40 10 20 30|
|||10 20 30 40 4 -1 `roll`|20 30 40 10|
|`clear`|スタックの全クリア|10 20 30 40 `clear`|- |
|`pstack`|全スタックの表示|10 20 30 `pstack`|10<br>20<br>30|
|`print`|先頭スタック文字列の表示|(abc) `print`|標準出力に abc|
|`==`|先頭スタックの表示|(abc) `==`|標準出力に (abc) |

### オブジェクト型変換
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`cvi`|整数に変換|2.0 `cvi`| 2 |
|`cvr`|実数に変換|2 `cvr`|2.0|
|`cvs`|文字列に変換|2 `cvs`|(2)|
|`cvx`|実行型に変換|[ 1 2 3 ] `cvx`|{1 2 3}|
|||( 1 2 3 ) `cvx`|{1 2 3}|
|`cvn`|名前に変換|(abc) `cvn`|/abc|


### 配列操作
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`array`|指定長さの配列生成|3 `array`| [ *--null--* *--null--* *--null--* ] |
|`aload`|配列展開|[ 0 1 2 ] `aload`| 0 1 2 [ 0 1 2 ] |
|`copy`|配列要素コピー|[ (a) (b) ] [1 2 3 ] `copy`|[ (a) (b) 3 ]|
|`[`|配列開始|[ (a) (b) `]`|[ (a) (b) ]|
|`mark`|配列開始|[ (a) (b) `]`|[ (a) (b) ]|
|`]`|配列生成|[ (a) (b) `]`|[ (a) (b) ]|
|`closearray`|配列生成|[ (a) (b) `]`|[ (a) (b) ]|
|`}`|実行配列生成|{ (a) (b) `}`|{ (a) (b) }|
|`put`|配列に要素を挿入|3 array dup 0 (abc) `put`| [ (abc) *--null--* *--null--* ] |
|`putinterval`|配列Aに配列Bの要素を挿入|/ar [1 2 3 4] def ar 2 [98 99] `putinterval` ar| [1 2 98 99] |
|`get`|配列から要素を取得|[ 1 2 3 ] 1 `get`| 2 |
|`getinterval`|配列から要素を配列として取得|[1 2 3 4] 2 3 `getinterval`| [3 4] |
|`length`|要素数取得|[ 1 2 3 ] `length`| 3 |
|`maxlength`|容量取得|10 array `maxlength`| 10 |
|`forall`|全配列要素に対して*--proc--*を実行|[ 1 2 3 ] { 5 add == } `forall`| 6<br>7<br>8 |

### 辞書
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`begin`|指定辞書を辞書スタック先頭に載せてカレント辞書にする|`10 dict begin`|*--dict--*|
|`def`|辞書登録|`/Name (value) def`|カレント辞書にNameという名前で文字列(value)を登録|
|`end`|カレント辞書を閉じる|`end`|辞書スタック先頭の辞書をPOPしてその下の辞書をカレント辞書にする|
|`dict`|デフォルトで指定した数のKey-Valueペアを格納できる辞書を作成する。その数を超える事は出来るがreallocが走るので少し遅くなる。|10 `dict`|*--dict--*|
|`currentdict`|辞書スタックの先頭にある辞書をスタックに載せる|`currentdict`|*--dict--* |
|`load`|カレント辞書に登録されているValueをスタックに載せる|/Name (value) def /Name `load`|(value) |
|`put`||3 dict dup /ABC (abc) `put` begin /ABC load|(abc)|
|`get`||3 dict begin /ABC (abc) def currentdict /ABC `get`| (abc) |
|`length`|登録アイテム数取得|3 dict `length`| 0 |
|||3 dict /ABC (abc) def currentdict `length`| 1 |
|`maxlength`|容量取得|10 dict `maxlength`| 10 |
|`forall`|全辞書エントリーに対して*--proc--*を実行|`10 dict begin /A (a) def /B (b) def /C (c) def currentdict { } forall `end| /A (a) /B (b) /C (c) |

### 文字列
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`search`|文字列の部分検索|(abcd) (bc) `search`|(d) (bc) (a) true|
|||(abcd) (ab) `search`|(cd) (ab) () true|
|||(abcd) (cd) `search`| () (cd) (ab) true                                            |
|||(abcd) (efg) `search`|(abcd) false|
|`concat`|結合文字列生成|(abc) (def) `concat`| (abcdef) |
|`put`||(0123456789) dup 3 97 `put`|(012a456789)|
|`putinterval`||/ar (1234) def ar 2 (xy) `putinterval` ar| (12xy) |
|`get`||(abcdefg) 0 `get`| 97 |
|`length`|サイズ取得|(abcdef) `length`| 6 |
|`getinterval`|*--string--* *--index--* *--count--* `getinterval`<br> *string*の*index*番目から*count*数分の文字を切り出して文字列作成|(abcdef) 2 3 `getinterval`| (cde) |
|`forall`|文字列中の文字に対して*--proc--*を実行|(abcdef) { } `forall`| 97 98 99 100 101 102 |

### 実行
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`exec`|強制実行|{1 2 add} `exec`| 3 |
|`execfile`|ファイル読み込み実行|(test.tos) `execfile`| (test.tos) を実行 |
|`run`|ファイル読み込み実行|(test.tos) `run`| (test.tos)を実行 |

### ループ
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`for`|*--initvalue--* *--step--* *--limit--* { } for|1 1 3 { } `for`|1 2 3|
|`forall`|配列，辞書，文字列のforallを参照|||
|`loop`| *--proc--* をexitするまで実行 | { } `loop` | exitコマンドを実行するまで永遠にproc実行を繰り返す |
|`repeat`| *--count--* { } repeat <br>*count*数分*--proc--*を実行する | 3 { } `repeat` | 3回proc実行を繰り返す |
|`exit`|for, forall, loop, repeatの繰り返しから抜ける|||

### 分岐
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`if`|IF分岐|true { (ABC) } `if`|(ABC)|
|`ifelse`|IF-ELSE分岐|true { (ABC) } { (DEF) } `ifelse`|(ABC)|

### 計測
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`tickcount`|msecオーダのTick Count|- `tickcount`| *--num--* as time in msec                                    |

### 日時
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`time`||- `time`|*--string--* like (2021/12/20@12:00:00)|

### ファイル
|オペレータ|説明|使用例|結果|
|--:|:-|:--|:--|
|readfile|読み込みファイルを開く|(filename) **readfile** *--file--*<br/>null **readfile** *--file--*|ファイル入力<br/>標準入力|

		{"read", __opr_read, "<file> read <int> <true> if not end-of-file\n<file> read <false> if end-of-file", ""},
		{"closefile", __opr_closefile, "--file-- closefile -", ""},

### メモリチェック
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`enummem`|使用中のメモリーブロック表示|- `enummem`|使用中のメモリーブロック表示|
|`checkmem`|メモリー破壊箇所表示|- `checkmem`|メモリー破壊箇所表示|

### その他コマンド
|オペレータ|説明|使い方|出力|
|--:|:-|--|:--|
|`prompt`|プロンプト表示|mysh起動後に`prompt`と入力して改行すると下記のようにプロンプトを表示<br/>`mash> `|なし。<br/>入力を促すのみ。 |
|`cd`|カレントパス変更|(/temp) `cd`| /tempに移動 |
|`pwd`|カレントパスを文字列にしてスタックに載せる|- `pwd`| (/temp) |
|`enumfiles`|filter検索されたファイル名を順次スタックに載せて*--proc--* を実行|(*) { } `enumfiles`|  |
|`getenv`|環境変数値の取得|(HOME) `getenv` |<string> |
|`messagebox`|メッセージボックスを表示してYES/NOの選択を促す|(TITLE) (YES to continue, NO to stop) `messagebox` | --true/false-- |
|`async`|外部プロセスを実行(非同期)| `[ (ros2 pram set /speed_calc_node wheel_radius) (0.5) ] async`<br/> 起動時コマンドを配列に入れて指定|integer<br/>(プロセス管理番号)|
|`finished`|`async`で取得したプロセス管理番号に対して`finished`を実行して、完了か動作中かを確認 | |終了していれば`true`、実行中なら`false` |
|`sleep`|処理を指定秒停止 |10秒停止 `10 sleep`|なし|
|`quit`|TOS終了|`quit`|なし|




