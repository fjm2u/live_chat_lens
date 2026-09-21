# 大量コメント検証 — 2026-09-21

## 再実行

設定済みのbuildディレクトリで実行します。

```sh
CMAKE_COMMAND=/Users/yuki/Library/Python/3.9/bin/cmake scripts/test-load.sh
# 実Jev API: 保存済みキーチェーンのキーを使用。API利用料が発生し得ます。
python3 scripts/test-jev-live.py
```

結果は `build/reports/load-test.json` と `build/reports/jev-live.json`。キーは出力しません。YouTubeへの投稿は行いません。

## ローカル結果

- 入力10,000件、Basic Filter通過6,003件、除外3,997件。
- 実際のCommentDockへ16件単位で376バッチを投入（Qt offscreen）。最大表示10件。
- 攻撃ラベルのfixture 1,000件・低評価fixtureが表示されないことを検証。
- 同一ユーザー1,000件/同時刻のうち通過3件。
- mediumの位置保持、60秒経過後の期限切れ、新規候補による復旧、worker終了を検証。
- 全体3,166 ms、UIバッチ処理p95 8.92 ms、最大80.01 ms。

これはBasic Filterと順位・実UIロジックのバースト負荷検証です。AIスコアは固定fixtureです。YouTube受信、Engineのネットワーク待ち行列・buffer・過負荷ドロップ、実OBSの描画・配信負荷、実AIの大量処理能力は測定していません。UI処理時間を受信→推薦のレイテンシとして扱うことはできません。

## 実Jev API結果

4種類の合成コメントを4回送信、計16件。4回すべてHTTP 200。HTTP時間759 / 943 / 776 / 967 ms。質問high、批判show、攻撃hide、挨拶lowの条件は4/4成功。批判のpick/typeには揺れがあります。

事前の接続確認1回（4件、915 ms）を含めると、この作業の実API呼び出しは5回・20件です。繰り返し例による少数検証のため品質KPI・高負荷時p95の保証ではありません。実APIスクリプトは初期プロンプトを使用し、Dockに保存したカスタム方針は使用しません。
