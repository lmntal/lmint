# Lmint
LMNtal interpreter

## 使い方

### コンパイル
```bash
./compile.sh
```

### 実行
```bash
./lmint < {LMNtal program}
```

## TODO

### 空のBody
a :- . 
に対応

### Parseのエラー処理
* ファイル最後の. 忘れでセグフォ

### 最適化
* 「未決定アトムがない」をfor文ではなく簡単なカウントでできそう

### Guard
RuleLinkに型制約やGuard制約を設ける

### Docker化

### オプション
* トレース実行
* ランダム実行
* グラフ no dump
* パース情報dump
* バックトラック回数dump
* ルール実行時間dump


### データアトムの管理
int, float, string のデータをどこに管理するかというと、int と　string は最悪functorのどっちかをいじればいいが、floatのことを考えるとむずかしい？

そこで、Atomクラスに静的な vector<int>, vector<double>, vector<string>
を用意して、そのindex だけもつ <"string", 142> のように
あまりに不要な要素の割合が高くなったら、どうする？

そもそも、atomlistがstd::listなのはfindatomでforで回すからであって、
それをしないint, float, stringのデータは配列管理でもいいのではないか


### atomlistの追加方法改良
失敗したアトムを末尾に入れる
