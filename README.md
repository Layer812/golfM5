# BALL’S EYE GOLF — golfM5

M5Stack **StopWatch**（ESP32-S3 / 円形AMOLED）用、一人称視点ゴルフゲームです。**ボール自身の視点**で9つのコースを回ります。

<img src="66e16480-e4b8-441a-9c59-ccfb17013c2e.png" width="240" alt="BALL'S EYE GOLF screenshot"> <img src="622858ee-cdf8-4766-8c43-83b52ee1f9a4.png" width="240" alt="BALL'S EYE GOLF screenshot">

*画像は開発中のイメージであり、実際の画面とは異なる場合があります。*

## インストール

- [M5Stack WebBurner](https://burner.m5stack.com/WQ7ULB)
- M5Burnerのシェアコード：`K6QLhtHT8JSs3j9S`

※ WebBurner掲載版のバージョンと、このGitHub上の最新ソース（**R13 FIX3**）が一致するとは限りません。

## ゲームの特徴

- **9ホール・トーナメント**：ホールごとの地形、フェアウェイ、樹木、バンカー、池、谷、ドッグレッグ
- **92〜480mのコース**、スコアカード（PAR、打数、ホール差分、通算成績）
- **ドラコンモード**：3球で最長飛距離に挑戦
- 風・バウンド・転がり、**打点の左右調整**による曲がり
- 水や谷に落ちたときの一人称演出とペナルティ復帰
- ドライバー／スプーン／パターごとの打球音
- **AUTO CUP ON/OFF**：カップへの自動方向合わせの切替

## 操作

| 操作 | 内容 |
| --- | --- |
| 右物理ボタン | ショット |
| 左物理ボタン | 設定メニュー |
| 画面左右の矢印 | 照準の左右調整 |
| IMPACT円のドラッグ | 左右の打点変更 |
| ホール終了後のタップ／物理ボタン | 次ホール／新ラウンド |

メニューには **TOURNAMENT 9H / DRACON 3 BALLS / AIM STEP / AUTO CUP / SOUND / VIBRATION** を用意しています。

## ソースコードとビルド

現在のソースは、**2026年11月10日に実機で画面表示成功が報告されたR13 FIX3**です。

PlatformIO CLI をインストールし、Windowsのコマンドプロンプトでリポジトリのルートから実行してください。

```bat
RUN_NATIVE_BUILD_ONLY.cmd
```

書込みは、StopWatchを **ESP32-S3 ROM DOWNLOADモード**にしてから、実際のCOM番号を指定します。

```bat
RUN_NATIVE_BUILD_FLASH_APP_ONLY.cmd COM13
```

**COM13は例です。** 実際のポートに置き換えてください。書込み対象APPはスクリプトが実機のパーティション表を読み取り、起動APPを照合して決めます。安全確認に失敗した場合は停止します。Bootloader、NVS、ファイルシステム、otadata、パーティション表は更新しません。

詳細は [日本語のビルド・書込みガイド](README_JA.md) を参照してください。

## 主なファイル

- `src/golf_game.cpp` — ゲーム描画、効果音、UI
- `src/GamePhysics.hpp` / `src/GameControls.hpp` — 物理・操作
- `src/main.cpp` — アプリ起動処理
- `platformio.ini` / `boards/golf-stopwatch.json` — StopWatchのビルド設定
- `flash_app_only.py` — 実機確認付きAPP-ONLY書込み
- `TEST_FACTORY_*.py` / `tests/` — ホストテスト
- `WAV_PREVIEW/` — 打球音の試聴データ

## 開発環境と依存ライブラリ

ESP32-S3 / Flash 16MB / OPI PSRAM 8MB、PlatformIO `espressif32 @ 6.12.0`、[M5Unified 0.2.21](https://github.com/m5stack/M5Unified)、[M5GFX 0.2.28](https://github.com/m5stack/M5GFX) を使用しています。依存ライブラリ・SDKはそれぞれのライセンスに従ってください。ゲーム固有のソースコードの再利用ライセンスは別途指定予定です。
