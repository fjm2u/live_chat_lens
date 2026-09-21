# 検証状態 — 2026-09-21

## 確認済み

- macOS arm64、OBS Studio 32.2.2、OBS実行時Qt 6.11.1。ビルド側Qt 6.11.2 / AppleClang 21 / CMake 4.4.3。
- Native moduleのコンパイル成功。QtはOBS同梱Frameworkへリンクを付け替え。
- ローカルPluginインストールとad-hoc署名検証成功。
- OBS起動ログ `2026-09-21 21-24-43.txt` のLoaded Modulesに `obs-comment-dock` を確認。
- OBS `Docks → AI Comments` からNative Dockを開き、Live URL、マスクされたAPI入力欄、Hostileフィルター、Minimum recommendation、接続ボタン、推薦待ち表示をUIで確認。
- Jevキーは現在のDockメモリへ入力済み。YouTubeキーと動画URLは未入力。キーはソース・設定ファイルに保存していない。
- CTest 3/3 PASS：core、protocol、ui（offscreen）。フィルター・順位・多様性・3秒固定・high追加・件数上限・hostile除外・不正応答・HTMLのPlainText表示・👍重複防止・×・旧session結果拒否・thread終了を検証。

## Jev実APIスモーク

合成コメント4件、12 Choiceを1つの実API requestで送信。HTTP 200、HTTP全体1611 ms。本文はこのテストに書かれた合成例のみ。以下は観測値で、統計的な性能保証ではない。

| 入力例 | hide | pick_value | comment_type |
|---|---|---|---|
| B2Cだと誰がAPI代払うの？ | show | high | question |
| そのビジネスモデル成立しなくない？ | show | high | question |
| お前頭悪すぎ | hide | low | reaction |
| こんにちは | show | low | other |

## 未確認・未達

- YouTubeの実ライブ受信→Jev→OBSの一連の動作：YouTube Data APIキーと対象ライブURL待ち。
- p95 < 1秒：未証明。1回のAPI実測自体が1611 ms。150 ms buffer、待ち行列、UI固定を含む実運用のp95は未測定。
- 3 KPI（Hide Precision、Recommendation Precision、Discovery Improvement）：人による評価データ未収集。
- 実配信・実録画中の障害注入、フレーム落ち比較、長時間運転：未実施。
- Windows/Linuxの実機ビルド、配布用署名・公証：未実施。

したがって「実装・macOS導入・Dock表示・Jev単体接続は確認済み」であり、MVP完成条件全体の受け入れは未完了。
