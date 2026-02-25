---
order: 1
icon: iconoir:developer
---

# Development Guide

::: tip
This page mainly describes the PR workflow and MAA's file formatting requirements. If you want to learn specifically how to make changes to MAA's operational logic, please refer to the [Protocol Documentation](../protocol/)
:::

::: tip
You can [Ask DeepWiki](https://deepwiki.com/MaaAssistantArknights/MaaAssistantArknights) to learn about the overall architecture of the MAA project.
:::

## I don't know programming but just want to modify some JSON files/documents, how can I do it?

Welcome to the [Web-based PR Tutorial](./pr-tutorial.md) that anyone can understand (purely web-based on GitHub.com)

## I want to make simple modifications to a few lines of code, but configuring the environment is too tedious and pure web editing is difficult to use. What should I do?

Use the [GitHub Codespaces](https://github.com/codespaces) online development environment and try it out!

We've preset several different development environments for you to choose from:

- Blank environment with a bare Linux container (default)

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2Fdevcontainer.json)

- Lightweight environment, suitable for documentation site frontend development

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F0%2Fdevcontainer.json)

- Full environment, suitable for MAA Core related development (not recommended, suggest local development with full environment setup. See next section)

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F1%2Fdevcontainer.json)

## Complete Environment Setup (Windows)

1. If you forked the repository long ago, first delete it via your repository's `Settings` at the bottom.
2. Visit the [MAA main repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights), click `Fork`, then `Create fork`.
3. Clone the dev branch of your repository with submodules:

   ```bash
   git clone --recurse-submodules <your repository link> -b dev
   ```

   ::: warning
   If using Git GUI clients like Visual Studio without `--recurse-submodules` support, run `git submodule update --init` after cloning to initialize submodules.
   :::

4. Download prebuilt third-party libraries  
   **Python environment required - search for Python installation tutorials if needed**  
   _(tools/maadeps-download.py is located in the project root)_

   ```cmd
   python tools/maadeps-download.py
   ```

5. Configure development environment
   - Download and install `CMake`
   - Download and install `Visual Studio 2026 Community`, selecting `Desktop development with C++` and `.NET Desktop Development` during installation.

6. Execute cmake project configuration

   ```cmd
   cmake --preset windows-x64
   ```

7. Double-click `build/MAA.slnx` to open the project in Visual Studio.
8. Configure Visual Studio settings
   - Select `Debug` and `x64` in the top configuration bar
   - Right-click `MaaWpfGui` - Set as Startup Project
   - Press F5 to run

   ::: tip
   To debug Win32Controller (Windows window control) features, you need to manually download the corresponding platform package from [MaaFramework Releases](https://github.com/MaaXYZ/MaaFramework/releases), and place `MaaWin32ControlUnit.dll` from the `bin` directory into MAA's DLL directory (e.g. `build/bin/Debug`). PRs for an auto-download script are welcome!
   :::

9. Now you're ready to happily ~~mess around~~ start developing!
10. Commit regularly with meaningful messages during development  
    If you're not familiar with git usage, you might want to create a new branch for changes instead of committing directly to `dev`:

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    This keeps your changes isolated from upstream `dev` updates.

11. After development, push your local branch (e.g. `dev`) to your remote repository:

    ```bash
    git push origin dev
    ```

12. Submit a Pull Request at the [MAA main repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights). Ensure your changes are based on the `dev` branch, not `master`.
13. To sync upstream changes:
    1. Add upstream repository:

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. Fetch updates:

       ```bash
       git fetch upstream
       ```

    3. Rebase (recommended) or merge:

       ```bash
       git rebase upstream/dev # rebase
       ```

       or

       ```bash
       git merge # merge
       ```

    4. Repeat steps 8, 9, 10, 11.

::: tip
After opening Visual Studio, Git operations can be performed using VS's built-in "Git Changes" instead of command-line tools.
:::

## Using VS Code for Development (Optional)

::: warning
**Visual Studio is the recommended IDE for development.** The MAA project is primarily built around Visual Studio, and the complete environment setup described above covers all development needs with the best out-of-the-box experience. The VS Code workflow is provided only as an alternative for developers already familiar with VS Code + CMake + clangd, and requires more configuration effort.
:::

If you prefer VS Code, you can use CMake, clangd, and related extensions for code completion, navigation, and debugging. After completing steps 1–6 above (clone, dependencies, CMake configuration), follow these steps:

### Recommended Extensions

Install from the VS Code marketplace:

| Extension                                                                                           | Purpose                                                    |
| --------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- |
| [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)            | CMake configure, build, and debug integration              |
| [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) | C++ IntelliSense, code navigation, diagnostics (LSP-based) |
| [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)                     | Debug C++ programs (works with CMake Tools or launch.json) |

::: tip
When using clangd, set `C_Cpp.intelliSenseEngine` to `disabled` to avoid conflicts with the C/C++ extension's IntelliSense.
:::

### Setup Steps

1. Open the project root folder in VS Code
2. **CMake Tools**:
   - Select a Configure Preset from the status bar (e.g. `windows-x64`, `linux-x64`)
   - Select a Build Preset and run configure/build
3. **clangd**: On Linux/macOS, presets enable `CMAKE_EXPORT_COMPILE_COMMANDS` and clangd uses `build/compile_commands.json` automatically. On Windows, clangd's code completion and navigation requires generating `compile_commands.json` first:

   ::: warning clangd Setup on Windows
   - In **VS Installer**, check **C++ Clang compiler for Windows** (clang-cl)
   - Switch to the `windows-x64-clang` preset and run Configure once to generate `compile_commands.json` in `build/`, then clangd will work
   - **This preset uses clang-cl instead of MSVC and cannot produce valid build artifacts**; switch back to `windows-x64` for actual builds
   - clangd analyzes based on clang-cl; some code (e.g. MSVC-specific extensions) may show errors. These can be ignored and do not affect MSVC compilation

   **Command-line preset switching example** (run from project root):

   ```cmd
   rem Generate compile_commands.json (Configure only, no build)
   cmake --preset windows-x64-clang

   rem Switch back to MSVC for actual build
   cmake --preset windows-x64
   cmake --build --preset windows-x64-RelWithDebInfo
   ```

   :::

4. **Debugging**: The project includes `.vscode/launch.json` for launching MaaWpfGui or Debug Demo

### Build and Debug Shortcuts

- **Build**: `Ctrl+Shift+B` or via CMake Tools status bar
- **Debug**: F5 or choose a configuration from the Run and Debug panel

## MAA File Formatting Requirements

MAA uses a series of formatting tools to ensure that the code and resource files in the repository are visually unified for easy maintenance and reading.

Please ensure that it has been formatted before submission, or [enable Pre-commit Hooks for automatic formatting](#use-pre-commit-hooks-to-automatically-format-code).

The currently enabled formatting tools are as follows:

| File Type | Format Tool                                                     |
| --------- | --------------------------------------------------------------- |
| C++       | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)    |
| JSON/YAML | [Prettier](https://prettier.io/)                                |
| Markdown  | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

### Use Pre-commit Hooks to Automatically Format Code

1. Ensure that you have Python and Node environments on your computer.

2. Execute the following command in the root directory of the project:

   ```bash
   pip install pre-commit
   pre-commit install
   ```

If pre-commit still cannot be used after pip install, please check if the pip installation path has been added to the PATH.

The formatting tool will automatically run every time you submit to ensure that your code format conforms to the style guide.

### Enable clang-format in Visual Studio

1. Install clang-format version 20.1.0 or higher.

   ```bash
   python -m pip install clang-format
   ```

2. Use tools like 'Everything' to locate the installation location of clang-format.exe. As a reference, if you are using Anaconda, clang-format.exe will be installed in YourAnacondaPath/Scripts/clang-format.exe.
3. In Visual Studio, search for 'clang-format' in Tools-Options.
4. Click `Enable ClangFormat support` and select `Use custom clang-format.exe file` and choose the `clang-format.exe` located in Step 2.

![Enable clang-format in Visual Studio](/images/zh-cn/development-enable-vs-clang-format.png)

You are all set with the clang-format integrated in Visual Studio supporting c++20 features!

You can also format with `tools\ClangFormatter\clang-formatter.py` directly, by executing in the root folder of the project:

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`
