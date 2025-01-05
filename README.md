# mysh

#### プログラムの目的

PostScriptのグラフィックス制御機能を除いたスクリプト機能の一部を実装したインタープリタをプロトタイプしました。これを基盤とし、ROS2の制御システムを構築しようとしています。この方向は、制御システム開発における完全に新しい法解を提示するものです。

#### 仕様

* mysh自体の仕様は[言語仕様](#言語仕様)に記載しました
* ROS2との連携サンプルスクリプトによる動作仕様は、[動作確認方法](#動作確認方法)に記載します。

#### アピールポイント（正直、言い過ぎです）

1. **コンパクトな文法によるフレキシビリティ** PostScriptの文法はコンパクトで考え方が分かりやすいため、制御ロジックの構築が短時間で完了できます。これは制御ロジックの操作性を向上させると同時に、システムのメンテナンス性を高めます。
2. **データ構造の単純化による高速動作** PostScriptのスタック・ベースのデータ構造をROS2のコミュニケーションパスに適用することで、コンパクトな制御ロジックの構築が可能になります。この結果、ロボット制御システムの高速動作を実現できます。

#### 動作確認方法

1. gitから https://github.com/shimooku/ros2_cpp_ws を取得してビルド

2. [外部コマンド(アプリ)との連携](#外部コマンド(アプリ)との連携) に記載の実行方法で、ロボットシミュレータを起動

3. myshのプロジェクトフォルダから build/mysh を起動して下記のように操作する

    ```
    prompt
    msh> (sample.txt) run
    ```

4. myshがキー入力を検知出来るようmyshを動かしているターミナルをActiveな状態にする

5. 下記のキー入力に対して所望の動作が出来ている事を確認

    | 入力キー | 所望の動作 |
    | :------: | :--------: |
    |    w     |    前進    |
    |    s     |  ストップ  |
    |    x     |   バック   |
    |    d     |   右回転   |
    |    a     |   左回転   |

> Windows 11 + VMware + Ubuntu 22.04 + ROS2-humble で動作確認済み

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

## 基本アルゴリズム

myshは入力データをスキャンして、トークンを抽出し、[トークン記述ルール](#オブジェクトの種類)に則り下記を行う。
* トークンが実行名の場合は、それをKeyにした辞書検索を行い、
  * 辞書から取得したValueの実行属性がtrueなら、Valueを実行 (オペレータ、実行配列、実行名が相当)
  * 辞書から取得したValueの実行属性がfalseなら、Valueをオペランドスタックにpush
* トークンが実行名以外の場合は、オブジェクト化してオペランドスタックにpush

つまり、実行属性がtrueのオブジェクトが実行されるタイミングは、基本的には辞書検索で見つかった時。

それだと機能性が若干損なわれる (動的に命令を生成したい) ため、文字列を実行配列に変換 (***cvx***オペレータ) し、オペランドスタックに載っている実行可能 (=実行属性がtrue) なオブジェクトを強制的に実行する ***exec***オペレータ を追加している。

例えば下記の場合 {1 2 add} を実行する事になる。
```
(1 ) (2 ) concat (add) concat cvx exec
```

## オブジェクトの種類

|オブジェクト|表記|実行<br />属性|トークン記述ルール|
|---|:-:|:-:|:-:|
|Boolean|*--bool--*|false|***true, false***|
|整数|*--int--*|false|*-2, -1, 0, 1, 2, ...*|
|浮動小数|--*real*--|false|*-1.0, -20., 0.0, 1.0, 2.0, ...*|
|配列開始マーク|--*mark*--|false|***[***|
|[リテラル名](#リテラル配列と実行配列手続き)|--*litname*--|false|名前の前に/を付けたもの  e.g. */ABC, /abc, /Abc, ....*|
|[実行名](# リテラル名と実行名)|--*name*--|**true**|セパレータ以外の文字を使った文字列 e.g. *ABC, abc, Abc, ....*|
|文字列|--*string*--|false|***( )*** で囲った文字列でセパレータ文字を含める事ができる|
|[リテラル配列](#リテラル配列と実行配列手続き) |--*array*--|false|***[ ]*** で複数のトークンを囲ったもの|
|[実行配列](#リテラル配列と実行配列手続き)|--*execarray*--|**true**|***{ }*** で複数のトークンを囲ったもの|
|空オブジェクト|--*null*--|false|***null***|
|辞書|--*dict*--|false|Key-Valueのペアの集合。複数作成可。辞書をValueにする事も可。|
|ファイル|--*file*--|false| _**readfile**_で作成するものなのでトークンは無い             |
|オペレータ||**true**|実行名と同じ|


### リテラル名と実行名

* リテラル名はスタック上にリテラル名オブジェクトとして積まれる一方で，実行名は辞書検索結果がスタック上に積まれる。
* 例えば /ABC (abc) def の登録がある場合，リテラル名の/ABC⇨ /ABC となり 実行名のABC ⇨ (abc) となる。
* 実行名による検索結果が実行配列の場合，スタックに積まれた実行配列は即実行される。

### リテラル配列と実行配列(手続き)

#### リテラル配列を作成する操作
* `[ 1 2 add]` の入力結果は `[ 3 ]` となり，`[`と`]`の間は直ぐに実行
#### 実行配列(手続き)を作成する操作
* `{ 1 2 add }` の入力結果は `{ 1 2 add }` となる。
* `{`をオペランドスタックに載せたら`}`が来るまで、実行可能オブジェクトも実行せず、すべてのトークンをオペランドスタックにpushし、`}`で実行配列を形成してオペランドスタックに載せる
  * ここは実行配列の実行タイミングにはならない

## オペレータの種類

### 演算
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***add***|整数あるいは実数の加算<br/>  _--num#1-- --num#1-- **add**_|_--num#1-- **+** --num#2--_|
|***sub***|整数あるいは実数の減算<br/>  _--num#1-- --num#1-- **sub**_|_--num#1-- **-** --num#2--_|
|***mul***|整数あるいは実数の積算<br/>  _--num#1-- --num#1-- **mul**_|_--num#1-- **x** --num#2--_|
|***div***|整数あるいは実数の除算(整数だけの時は切り捨て発生)<br/>  _--num#1-- --num#1-- **div**_<br/> ーーーーーーーーーーーーーーー<br/>  整数演算 _5 2 **div**_ ➔ 2 切り捨て発生<br/>  小数点演算 _5.0 2 **div**_ ➔ 2.5|_--num#1-- **/** --num#2--_|
|***mod***|整数除算の余り<br/>  _--int#1-- --int#2-- **mod**_|_--int#1-- **mod** --int#2--_|
|***ceil***|切り上げ<br/>  _--num-- **ceil**_|_--num-- **ceil**_|
|***round***|四捨五入<br/>  _--num-- **round**_|_--num-- **round**_|
|***truncate***|切り捨て<br/>  _--num-- **truncate**_|_--num-- **truncate**_|
|***abs***|絶対値<br/>  _--num-- **abs**_|_--num-- **abs**_|
|***and***|整数のAND演算<br/>  _--int#1-- --int#2-- **and**_|_-int#1-- **&** --int#2--_|
|***or***|整数のOR演算<br/>  _--int#1-- --int#2-- **or**_|_-int#1-- **or** --int#2--_|
|***xor***|整数のXOR演算<br/>  _--int#1-- --int#2-- **xor**_|_-int#1-- **xor** --int#2--_|


### 比較
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***eq***|オブジェクト同士 (数値、名前、文字列) の等価比較<br>  _--any#1-- --any#2-- **eq**_| 等しければ _--true--_<br>違えば _--false--_ |
|***ne***|オブジェクト同士 (数値、名前、文字列) の非等価比較<br>  _--any#1-- --any#2-- **ne**_| 等しければ _--false--_<br>違えば _--true--_ |
|***ge***|オブジェクト同士 (数値) の ≧ 評価<br>  _--num#1-- --num#2-- **ge**_| _--num#1-- **≧** --num#2--<br>成立すれば _--true--_ <br>しなければ _--false--_ |
|***gt***|オブジェクト同士 (数値) の ＞ 評価<br>  _--num#1-- --num#2-- **gt**_| _--num#1-- **＞** --num#2--<br>成立すれば _--true--_ <br>しなければ _--false--_ |
|***le***|オブジェクト同士 (数値) の ≦ 評価<br>  _--num#1-- --num#2-- **le**_| _--num#1-- **≦** --num#2--<br>成立すれば _--true--_ <br>しなければ _--false--_ |
|***lt***|オブジェクト同士 (数値) の ＜ 評価<br>  _--num#1-- --num#2-- **lt**_| _--num#1-- **＜** --num#2--<br>成立すれば _--true--_ <br>しなければ _--false--_ |

### スタック操作
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***pop***|先頭スタックをポップ<br>  _--any-- **pop**_ |先頭スタックがpopされた状態|
|***exch***|先頭スタック入れ替え<br>  _--any#1-- --any#2-- **exch**_ | _--any#2-- --any#1--_ |
|***dup***|先頭スタックの複製<br>  _--any-- **dup**_|_--any-- --any--_|
|***roll***|オペランドスタック回転 n個の要素をm個シフトする<br>  _--any#1-- ... --any#n-- --int@n-- --int@m-- **roll**_ <br/>ーーーーーーーーーーーーーーーーーーーー<br/>  _(a) (b) (c) 3 -1 **roll**_ ➔ (b) (c) (a) <br/>  _(a) (b) (c) 3 1 **roll**_ ➔ (c) (b) (a)|回転後のオペランドスタック|
|***clear***|オペランドスタックの全クリア<br/>  _--any#1-- ... --any#n-- **clear**_|オペランドスタックが空に|
|***pstack***|オペランドスタックの表示<br/>  ***pstack***|なし|
|***print***|標準出力/ファイルに指定文字列を出力<br/>  _--string-- **print**_|なし|
|***==***|先頭スタックのみ表示してpopする<br/>  _--any-- **==**_|なし|

### オブジェクト型変換
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***cvi***|数値や文字列を整数に変換<br/>  _--num-- **cvi**_<br/>  _--string-- **cvi**_<br/>ーーーーーーーーーーー<br/>  _1.5 **cvi**_ ➔ 1<br/>  _(123) **cvi**_ ➔ 123|_--int--_|
|***cvs***|数値を文字列に変換<br/>  _--num-- **cvs**_<br/>ーーーーーーーーーーー<br/>  _3.14 **cvs**_ ➔ (3.14)|--string--|
|***cvx***|任意のオブジェクトを実行型に変換<br/>  _--any-- **cvx**_<br/>ーーーーーーーーーーー<br/>  _( 1 2 add ) **cvx**_ ➔ { 1 2 add }<br/>  _[ 1 2 3 ] **cvx**_ ➔ { 1 2 3 }<br/>  _( 1 2 3 ) **cvx**_ ➔ { 1 2 3 }<br/>  _1 2 /add **cvx**_ ➔ 1 2 /add|実行型オブジェクト|
|***cvn***|文字列をリテラル名に変換<br/>  _--string-- **cvn**_<br/>ーーーーーーーーーーー<br/>  _(January) **cvn**_ ➔ /January|リテラル名オブジェクト|


### 配列操作
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***array***|指定長さの配列生成<br/>  _--int#n-- **array**_| 指定数のnullオブジェクトが入った配列<br/> _[--null#1-- ... --null#n--]_ |
|***aload***|配列をオペランドスタックに展開<br/>  _[ --any#1-- ... --any#n] **aload**_|_--any#1-- ... --any#n-- [ --any#1-- ... --any#n]_|
|***copy***|配列aの要素を配列bにコピーする<br/>  _[ --any#**a1**-- .. --any#**an**-- ] [ --any#**b1**-- .. --any#**bn**-- any#**bn-+1**- ] **copy**_|_[ --any#**a1**-- ... --any#**an**-- --any#**bn+1**-- ]_|
|***[***|配列開始|***[***|
|***mark***|配列開始 ( ***[*** と同じ)| ***[***                                                      |
|***]***|オペランドスタックで一番近い ***[*** との間にある全オブジェクトを格納したリテラル配列を生成|リテラル配列|
|***closearray***|オペランドスタックで一番近い ***[*** との間にある全オブジェクトを格納したリテラル配列を生成|リテラル配列|
|           ***}*** |オペランドスタックで一番近い ***{*** との間にある全オブジェクトを格納した実行配列を生成<br /> ***{*** と ***}*** の間は、実行可能オブジェクトであって実行しないので、スタック上にある ***{***  を見る術は無い。|実行配列|
|***put***|配列に任意オブジェクトをindexの位置に挿入する<br/>  _--array-- --int@index-- --any-- **put**_<br/>ーーーーーーーーーーー<br/>  _[ (0) 1 2 (3) 4 ] dup 2 /hoge **put**_ ➔ [ (0) 1 /hoge (3) 4 ]<br/>  _{ (0) 1 2 (3) 4 } dup 2 /hoge **put**_ ➔ { (0) 1 /hoge (3) 4 }|要素を挿入した配列|
|***putinterval***|配列#1に配列#2の要素をindexの位置から挿入する<br/>  _--array#1-- --int@index-- --array#2-- **putinterval**_<br/>ーーーーーーーーーーー<br/>  _[ 0 1 2 3 ] dup 1 [(a) (b)] **putinterval**_ ➔ [ 0 (a) (b) 3]|要素を挿入した配列|
|***get***|配列からindexの位置にある要素を取得する<br/>  _--array-- --int@index-- **get**_<br/>ーーーーーーーーーーー<br/>  _[ (0) 1 2 (3) 4 ] 3 **get**_ ➔ (3)<br />  _{ (0) 1 2 (3) 4 } 3 **get**_ ➔ (3)|配列から取得した要素<br /> _--any--_***|
|***getinterval***|配列からindexの位置からcnt分の要素を配列として取得<br />  _--array-- --int@index-- --int@cnt **getinterval**_<br />ーーーーーーーーーーーーー<br/>  _[ (0) 1 2 (3) 4 ] 3 2 **getinterval**_ ➔ [(3) 4]<br />  _{ (0) 1 2 (3) 4 } 3 2 **getinterval**_➔ {(3) 4}|配列から取得した要素配列<br />_--array--_|
|***length***|配列要素数取得<br />  _--array-- **length**_|要素数<br />_--int--_|
|***maxlength***|配列最大容量取得<br />  _--array-- **maxlength**_|最大容量数<br />_--int--_|

### 辞書
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***begin***|指定辞書を辞書スタックにpushしてカレント辞書にする<br />  _--dict-- **begin**_|なし|
|***def***|Key-Valueペアをカレント辞書に登録する<br />  _--litname@key-- --any@value-- **def**_<br />ーーーーーーーーーーーーーーーーーーーーー<br/>  _/hoge (hoge's value) **def**    hoge_ ➔ (hoge's value)|なし|
|***end***|辞書スタックの最上位辞書をpopし、次の辞書をカレント辞書にする。最下層にあるシステム辞書は***end***出来ない。<br />  _**pop**_|なし|
|***dict***|指定した数のKey-Valueペアを格納する辞書を作成する。Key-Valueペアは指定数を超えてその辞書登録できるが、処理速度が遅くなる可能性がある。<br />  _--int-- **dict**_|*--dict--*|
|***currentdict***|辞書スタックの最上位にある辞書(カレント辞書)をオペランドスタックにpushする<br />  _**currentdict**_|*--dict--* |
|***load***|指定した/nameをKeyにして検索し、得られたValueをオペランドスタックにpushする。Valueが実行可能(実行属性がtrue)であっても実行しない。<br />  _--litname@key-- **load**_<br />ーーーーーーーーーーーーーーーーーーーーー<br/>  _/hogeP {1 2 add} **def**   /hogeP load_ ➔ {1 2 add}|_--any@value--_ |
|***put***|任意のオブジェクトを指定した/nameで指定辞書に登録する。辞書を指定できる点が***def***と異なる。<br />  _--dict-- --litname@key-- --any@value-- **put**_|なし|
|***get***|指定した辞書から指定した/nameのValueを取得してオペランドスタックのpushする。Valueが実行可能であっても実行しない。<br/>  _--dict-- --litname@key-- **get**_| _--any@value--_ |
|***length***|指定した辞書から登録アイテム数を取得してオペランドスタックにpush<br />  _--dict-- **length**_| _--int--_ |
|***maxlength***|指定した辞書の容量(Key-Valueペア登録可能最大容量)を取得<br />  _--dict-- **maxlength**_| _--int--_ |

### 文字列
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***search***|文字列#1から文字列#2の部分一致を検索する<br />  _--string-- --seek-- **search**_<br />ーーーーーーーーーーーーーーーーーーーーー<br/>  _(abbc) (ab) **search**_ ➔ (bc) (ab) () true<br />  _(abbc) (bb) **search**_ ➔ (c) (bb) (a) true<br />  _(abbc) (bc) **search**_ ➔ () (bc) (ab) true<br />  _(abbc) (B) **search**_ ➔ (abbc) false|あった場合:<br />  _--post-- --match-- --pre-- --true--_<br />ない場合:<br />  _--string-- --false--_|
|***concat***|文字列#1と文字列#2を結合した文字列を生成する<br />  _--string#1-- --string#2-- **concat**_ <br />ーーーーーーーーーーーーーーーーーーーーー<br/>  _(abc def) (ghi jkl) **concat**_ ➔ (abc defghi jkl)| _--string--_ |
|***put***|文字列の指定index位置に指定した文字コードを挿入する<br/>  _--string-- --int@index-- --int@charcode-- **put**_<br/>ーーーーーーーーーーーーーーーーーーー<br/>  _(abcdefg) dup 2 32 **put**_ ➔ (ab defg)|_--string--_|
|***putinterval***|文字列#1に文字列配列#2の要素をindexの位置から挿入する<br/>  _--string#1-- --int@index-- --string#2-- **putinterval**_<br/>ーーーーーーーーーーーーーーーーーーーーーーー<br/>  _(abcdefghi) dup 2 (1234) **putinterval**_ ➔ (ab1234ghi)| _--string--_ |
|***get***|文字列からindexの位置にある文字コードを取得する<br/>  _--string-- --int@index-- **get**_<br/>ーーーーーーーーーーー<br/>  _(abcdefg) 3 **get**_ ➔ 100| _--int--_ |
|***length***|指定した文字列の文字列長を取得する<br />  _--string-- **length**_| _--int--_ |
|***getinterval***|文字列からindexの位置からcnt分の文字を文字列として取得<br />  _--string-- --int@index-- --int@cnt **getinterval**_<br />ーーーーーーーーーーーーー<br/>  _(abcdefghijkl) 3 4 **getinterval**_ ➔ (defg)| _--string--_ |

### 実行
|オペレータ|説明／使用例|出力|
|--:|--|:--|
|***exec***|オペランドスタック最上位のオブジェクトを実行<br />・オブジェクトが実行属性がtrueであれば実行<br />・オブジェクトの実行属性がfalseであれば何も起きない<br />  _--any-- **exec**_| なし |
|***run***|指定したmyshスクリプトファイルを実行<br />  _--string-- **run**_<br />ーーーーーーーーーーーーー<br/>  _(/temp/sample.txt) **run**_| なし |

### ループ
|オペレータ|説明／使用例|出力|
|--:|---|:--|
|***for***|initial値からlimit値までのstep値を指定し、その間の数値をスタックに乗せてprocを実行。数値は整数実数を問わない<br/>  _--num@initial-- --num@step-- --num#limit-- --proc-- **for**_<br/>ーーーーーーーーーーーーーーーーー<br/>  _1 1 5 {cvs} **for**_ ➔ (1) (2) (3) (4) (5) |procの処理内容に依存|
|***forall***|配列，辞書，文字列の全要素に対してprocを実行<br/>  _--array-- --proc-- **forall**_<br/>  _--dict-- --proc-- **forall**_<br/>  _--string-- --proc-- **forall**_<br/>ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー<br/>  _[ (a) (b) (c) 12 ] {} **forall**_ ➔ (a) (b) (c) 12 <br />  _3 dict begin /v#1 8 def/v#2 9 def currentdict {} **forall**_ ➔ /v#1 8 /v#2 9<br />  _(abcdef) {} **forall**_ ➔ 97 98 99 100 101 102|procの処理内容に依存|
|***loop***| procの中で明示的に***exit***するか、エラー発生までprocを繰り返し処理する<br/>  _--proc-- **loop**_<br/>ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー<br/>  _/cnt 0 def {/cnt cnt 1 add dup 3 1 roll def cnt 5 ge {**exit**} if } **loop**_ ➔ 1 2 3 4 5 |procの処理内容に依存|
|***repeat***| 指定回数分だけprocを繰り返し実行する<br />  _--int@count-- --proc-- **repeat**_<br/>ーーーーーーーーーーーーーーーーー<br/>  _3 {null} **repeat**_ ➔ --null-- --null-- --null-- | procの処理内容に依存 |
|***exit***|for, forall, loop, repeatの繰り返しから強制的に抜ける<br/>ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー<br/>  _/cnt 0 def {/cnt cnt 1 add dup 3 1 roll def cnt 5 ge {**exit**} if } **loop**_ ➔ 1 2 3 4 5|なし|

### 分岐
|オペレータ|説明／使用例|出力|
|--:|---|:--|
|***if***|IF分岐。trueの時のみ手続きを実行する指示。<br/>  _--bool-- --proc@true-- **if**_<br/>ーーーーーーーーーーーーー<br/>   _true { 1 2 add } **if** ➔ 3_<br/>   _false { 1 2 add } **if** ➔ none_ |proc@trueの処理に依存|
|***ifelse***|IF-ELSE分岐<br/>  _--bool-- --proc@true-- --proc@false-- **ifelse**_<br/>ーーーーーーーーーーーーー<br/>   _true { 1 2 add } { 3 4 add } **ifelse** ➔ 3_<br/>   _false { 1 2 add } { 3 4 add } **ifelse** ➔ 7_ |proc@true, proc@falseの処理に依存|

### 計測
|オペレータ|説明／使い方|出力|
|--:|----|:--:|
|***tickcount***|msecオーダのTick Countを返す<br/>  ***tickcount***| _--int--_|

### 日時
|オペレータ|説明／使い方|出力|
|--:|---|:--|
|***time***|その時の日時を取得<br/>  ***time***|_--string--_<br/>文字列のフォーマットは(日付@時間)<br/>例: _(2025/01/05@12:00:00)_|

### ファイル
|オペレータ|説明／使い方|出力|
|--:|--|:--|
|***readfile***|読み込み用ファイルを開く<br/>ファイル読み込み:<br/>  _--string@filename-- **readfile**_<br/>※***closefile***するまでファイルを掴んだまま<br />標準入力:<br/>  _null **readfile**_|_--file--_|
|***read***|ファイル/標準入力から1文字読み込み<br/>  _--file-- **read**_|_--int(charcode)-- --true--_ for not EOF<br/>_--false--_ for EOF<br/> |
|***closefile***|ファイルクローズ<br/>  _--file-- **closefile**_|なし|

### メモリチェック
|オペレータ|説明／使い方|出力|
|--:|----|:--:|
|***enummem***|使用中のメモリーブロックを画面にリスト表示<br/>  ***enummem***|なし|
|***checkmem***|メモリー破壊箇所を画面にリスト表示<br/>  ***checkmem***|なし|

### その他コマンド
|オペレータ|説明／使い方|出力|
|--:|----|:--:|
|***prompt***|プロンプト表示<br/>mysh起動後に_**prompt**_と入力して改行すると***msh>***を表示して入力を促す|なし|
|***cd***|指定したフォルダをカレントフォルダにする<br/>  _--string@path-- **cd**_<br />ーーーーーーーーーーーーー<br/>例) /tempに移動したい時は<br/>  _(/temp) **cd**_|なし|
|***pwd***|カレントフォルダを文字列にしてスタックに載せる<br/>  _**pwd**_| _(directory-name)_ |
|***enumfiles***|filter検索したファイル名を順次スタックに載せてprocを実行<br/>  _--string@filter-- --proc-- **enumfiles**_<br/>例) カレントフォルダにあるファイル列挙をする場合<br/>  _(*) { print (\n) print} **enumfiles**_| procで何をスタックに積むかに依存 |
|***getenv***|環境変数値の取得<br/>環境変数を文字列で指定して**getenv**<br/>  _--string@env-- **getenv**_ | _(value)_|
|***messagebox***|メッセージボックスを表示してYES/NOの選択を促し、その結果をスタックに積む<br/>表示パネルのtitleとmessageを設定して実行<br/>  _--string@title-- --string@message-- **messagebox**_ | YES ➔ _--true--_<br/>NO ➔ _--false--_ |
|***async***|外部コマンドを非同期に実行<br/>配列にコマンド文字列を入れて実行<br/>  _[ --string-- --string-- ... ] **async**_<br/>[]内のコマンド文字列は1つの文字列に連結して実行する|_--int--_<br/>プロセス識別番号|
|***finished***|呼び出した外部コマンドの終了状態確認<br/>**async**で取得したプロセス識別番号に対して**finished**を実行<br/>  _--int@pid-- **finished**_|済み➔_--true--_<br/>実行中➔_--false--_ |
|***sleep***|秒単位の停止<br/>  _--int@sec-- **sleep**_|なし|
|***quit***|mysh終了<br/>***quit***|なし|



