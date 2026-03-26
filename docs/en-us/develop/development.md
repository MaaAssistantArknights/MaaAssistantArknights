---
order: 1
icon: 'iconoir:developer'
---
# Development Guide

::: tip
This page mainly describes the PR process and MAA's file formatting requirements. If you want to understand how to make changes to MAA's operational logic, please refer to the [Protocol Documentation](../protocol/).
:::

::: tip
You can [ask DeepWiki](https://deepwiki.com/MaaAssistantArknights/MaaAssistantArknights) to get a preliminary understanding of the overall architecture of the MAA project.
:::

## I don't know programming, I just want to modify a little JSON file/documentation, how do I do it?

Welcome to watch [A GitHub Pull Request Guide Even an Ox Can Understand](./pr-tutorial.md) (Pure web-based operation on GitHub.com)

## I just want to modify a few lines of code, but configuring the environment is too troublesome, and pure web editing is difficult to use. What should I do?

Please use [GitHub Codespaces](https://github.com/codespaces) online development environment and try it out!

We have pre-configured several different development environments for you to choose from:

- Blank environment, bare Linux container (default)

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2Fdevcontainer.json)

- Lightweight environment, suitable for documentation site frontend development

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F0%2Fdevcontainer.json)

- Full environment, suitable for MAA Core related development (Not recommended, suggest local development with a complete environment configuration. See the next chapter for details)

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F1%2Fdevcontainer.json)

## Complete Environment Configuration Process (Windows)

1. If you forked a long time ago, first go to `Settings` in your own repository, scroll to the bottom, and delete it.
2. Open the [MAA Main Repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights), click `Fork`, then click `Create fork`.
3. Clone the dev branch from your own repository locally, and pull the submodules.

   ```bash
   git clone --recurse-submodules <Your repository's git link> -b dev-v2 --single-branch
   ```

   ::: tip
   `--single-branch` will only fetch the commit history of `dev-v2`. If you later want to switch to other branches, you need to first execute `git remote set-branches origin '*'` and pull again to complete the information of other branches; or re-clone a repository without `--single-branch`.
   :::

   ::: warning
   If you are using a Git GUI like Visual Studio that does not include the `--recurse-submodules` parameter, you need to execute `git submodule update --init` after cloning to pull the submodules.
   :::

4. Download pre-built third-party libraries

   **Requires a Python environment, please search for Python installation tutorials yourself.**

   ```cmd
   python tools/maadeps-download.py
   ```

5. Configure the programming environment
   - Download and install `CMake`
   - Download and install `Visual Studio 2026 Community`. During installation, you need to select `Desktop development with C++` and `.NET desktop development`.

6. Execute cmake project configuration

   ```cmd
   cmake --preset windows-x64
   ```

7. Double-click to open the `build/MAA.slnx` file. Visual Studio will automatically load the entire project.
8. Set up VS
   - At the top of VS, select `Debug` `x64`
   - Right-click `MaaWpfGui` - Set as Startup Project
   - Press F5 to run

   ::: tip
   If you need to debug Win32Controller (Windows window control) related functions, you need to download the corresponding platform's compressed package from [MaaFramework Releases](https://github.com/MaaXYZ/MaaFramework/releases) yourself, and place the `MaaWin32ControlUnit.dll` from the `bin` directory into the same directory as MAA's DLLs (e.g., `build/bin/Debug`). Welcome to PR an automatic download script!
   :::

9. At this point, you can happily ~~mess around~~ contribute.
10. During development, remember to commit a Commit every so often, and don't forget to write a Message.
    If you are not familiar with git usage, you may want to create a new branch for your changes instead of committing directly on `dev-v2`.

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    This way, your commits will grow on the new branch and won't be disturbed by updates to `dev-v2`.

11. After completing development, push your modified local branch (using `dev-v2` as an example) to the remote (your forked repository).

    ```bash
    git push origin dev-v2
    ```

12. Open the [MAA Main Repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights). Submit a Pull Request and wait for administrator approval. Don't forget you modified on the dev branch, don't submit to the master branch.
13. When changes appear in the original MAA repository (made by others), you may need to synchronize these changes to your branch.
    1. Link the original MAA repository.

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. Pull updates from the original MAA repository.

       ```bash
       git fetch upstream
       ```

    3. Rebase (recommended) or merge the changes.

       ```bash
       git rebase upstream/dev-v2 # Rebase
       ```

       or

       ```bash
       git merge # Merge
       ```

    4. Repeat steps 8, 9, 10, and 11 above.

::: tip
After opening VS, Git-related operations can be done without the command line tool; you can directly use VS's built-in "Git Changes".
:::

## Using VS Code for Development (Optional)

::: warning
**It is recommended to prioritize using Visual Studio for development.** The MAA project is primarily built based on Visual Studio. The complete environment configuration process above already covers all development needs, offering the best out-of-the-box experience. The VSCode solution is only an alternative, suitable for developers already familiar with the VS Code + CMake + clangd workflow, with a relatively higher configuration threshold.
:::

If you prefer using VSCode, you can pair it with extensions like CMake and clangd to gain code completion, navigation, and debugging capabilities. After completing the aforementioned steps 1–6 (clone, dependencies, CMake configuration), you can configure as follows:

### Recommended Extensions

Install from the VS Code extension marketplace:

| Extension                                                                                                                              | Purpose                                                                 |
| -------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)                                               | CMake configuration, build, debug integration                           |
| [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)                                    | C++ intelligent suggestions, code navigation, diagnostics (based on LSP) |
| [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)                                                        | Debug C++ programs (works with CMake Tools or launch.json)              |

::: tip
When using clangd, it's recommended to disable the C/C++ extension's IntelliSense (set `C_Cpp.intelliSenseEngine` to `disabled`) to avoid conflicts with clangd.
:::

### Configuration Steps

1. Open the project root directory with VS Code.
2. Use **CMake Tools**:
   - Select Configure Preset from the status bar (e.g., `windows-x64`, `linux-x64`, etc.)
   - Select Build Preset, execute configuration and build.
3. Use **clangd**: On Linux/macOS, the preset already has `CMAKE_EXPORT_COMPILE_COMMANDS` enabled, and clangd will automatically use `build/compile_commands.json`. On Windows, if you need clangd's completion and navigation, you first need to generate `compile_commands.json`:

   ::: warning Windows clangd configuration instructions
   - In the VS Installer, check to install **C++ Clang compiler for Windows** (clang-cl).
   - You need to switch to `windows-x64-clang` and execute Configure once to generate `compile_commands.json` under `build/`. After that, clangd can use it.
   - **This preset uses clang-cl instead of MSVC and cannot directly compile usable output.** For actual building, you must switch back to `windows-x64`.
   - clangd analyzes based on clang-cl's compilation information. Some code (like MSVC-specific extensions) may still show errors, which can be ignored and do not affect actual MSVC compilation.

   **Command line example for switching presets** (execute in the project root directory):

   ```cmd
   rem Generate compile_commands.json (Configure only, no build)
   cmake --preset windows-x64-clang

   rem Switch back to MSVC for actual building
   cmake --preset windows-x64
   cmake --build --preset windows-x64-RelWithDebInfo
   ```

   :::

4. **Debugging**: The project already includes `.vscode/launch.json`. You can directly launch MaaWpfGui or Debug Demo for debugging.

### Quick Build and Debug

- **Build**: `Ctrl+Shift+B` or via the CMake Tools status bar.
- **Debug**: F5 or select the corresponding configuration in the Run and Debug panel.

## MAA's File Formatting Requirements

MAA uses a series of formatting tools to ensure code and resource files in the repository are aesthetically unified, making them easy to maintain and read.

Please ensure formatting is done before submission, or [enable Pre-commit Hooks for automatic formatting](#utilizing-pre-commit-hooks-for-automatic-code-formatting).

Currently enabled formatting tools are as follows:

| File Type | Formatting Tool                                                       |
| --------- | --------------------------------------------------------------------- |
| C++       | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)          |
| JSON/YAML | [Prettier](https://prettier.io/)                                      |
| Markdown  | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2)       |

### Utilizing Pre-commit Hooks for Automatic Code Formatting

1. Ensure your computer has Python and Node environments.

2. Execute the following command in the project root directory.

   ```bash
   pip install pre-commit
   pre-commit install
   ```

If Pre-commit still cannot run after pip installation, please confirm the PIP installation location has been added to PATH.

From now on, formatting tools will automatically run on each commit to ensure your code format complies with the standards.

### Enabling clang-format in Visual Studio

1. Install clang-format version 20.1.0 or higher.

   ```bash
   python -m pip install clang-format
   ```

2. Use tools like Everything to find the installation location of clang-format.exe. For reference, if you use Anaconda, clang-format.exe will be installed at YourAnacondaPath/Scripts/clang-format.exe.

3. Search for `clang-format` in Visual Studio `Tools-Options`.
4. Click `Enable ClangFormat support`, then select `Use custom clang-format.exe file` below, and choose the `clang-format.exe` found in step 2.

![Visual Studio Setting clang-format](/images/zh-cn/development-enable-vs-clang-format.png)

Then your Visual Studio can happily use clang-format that supports C++20 syntax!

You can also use `tools\ClangFormatter\clang-formatter.py` to directly call your clang-format for formatting. Just execute in the project root directory:

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`
