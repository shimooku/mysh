# mysh

## 言語仕様

### オペランド スタック

例えば足し算１＋２を実行する場合，１と２を順番にオペランドスタックに積んだあとにaddというオペレータ（後述）を実行するようにスクリプトを作成する。このスクリプトを実行するとオペレータ`add`は必要なパラメータ１と２をスタックから取得（＆削除）して計算結果３をオペランドスタックに積む。
```
SMS> 1
SMS> 2
SMS> add
SMS> ==
3
```
|オブジェクト|説明|
|---|---|
|Boolean|true, false|
|整数|-2, -1, 0, 1, 2, ...|
|浮動小数|-1.0, -20., 0.0, 1.0, 2.0, ...|
|配列開始マーク"["|オペランドスタックに配列要素を積む直前の配列開始マーク|
|[リテラル名](# リテラル名と検索名)|/ABC, /abc, /Abc, ....|
|[検索名](# リテラル名と検索名)|ABC, abc, Abc, ....|
|文字列|(ABC), (abc), (Abc), ...|
|[リテラル配列](# リテラル配列と実行配列) array|[1 2 3], [(a) (b) (cd)], [(abc) [10 0.2] /cc], ...|
|[実行配列](# リテラル配列と実行配列) proc|{1 2 3}, {(a) (b) (cd)}, {(abc) [10 0.2] /cc}, ...|
|空オブジェクト"null"|10 arrayの操作で生成される配列の初期要素などで使用|
|辞書| Key-Valueのペアの集合|

### リテラル名と検索名

リテラル名はスタック上にリテラル名オブジェクトとして積まれる一方で，検索名は辞書検索結果がスタック上に積まれる。例えば /ABC (abc) def の登録がある場合，/ABC ⇨ /ABC となり ABC ⇨ (abc) となる。検索名による検索結果が実行配列の場合，スタックに積まれた実行配列は即実行される。

### リテラル配列と実行配列

リテラル配列操作では [ 1 2 add] ⇨ [ 3 ] となり，'['と']'の間の操作は直ぐに実行される。以降これを`array`と表記する。
一方で，実行配列操作では { 1 2 add } ⇨ { 1 2 add } となり，'{'と'}'の間の操作は直ぐに実行されない。以降これを`proc`と表記する。なお実行配列`proc`は，検索結果のValueとしてスタックに積まれたら即実行される。

### 辞書／辞書スタック

* Key-Valueペアが登録されている複合オブジェクトを辞書と呼ぶ。辞書にはシステム辞書（TOSが定義したKeyValueペア群）と，ユーザ定義辞書（ユーザがTOSスクリプトで定義するKeyValueペア群）の2種類があり，ユーザ定義辞書は明示的に作成（`dict`オペレータで生成し，`begin`オペレータで辞書スタックに載せる）しない限り有効にならない。        
* 使いたい辞書（KeyValueペアを登録辞書／検索対象にしたい辞書）は`begin`オペレータで辞書スタックに載せる必要がある。
* 登録は，辞書スタックの最上位の辞書に対して行う。
* 検索は，辞書スタックの最上位から最下位まで，該当するKeyが見つかるまで行う。
* `end`オペレータは，辞書スタック最上位の辞書を外す処理を行う。また，辞書スタックから外れた辞書は，Valueとして登録されている限り，`begin`オペレータで何度も辞書スタックに戻すことが出来る。
* システム辞書は辞書スタックから外すことは出来ない。

## オペレータ

### ヘルプ
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`help`|コマンド仕様表示|/dup `help`|標準出力にdupコマンド仕様を表示|

### 演算
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`add`|加算|1 2 `add`|3|
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
|`begin`|指定辞書を辞書スタック先頭に載せてカレント辞書にする|10 dict `begin`|*--dict--*|
|`def`|辞書登録|/Name (value) `def`|カレント辞書にNameという名前で文字列(value)を登録|
|`end`|カレント辞書を閉じる|--dict-- `end`|辞書スタック先頭の辞書をPOPしてその下の辞書をカレント辞書にする|
|`dict`|辞書を作成する|10 `dict`|*--dict--*|
|`currentdict`|辞書スタックの先頭にある辞書をスタックに載せる|`currentdict`|*--dict--* |
|`load`|カレント辞書に登録されているValueをスタックに載せる|/Name (value) def /Name `load`|(value) |
|`put`||3 dict dup /ABC (abc) `put` begin /ABC load|(abc)|
|`get`||3 dict begin /ABC (abc) def currentdict /ABC `get`| (abc) |
|`length`|登録アイテム数取得|3 dict `length`| 0 |
|||3 dict /ABC (abc) def currentdict `length`| 1 |
|`maxlength`|容量取得|10 dict `maxlength`| 10 |
|`forall`|全辞書エントリーに対して*--proc--*を実行|10 dict begin /A (a) def /B (b) def /C (c) def currentdict { } `forall `end| /A (a) /B (b) /C (c) |

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
|`tickcount`||- `tickcount`| *--num--* as time in msec                                    |

### 日時
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`time`||- `time`|*--string--* like (2021/12/20@12:00:00)|

### 標準出力
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`stdout`|*--string--* \| *--null--* `stdout`|(/temp/poi.log) `stdout`| 指定ファイルに標準出力 |
|||null `stdout`|画面に標準出力|

### メモリチェック
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`enummem`|使用中のメモリーブロック表示|- `enummem`|使用中のメモリーブロック表示|
|`checkmem`|メモリー破壊箇所表示|- `checkmem`|メモリー破壊箇所表示|

### その他コマンド
|オペレータ|説明|使用例|結果|
|--:|:-|--:|:--|
|`prompt`|プロンプト（SMS>）表示|- `prompt`| プロンプトを表示して入力を促す |
|`cd`|カレントパス変更|(/temp) `cd`| /tempに移動 |
|`pwd`|カレントパスを文字列にしてスタックに載せる|- `pwd`| (/temp) |
|`enumfiles`|filter検索されたファイル名を順次スタックに載せて*--proc--* を実行|(*) { } `enumfiles`|  |
|`getenv`|環境変数値の取得|(HOME) `getenv` |<string> |
|`messagebox`|メッセージボックスを表示してYES/NOの選択を促す|(TITLE) (YES to continue, NO to stop) `messagebox` | --true/false-- |
|`quit`|TOS終了|- `quit`| TOS終了 |

### コメント
* 行内で `%`, `#`, `;` 以降の文字列は無視する



