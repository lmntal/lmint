# TODO

## 実装

`unordered_map<Atom*, int>` と `stringをstoi変換するの` とで性能の差を測る


- class VM に諸々を入れる


### Guard を Enum で実装
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
    - 変数の依存関係がDAGであるか検査(Parse時)(後回し)


### UnaryIndex

- 変にunordered_mapでhashをしたりmapで赤黒木を操作するよりは単純なforループの方が軽いことさえあることを念頭に置き、定数倍高速化は後でも構わないことを覚えておこう

```
class UnaryIndex{
    map<Functor, map<int, unordered_map<string, list<Atom*>>>>
    
    list<Atom*> &retrievekey(Functor &dst_functor, int dst_pos, string &unary_name) {

    }
    
}
```

#### 挿入・削除

- relink時にunaryかどうか見て、そうであればindexingする
- これに保存しておけば、unaryが消えるもしくはrelinkが行われる時に消せばいい

```
unordered_map<Atom*, list::iterator<Atom*>> unary_indexing_itr;
             ↑unary atom

Atom::Atom(Functor functor_): functor(functor_) {
    link.resize(functor.arity);
    atomlist[functor].push_front(this);
    maneged_iterators[this].push_back(atomlist[functor].begin());
}

Atom::~Atom() {
    maneged_iterators.erase(this);
}

void connect_links(Atom *atom1, int pos1, Atom *atom2, int pos2) {
    if (atom1->functor.arity == 1) {
        // atom2がunary

    }
    if (atom2->functor.arity == 1) {
        // atom1がunary
        
    }
}
```


```
a(X),b(Y,Z)  :- X=:=Z, X=:=Y | 
```

b(Y,Z) 決定

Yに対してa(X)をretrieve可能
Zに対してa(X)をretrieve可能

この場合、 Z=:=Yをつけると、高速化できるがそれはユーザーに任せることとする

左辺・右辺それぞれのeval結果を保持
retrieve可能な変数(条件式の片側の辺の式が Link =:= Exp もしくは Exp =:= Link であり、かつHeadの自由リンク)の逆側の辺(Exp)がeval済みなとき、retrieveをする。

Ruleは [ retrieve可能な変数(X) (var_id) -> eval済み対辺(Y)(Z) (compare_id, LHS or RHS) のタプル ] のマッピング (複数)(vector?) をもつ 対辺

find_atom時に
- retrieve可能な変数(X) (var_id) が未決定
- a(X)のaが未決定
- 対辺(Y)(Z)がeval済み


follow(A1, B1) 
A1: NULL or 




```
query(A,B), follow(A1,C1), follow(C2,B2) :-
    A =:= A1, B =:= B2, C1 =:= C2, uniq(A,B,C1) |
    query(A,B), follow(A1,C1), follow(C2,B2), common(A,B,C1).
``` 

`head_atoms_id` -> {`(unary_pos, value, functor)`, ... } -> 
`follow(A1,B1)` には `follow(var[A1],*)` のUI と `follow(*,var[B1])` のUI がある -> この「ある」はロード時にやる？
つまり `UI(follow/2, 0, var[A1])` と `UI(follow/2, 1, var[B1])` のlistがある



辺

```
map<int, int> expected_value;
expected_value[`A1`]
```
対辺が満たされたら登録

expected_value[`B1`]

expected_valueを外すタイミングは、入れるタイミングのところでバックトラックに任せるだけ


? 「eval済み -> evalまだ」と「evalまだ -> eval」のタイミングをどうするか

e


Atom* はdst側のAtom
もしこのアトムが

* 管理の仕方
    * 追加？
    * 削除
    * iterator

```
list<Atom*> *candidates;
```

```
unordered_map<Atom*, bool> deleted_atoms;
のような管理はできません。
なぜなら、消したアトムのアドレスは、解放してから新たに使われる可能性があるからです。
（そのために解放している）
よって、固有なIDでも振らないとだめです→もうそれ、vectorに詰めてもよくない？ 
```

```
シンボルアトム側で管理はしない。面倒なので。
X = a(Y) :- X = Y.
でXとYそれぞれ再度索引付けが必要となる
例えば、a(N0,N1,N2)とあって、N1が自由リンクの繋ぎ変えで変わったとする。
すると
```





- 片方の辺が決まって、かつもう片方が自由リンクならばUnaryIndexを使う




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
