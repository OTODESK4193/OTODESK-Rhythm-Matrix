
# OTODESK Rhythm Matrix 🥁

**OTODESK Rhythm Matrix** is a powerful, algorithmic drum machine plugin (VST3 / Standalone) built with C++ and the JUCE framework. It generates complex polyrhythms, Euclidean rhythms, and highly musical patterns across 26 different genres using advanced probability and anchor/sub-anchor logic.

OTODESK Rhythm Matrix は、C++とJUCEフレームワークで開発された、アルゴリズミック・ドラムマシン・プラグインです。確率論とアンカーロジックを用いて、複雑なポリリズムやユークリッドリズム、26種類のジャンルに特化した音楽的なビートを自動生成します。

## ✨ Features (主な機能)
* **26 Genre Algorithms:** Instantly generate patterns for UK Drill, Breakcore, Amapiano, Techno, Math Rock, and more.
* **Polyrhythm & Euclidean Matrix:** Mathematically perfect beat distribution with humanized ghost notes.
* **Complexity & Entropy Controls:** Dynamically adjust the density and randomness of each track.
* **Drag & Drop Samples:** Easily load your own `.wav`, `.mp3`, or `.aif` samples into any of the 8 tracks.
* **MIDI Drag & Drop Export:** Drag patterns directly from the UI into your DAW's timeline.
* **Lightweight UI:** Optimized rendering for smooth performance even with multiple instances.

## 📥 Installation (インストール方法)
Download the latest `.zip` from the [Releases](../../releases) page and extract the `.vst3` file to your plugin folder.

最新のReleaseページからZIPファイルをダウンロードし、以下のフォルダに `.vst3` ファイルを配置してください。

* **Windows:** `C:\Program Files\Common Files\VST3`
* **macOS:** `/Library/Audio/Plug-Ins/VST3`

*(A Standalone executable is also available for jamming without a DAW. / DAWなしで遊べるスタンドアロン版も同梱しています。)*

## 🛠 Building from Source (ソースからのビルド方法)
If you want to build the plugin yourself, you will need **CMake (>= 3.22)** and a **C++20** compatible compiler.

1. Clone this repository:
   ```bash
   git clone [https://github.com/YourUsername/OTODESK-Rhythm-Matrix.git](https://github.com/YourUsername/OTODESK-Rhythm-Matrix.git)
````

2.  **Important:** Open `CMakeLists.txt` and change the `JUCE_DIR` path to where JUCE is installed on your system.
    *(CMakeLists.txt を開き、`JUCE_DIR` をご自身のJUCEインストールパスに変更してください)*
3.  Build the project:
    ```bash
    cmake -B build
    cmake --build build --config Release
    ```

## ⚖️ License (ライセンスと規約)

This project is open-source and released under the **[GPLv3 License](https://www.google.com/search?q=LICENSE)**.
You are free to use, modify, and distribute this software, but any derivative works must also be open-source and licensed under GPLv3.

このプロジェクトは **GPLv3 ライセンス** の下で公開されています。無料で使用・改変・再配布が可能ですが、派生物も同じくGPLv3でオープンソース化する必要があります。

  * Built with [JUCE](https://juce.com/). JUCE is a framework for multi-platform audio applications.
  * To release a closed-source commercial product based on this code, you must acquire an appropriate commercial license from JUCE.

## 🤝 Credits

Developed by OTODESK.
