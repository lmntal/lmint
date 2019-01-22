# TODO

## 実装
```
unordered_map<Atom*, int> と stringをstoi変換するの とで性能の差を測る
```
- Guard を Enum で実装
-- read_expを実装
-- eval_expを実装 
-- 代入使わない例で検証


- rule側のtypecheckを消す

```
in Rule

enum class LinkType{
    FreeLink, Unary, Int, Float, string
};

vector<LinkType> var_types;
```



```
in Gurad

template<typename T>
class Expression{
  public:
    enum class Token{
        LParen, RParen, Plus, Minus, Asterisk, Slash, Mod
    };
    vector<T> const_values;
    vector<T> var_ids;
}
```

```
in Compare

enum class RelativeOp{
    EQ, NEQ, GE, GT, LE, LT
};
```
```
in Guard

template<typename T>
class Compare {
  public:
    Expression<T> left_exp;
    RelativeOp op;
    Expression<T> right_exp;

    Compare();
    ~Compare();
}

template<typename T>
class Assign {
  public:
    int new_var_id;
    Expression<T> exp;

    Assign();
    ~Assign();
};
```

```
in Register
map<int, int> int_var;
vectorと違ってeraseができる
```

- rewrite時にGuardに現れるvarの生成と削除
    - ひとまず全て削除、現れたら生成でいいのでは？
    - 代入の有無は実は関係ない

- 代入を実装する
    - Registerへのストア
    - 変数の依存関係がDAGであるか検査(Parse時)

- unary index の設計を錬る
    - class UnaryIndex
    - 全てのAtomのリンク接続時にunaryかどうか見て、そうであればindexingする

- 片方の辺が決まって、かつもう片方が自由リンクならばUnaryIndexを使う


- class VM に諸々を入れる

## 命名
- TopSet ->
- Register ->
- const 修飾子をつける



## データ型の最適化
- unordered_mapの性能を体感で知る
    - mapと比べて2~3倍速い
- atomlistをunordered_mapに

- `unordered_set<Atom*> deleted_atoms;`
    - findatomのサイズチェックはlist.size - 削除した数にする？
    - findatomのforを回す時に、deleted_atomsにないかチェックして、削除済みのものであれば、erase


# EBNF

parse
```Bash
    Gurad := (TypeCheck | Compare | Assign) {',' (TypeCheck | Compare | Assign)}
    TypeCheck := Type '(' Link ')'
    Type := int | unary
    Compare := Exp COP Exp
    Assign := Link '=' Exp
    COP := '=:=' | '=\=' | '<' | '=<' | '>' | '>='
    AOP := '+' | '-' | '*' | '/' | 'mod'
    Exp := Factor { AOP Factor }
    Factor := Link | Num | '(' Exp ')' | ('+'|'-') Factor
    Num := [0-9]+
```

eval
```Bash
    COP := '=:=' | '=\=' | '<' | '=<' | '>' | '>='
    TOP := '+' | '-'
    FOP := '*' | '/' | 'mod'
    Exp := Term { TOP Term }
    Term := Factor { FOP Factor }
    Factor := Link | Num | '(' Exp ')' | TOP Factor
    Num := [0-9]+
```

## check_link_num_of_ruleの仕事

Guardに現れるリンク名が全てHeadにある物であることを確認

Bodyに現れるはずのリンクがなくても、Guardに現れていればOK


```
gen(N) :- N > 0, N1 = N - 1, R = rand(100000) | gen(N1), a(R).
a(X), b(Y) :- X =:= Y | c(X).
a(X), b(Y), c(Z) :- X*X + Y*Y =:= Z*Z | triangle(X,Y,Z).
gen(0,R) :- int(R) | .
```

型制約は一旦省いて考えてもいい？


## set_ruleの仕事

[データ構造] Guardに出てきたリンクに全てint制約(unary, ground, etc...)を設ける

[データ構造] リンク名 -> gurad文の片辺(many)へのmap(vector?)

[データ構造] gurad文の片辺 -> リンクの個数

[]

## 実行中のショートカット

- gurad文の片辺が揃う
- =:=
- 対辺がリンク名のみ

の時にそのリンク名でfindatom

## 普通のGuard検査

- 型制約があれば検査
- guard文の両辺のリンクが揃ったら検査


# 落書き

```
Ret = p(A,B,C,D)
    :- A + 2 =:= B, E = C + 1, int(D), F = E * 2, F =:= B, G = rand(1000)
    |  Ret = a(A), a(A), b(B), e(E), g(G).
```

## PrsGuard -> Guard
```
PrsGuard :
map<string, int> var_id = {A:0, B:1, C:2, D:4, Ret:5 | E:6, F:7, G:8}
```

Each Exp
```
A + 2 :
const_int = {2}, var = {0}
tokens = {Var, Plus, Constant}

B :
const_int = {}, var = {1}
tokens = {Var}

C + 1 :
const_int = {1}, var = {2}
tokens = {Var, Plus, Constant}

E * 2 :
const_int = {2}, var = {6}
tokens = {Var, times, Constant}

F :
const_int = {}, var = {7}
tokens = {Var}

B :
const_int = {}, var = {1}
tokens = {Var}

rand(1000) :
const_int = {1000}, var = {}
tokens = {BracketL, Constant, BracketR}
```

double, int, string に対応するためにtemplate<T> class Exp でもいいかも

## Guard

var_in_gurad = {0:true, 1:true, 2:true, 3:true, 4:true, 5:false, 6:true, 7:true, 8:true}


## Register



- イコールをみたせるのが複数あるとき、少ないものから選ぶ
- 連結グラフのルートが１つに決まればグラフを一意に表現できる


```
a(*,*,3221)
a(b/1/0,432,*)
a(?,*,a/3/2) -> a(?,*,a/3/2)

a(b,X)
a(b,X) int(X)
a(A,X)

a(b/1/0, *)
a(b/1/0, ?)
a(*, *)
```
