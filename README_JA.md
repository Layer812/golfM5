# BALL’S EYE GOLF R12 FIX4 — AUTO CUP AIM ON/OFF

R12 FIX3（9ホール成績表）を元に、左ボタンの設定メニューへ `AUTO CUP ON/OFF` を追加。

- `AUTO CUP ON`（初期値）: 従来どおり、ボール移動に合わせて常にカップ方向を基準として視点・打球方向を更新します。左右矢印で微調整できます。
- `AUTO CUP OFF`: その時点の視点方向を保持し、打球・飛行・転がり・池からの復帰で自動追従しません。左右矢印では手動で方向を調整できます。次ホールはフェアウェイ正面（+Z方向）で開始します。
- 設定をONへ戻すと、その時点でカップへ正対します。設定は次ホールや新ラウンドでも維持します（電源断後の保存はしません）。

左ボタン＝メニュー、右ボタン＝ショット、従来の池・バンカー・9ホール・成績表・音声などは維持。

## 変更ファイル
- `golf_game.cpp`：メニューにAUTO CUP追加、タッチ対応、ホール変更時の反映。
- `GamePhysics.hpp`：AUTO中のみ毎回カップ方向を向き、OFFは世界座標で向きを保持。
- 追加の説明・差分・ホストテストファイルのみ。

**保護**：`stage_original_hook.py`、`RUN_ORIGINAL_SAMPLE_THEN_GOLF_COM16.cmd`、`flash_app_only.py`、`GameControls.hpp`、WAV音源、元GitHubサンプル `board.cpp` 82行目まで、およびライブラリ設定はR12 FIX3からバイト単位で不変。表示成功済み経路に変更はありません。

## 実行
ZIPを `F:\build\tmp` に展開し、COM16 ROMダウンロードモードで次のコマンドを1行だけ実行します。

```bat
F:\build\tmp\BALLSEYE_R12_FIX4_AUTO_CUP_AIM_COM16\RUN_ORIGINAL_SAMPLE_THEN_GOLF_COM16.cmd COM16
```

アプリだけ書込み、他パーティションは変更しません。

## 検証範囲
C++11ホストテスト（自動追従・方向保持・左右調整・新ホール・池の2打罰復帰）、ゲームソースの構文・保護ファイルのSHA-256一致を確認。Windows PlatformIO実ビルド・実機表示は未確認。
