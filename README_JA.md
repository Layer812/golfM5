# BALL'S EYE GOLF R13 FIX3 — Windows向けソース・書込みガイド

**2026-11-10 実機画面表示成功版**。9ホール、ドラコン、物理演算、スコアカード、AUTO CUP、打球音のゲーム本体はR12 FIX4から維持しています。

## 必要な環境

- M5Stack StopWatch（ESP32-S3、16MB Flash、8MB OPI PSRAM）
- Windows + PlatformIO CLI (`pio`) + Python / esptool
- `platformio.ini` 固定の `espressif32 @ 6.12.0`、M5Unified `0.2.21`、M5GFX `0.2.28`
- ボード定義 `boards/golf-stopwatch.json` の `qio_opi` は変更しないでください

## ビルド

クローンしたリポジトリのルートで次を実行します。

```bat
RUN_NATIVE_BUILD_ONLY.cmd
```

内部では `pio run -e golf_native` を実行します。生成ファイル：`.pio\build\golf_native\firmware.bin`。

## APP-ONLY書込み

1. StopWatchをESP32-S3 ROM DOWNLOADモードにする。
2. USBシリアルモニター等を閉じ、実際のCOM番号を確認する。
3. リポジトリのルートで下記を実行する（COM13は例）。

```bat
RUN_NATIVE_BUILD_FLASH_APP_ONLY.cmd COM13
```

スクリプトは実機から情報を読んでESP32-S3、Flash 16MB、セキュリティ状態、パーティション表、起動先APP、イメージヘッダーと容量を確認します。**APP領域だけ**を書き換え、bootloader・partition・NVS・otadata・SPIFFS等は変更しません。ROM接続ができない場合は安全停止します。

実機は `app0` というラベルでも `type=0/subtype=0x00` のfactory APPである場合があります。factory APPのみの構成では未使用のotadataに依存せずfactoryを選択します。OTAアプリが存在する構成ではotadataにより起動先を判定し、確認できなければ停止します。**実機のAPPオフセットを手入力で決め打ちしません。**

## ゲーム内容と操作

- 9ホール・トーナメント、スコアカード、ドラコン（3球）、池・谷・バンカー、カップ・復帰の演出
- 左ボタン：メニュー、右ボタン：ショット、左右矢印：照準、IMPACTドラッグ：打点
- `AUTO CUP ON`：カップ方向へ自動追従。`OFF`：視点を保持し、方向は手動操作
- ドライバー／スプーン／パターのPCM打球音、サウンド・振動の設定

## 検証状況

- **確認済み**：Windows PlatformIOでR13系列のビルド成功、golfM5 R13 FIX3の実機画面表示成功（ユーザー報告）。
- **未断定**：他のStopWatch実機や別のライブラリ世代での再現性。
- 過去のR12版はGitHubの履歴から参照できます。最新のビルド／書込み用CMDを使用してください。
