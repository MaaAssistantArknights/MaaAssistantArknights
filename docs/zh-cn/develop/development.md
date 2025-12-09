---
order: 1
icon: iconoir:developer
---

# 开发指南

::: tip
本页面主要描述了 PR 流程以及 MAA 的文件格式化要求，如果你想要具体了解如何对 MAA 的运行逻辑做出更改，请参看 [协议文档](../protocol/)
:::

::: tip
你可以 [向 DeepWiki 询问](https://deepwiki.com/MaaAssistantArknights/MaaAssistantArknights)，以初步了解 MAA 项目的总体架构。
:::

## 我不懂编程，只是想改一点点 JSON 文件/文档等，要怎么操作？

欢迎收看 [牛牛也能看懂的 GitHub Pull Request 使用指南](./pr-tutorial.md) （纯网页端操作 Github.com）

## 我只想简单修改几行代码，但配置环境太麻烦，纯网页编辑又很难用，怎么办？

请使用 [GitHub Codespaces](https://github.com/codespaces) 在线开发环境，尽情尝试！

我们预置了多种不同的开发环境以供选择：

- 空白环境，裸 Linux 容器（默认）

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2Fdevcontainer.json)

- 轻量环境，适合文档站前端开发

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F0%2Fdevcontainer.json)

- 全量环境，适合 MAA Core 相关开发（不推荐使用，建议本地开发，完整配置相关环境。详见下一章节）

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F1%2Fdevcontainer.json)

## 完整环境配置流程（Windows）

1. 如果很久以前 Fork 过，先在自己仓库的 `Settings` 里，翻到最下面，删除
2. 打开 [MAA 主仓库](https://github.com/MaaAssistantArknights/MaaAssistantArknights)，点击 `Fork`，继续点击 `Create fork`
3. 克隆你自己仓库下的 dev 分支到本地，并拉取子模块

   ```bash
   git clone --recurse-submodules <你的仓库的 git 链接> -b dev
   ```

   ::: warning
   如果正在使用 Visual Studio 等不附带 `--recurse-submodules` 参数的 Git GUI，则需在克隆后再执行 `git submodule update --init` 以拉取子模块。
   :::

4. 下载预构建的第三方库

   **需要有 Python 环境，请自行搜索 Python 安装教程**

   ```cmd
   python tools/maadeps-download.py
   ```

5. 配置编程环境
   - 下载并安装 `CMake`
   - 下载并安装 `Visual Studio 2026 Community`, 安装的时候需要选中 `基于 C++ 的桌面开发` 和 `.NET 桌面开发`。

6. 执行 cmake 项目配置

   ```cmd
   mkdir -p build
   cmake -G "Visual Studio 18 2026" -B build -DBUILD_WPF_GUI=ON -DBUILD_DEBUG_DEMO=ON
   ```

7. 双击打开 `build/MAA.sln` 文件，Visual Studio 会自动加载整个项目。
8. 设置 VS
   - VS 上方配置选择 `Debug` `x64`
   - 右键 `MaaWpfGui` - 设为启动项目
   - 按 F5 运行

9. 到这里，你就可以愉快地 ~~瞎 JB 改~~ 发电了
10. 开发过程中，每一定数量，记得提交一个 Commit, 别忘了写上 Message  
    假如你不熟悉 git 的使用，你可能想要新建一个分支进行更改，而不是直接提交在 `dev` 上

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    这样你的提交就能在新的分支上生长，不会受到 `dev` 更新的打扰

11. 完成开发后，推送你修改过的本地分支（以 `dev` 为例）到远程（Fork 的仓库）

    ```bash
    git push origin dev
    ```

12. 打开 [MAA 主仓库](https://github.com/MaaAssistantArknights/MaaAssistantArknights)。提交一个 Pull Request，等待管理员通过。别忘了你是在 dev 分支上修改，别提交到 master 分支去了
13. 当 MAA 原仓库出现更改（别人做的），你可能需要把这些更改同步到你的分支
    1. 关联 MAA 原仓库

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. 从 MAA 原仓库拉取更新

       ```bash
       git fetch upstream
       ```

    3. 变基（推荐）或者合并修改

       ```bash
       git rebase upstream/dev # 变基
       ```

       或者

       ```bash
       git merge # 合并
       ```

    4. 重复上述 7, 8, 9, 10 中的操作

::: tip
在打开 VS 之后，和 Git 有关的操作可以不用命令行工具，直接使用 VS 自带的“Git 更改”即可
:::

## MAA 的文件格式化要求

MAA 使用一系列的格式化工具来保证仓库中的代码和资源文件美观统一，以便于维护和阅读

请确保在提交之前已经格式化，或是[启用 Pre-commit Hooks 来进行自动格式化](#利用-pre-commit-hooks-自动进行代码格式化)

目前启用的格式化工具如下：

| 文件类型  | 格式化工具                                                      |
| --------- | --------------------------------------------------------------- |
| C++       | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)    |
| Json/Yaml | [Prettier](https://prettier.io/)                                |
| Markdown  | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

### 利用 Pre-commit Hooks 自动进行代码格式化

1. 确保你的电脑上有 Python 与 Node 环境

2. 在项目根目录下执行以下命令

   ```bash
   pip install pre-commit
   pre-commit install
   ```

如果pip安装后依然无法运行 Pre-commit，请确认 PIP 安装地址已被添加到 PATH

接下来，每次提交时都将会自动运行格式化工具，来确保你的代码格式符合规范

### 在 Visual Studio 中启用 clang-format

1. 安装 clang-format 20.1.0 或更高版本

   ```bash
   python -m pip install clang-format
   ```

2. 使用 Everything 等工具 找到 clang-format.exe 的安装位置。作为参考，若您使用了 Anaconda，clang-format.exe 将安装在 YourAnacondaPath/Scripts/clang-format.exe

3. 在 Visual Studio `工具-选项` 中搜索 `clang-format`
4. 点击 `启用 ClangFormat 支持`，然后选择下面的 `使用自定义 clang-format.exe 文件`，选择第 2 步找到的 `clang-format.exe`

![Visual Studio 设置 clang-format](/images/zh-cn/development-enable-vs-clang-format.png)

然后你的 Visual Studio 就能愉快的使用支持 C++20 语法的 clang-format 啦！

你也可以使用 `tools\ClangFormatter\clang-formatter.py` 来直接调用你的 clang-format 来进行格式化，只需要在项目根目录下执行：

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`
