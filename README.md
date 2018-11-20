# Flat LMNtal Interpreter

## 使い方

コンパイル
```bash
./compile.sh
```

実行
```bash
./lmntal-interpreter < {LMNtal program}
```

## TODO

a :- .
に対応
----------------------------------------------
失敗したアトムを末尾に入れる
----------------------------------------------
ファイル最後の.忘れでセグフォ
----------------------------------------------
「未決定アトムがない」をfor文ではなく簡単なカウントでできそう
----------------------------------------------
RuleLinkに型制約やGuard制約を設ける

## メモ

int, float, string のデータをどこに管理するかというと、
int と　string は最悪functorのどっちかをいじればいいが、
floatのことを考えるとむずかしい？

そこで、Atomクラスに静的な vector<int>, vector<double>, vector<string>
を用意して、そのindex だけもつ <"string", 142> のように
あまりに不要な要素の割合が高くなったら、どうする？

そもそも、atomlistがstd::listなのはfindatomでforで回すからであって、
それをしないint, float, stringのデータは配列管理でもいいのではないか
----------------------------------------------
cyclohexane の評価例題

Hがn個, Cがm個
n >> m のばあい

SLIMは nP3 * m
----------------------------------------------
* 簡単な例題だけでいいから測定とその結果
* ルール実行の様子をシクロヘキサンで説明
    * ルールのデータ構造を図に
