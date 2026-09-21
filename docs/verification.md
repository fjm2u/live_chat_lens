# 検証状態 — 2026-09-21

## 更新版の再テスト（22:17–22:19 JST）

限定公開 `xRsauxJe1eo` に実投稿し、OBS画面とログを照合。

- B2Cだと誰がAPI代払うの？: show/high、★表示381 ms。
- そのビジネスモデル成立しなくない？: show/high、★表示1358 ms。
- お前頭悪すぎ: hide/low、269 msで判定、表示イベントなし。
- こんにちは: show/low、352 msで判定、表示イベントなし。
- ユーザー投稿「その猫は、犬ですか？」: show/medium、表示398 ms。
- 新しいカード・接続済み表示・設定欄が閉じた状態を実画面で確認。自動折りたたみの瞬間は操作開始前のため未観測（遷移はUIテスト済み）。
- 👍と×を操作し、各ログとカード削除を確認。
- 表示3件のp95は1358 ms。1秒未満の目標は未達。YouTube投稿→受信は別途2398–4466 ms。
- Dropped Framesは0、エンコード過負荷警告は再発。原因の切り分けは未実施。
- 再テスト後も配信を継続。

## 確認済み

- macOS arm64、OBS Studio 32.2.2、OBS実行時Qt 6.11.1。ビルド側Qt 6.11.2 / AppleClang 21 / CMake 4.4.3。
- Native moduleのコンパイル成功。QtはOBS同梱Frameworkへリンクを付け替え。
- ローカルPluginインストールとad-hoc署名検証成功。
- OBS起動ログ `2026-09-21 21-24-43.txt` のLoaded Modulesに `obs-comment-dock` を確認。
- OBS `Docks → AI Comments` からNative Dockを開き、Live URL、マスクされたAPI入力欄、Hostileフィルター、Minimum recommendation、接続ボタン、推薦待ち表示をUIで確認。
- Jevキー、対象動画URL、既存YouTube OAuthファイルをDockへ入力。キーはソース・設定ファイルに保存していない。OAuthは既存スコープのrefreshのみ。
- CTest 3/3 PASS：core、protocol、ui（offscreen）。フィルター・順位・多様性・3秒固定・high追加・件数上限・hostile除外・不正応答・HTMLのPlainText表示・👍重複防止・×・旧session結果拒否・thread終了を検証。

## Jev実APIスモーク

合成コメント4件、12 Choiceを1つの実API requestで送信。HTTP 200、HTTP全体1611 ms。本文はこのテストに書かれた合成例のみ。以下は観測値で、統計的な性能保証ではない。

| 入力例 | hide | pick_value | comment_type |
|---|---|---|---|
| B2Cだと誰がAPI代払うの？ | show | high | question |
| そのビジネスモデル成立しなくない？ | show | high | question |
| お前頭悪すぎ | hide | low | reaction |
| こんにちは | show | low | other |

## 実ライブ通しテスト（2026-09-21 21:40–21:52 JST）

ユーザーの承認を得て「猫の遊び時間｜無音ループ配信」(video ID `dY4gYgNme4M`) を限定公開で開始。チャットを有効化し、合成コメントを投稿した。マイクとMediaはミュート。YouTube Studioの「ストリーム完了」を確認し、OBS送信も停止した。

- 実OBSで不足していたQt TLS backendをPluginへ同梱して修正。証明書検証は有効のまま、OAuth→YouTube→JevのHTTPS通信が成功。
- Dockの `● YouTube connected ● Jev connected` を確認。
- 11件受信・11件評価・5件表示・1件hide。本文を含まない `events.jsonl` と実画面を照合。

| 合成テスト入力 | 結果 | 受信→表示 |
|---|---|---|
| この猫のおもちゃは何歳くらいから遊べますか？ | show / medium / question、表示 | 345 ms |
| 同じ映像の繰り返しだと少し飽きるかも。別のおもちゃも見たいです | show / medium / opinion、批判を保持 | 584 ms |
| お前頭悪すぎ | hide / low / reaction、非表示 | 評価339 ms |
| こんにちは | show / low / other、非表示 | 評価279 ms |
| B2Cだと誰がAPI代払うの？ | show / high / question、★付き表示 | 309 ms |

- 質問カードで👍→無効化、×→カード消去を操作し、`thumbs_up` / `dismissed` 各1件を確認。
- 表示5件のp50=345 ms、p95=616 ms（nearest rank、少数標本）。YouTube投稿→受信の遅延は別フィールドで記録。
- 配信終了をDockも検知。OBSでエンコード過負荷警告とDropped Frames 522（約2.4–2.8%）を観測。原因比較は未実施のため、Pluginの配信負荷が問題ないとは断定しない。
- `配信なんてやめちまえ` は show / low で非表示だった。非表示の全件がhostile判定によるものではない。

## 未確認・未達

- p95 < 1秒の継続保証：未証明。今回の表示5件では目標内だが、単体APIで1611 msの観測もあり、高流量・長時間での検証が必要。
- 3 KPI（Hide Precision、Recommendation Precision、Discovery Improvement）：人による評価データ未収集。
- 実配信・実録画中の障害注入、フレーム落ち比較、長時間運転：未実施。
- Windows/Linuxの実機ビルド、配布用署名・公証：未実施。

実装・macOS導入・実YouTube→Jev→OBS表示・フィードバックの通し動作は確認済み。MVP完成条件全体の受け入れと品質KPIの評価は未完了。

## Dockデザイン更新

- ダーク背景、角丸カード、16px本文、上位4件の本文強調、高評価のブルー枠と★、日本語カテゴリ、小さいフィードバック操作へ更新。
- YouTube接続とJevの最初の評価成功後に設定欄を自動で折りたたむ。設定ボタンで再表示可能。エラー状態は折りたたみ後も上部に残る。
- 接続成功による折りたたみ・手動再表示の維持・エラー表示をUIテストへ追加。CTest 3/3 PASS。
- オフスクリーン描画でコメントカードを目視確認し、Plugin更新・OBS再起動後に実Dockの新しい設定画面を確認。今回のデザイン変更後には実ライブを再開していない。

## YouTube認証の自動設定

- 既存の認証ファイルを起動時に設定し、手動入力は「変更」の内側へ移動。保存対象は選択したパスのみ。
- 自動選択、指定ファイル欠落時の手動表示、前回パスの復元をUIテストで検証。CTest 3/3 PASS。
- 更新直前にOBSの送信中を確認したため、新版インストール・再起動は保留。ビルド済み。現在実行中の旧版の認証欄には既存OAuthファイルを設定済み。

## Jevキー永続化の実機確認

ユーザー承認後、OBS送信停止→Plugin更新→起動。既存YouTube OAuthが「認証を設定済み」として自動設定された。提供済みJevキーを入力・接続し、macOSキーチェーンへ保存。OBSを終了して再起動し、未接続の状態でJevキー欄が自動入力されたことを確認した。設定INIにAPIキーがないことも確認。配信は再開していない。保存・復元のUIテストを含むCTestは3/3 PASS（テストは専用メモリbackend、実キーチェーンはOBSで検証）。

## Jev prompt editor

Built editable policy dialog with save, cancel, restore defaults and 1-6000 character validation. Policy persists in settings.ini and is sent to the next evaluation batch through the worker. Protocol tests cover policy inclusion and retained choice schema; UI tests cover validation, save/reload, default reset and cancel. CTest 3/3 PASS. Installation and real OBS verification pending approval to interrupt the running stream.

Prompt editor deployment: user approved stopping the stream. OBS sending stopped, plugin installed and OBS restarted. Native dialog shows editable Japanese policy, 279/6000 count, Save/Cancel/Restore Defaults. Saved Jev key and YouTube authentication restored. Stream sending remains stopped; no custom policy live evaluation was performed.
