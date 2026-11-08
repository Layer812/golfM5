# BALL'S EYE GOLF — golfM5

M5Stack StopWatch（ESP32-S3R8 / 468×468 AMOLED）向け、一人称視点のゴルフゲーム。

> 開発版：R12 FIX4（AUTO CUP ON/OFF）。実機で表示確認済みの起動経路は、[MAPLE0212/m5stack-stopwatch-platformio](https://github.com/MAPLE0212/m5stack-stopwatch-platformio) の `00_smoke` を基準としています。R12 FIX4自体の実機動作は未確認です。

## ゲーム内容

- 9ホールのトーナメント：各ホール固有のフェアウェイ、池、バンカー、樹木、ドッグレッグなど
- 92mから480mまでのコース、9ホール終了時のスコアカード（PAR、打数、差分、通算）
- ドラコン（3球の最長飛距離）
- ボールから見た一人称視点、風・着地・転がり、IMPACTによる左右の打球カーブ
- 池や谷への落下時はボール視点の演出後、直前の打球位置へ戻り2打罰で再開
- ドライバー・スプーン・パターごとの短いPCM打球音
- カップ方向への自動旋回（AUTO CUP ON/OFF）

## 操作

| 操作 | 内容 |
|---|---|
| 右物理ボタン | ショット |
| 左物理ボタン | 設定メニューを開閉 |
| 画面左右の矢印 | 左右の照準調整 |
| IMPACT円のドラッグ | 左右の打点変更 |
| ホール終了時のTAP／左右ボタン | 次ホールへ／新ラウンド |

設定メニュー：TOURNAMENT 9H、DRACON 3 BALLS、AIM STEP、AUTO CUP、SOUND、VIBRATION。

## ビルドと書込み

アプリは、上記GitHubサンプルの **固定コミット `f2549b0bfba6f5cfc201f51b975fa2b134a2d3a5`** を別フォルダに展開し、元の `firmware/shared/board.cpp` **82行目** `M5.Display.drawString(title, w / 2, 66);` の直後にゴルフ開始呼び出しを追加する方式です。オリジナルの `platformio.ini`、ボード定義、既存初期化処理は変更しません。

Windows / PlatformIOでCOM16のROMダウンロードモードに書き込む場合、通常の全領域書込みは禁止です。実機からパーティション表と起動OTA先を読み取り専用で照合し、選択されたAPP領域だけを更新します。Bootloader、otadata、NVS、storageは書き換えません。

**R12 FIX4のゲームソースと書込みスクリプトは、このリポジトリに登録済みです。** 元GitHubサンプルの固定コミットを取得して、画面表示成功済みの82行目直後へゴルフを接続します。

Windows上でリポジトリをcloneし、StopWatchをROMダウンロードモードにした後、実機のCOMポートを指定して実行してください。

```bat
RUN_ORIGINAL_SAMPLE_THEN_GOLF_COM16.cmd COM16
```

実機のパーティション・起動先が確認できないときは書込みを中止します。

## 謝辞・ライセンス

- [M5Stack / M5Unified](https://github.com/m5stack/M5Unified)
- [M5Stack / M5GFX](https://github.com/m5stack/M5GFX)
- [MAPLE0212 / m5stack-stopwatch-platformio](https://github.com/MAPLE0212/m5stack-stopwatch-platformio) — 起動・表示の原型となったStopWatchサンプル

元サンプルと各依存ライブラリのライセンスに従ってください。ゴルフゲーム固有コードのライセンスは別途明記します。
