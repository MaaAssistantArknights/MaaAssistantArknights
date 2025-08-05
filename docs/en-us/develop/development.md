---
order: 1
icon: iconoir:developer
---

# Getting Started with Development

::: tip
This page mainly describes the PR workflow and MAA's file formatting requirements. If you want to learn specifically how to make changes to MAA's operational logic, please refer to the [Protocol Documentation](../protocol/)
:::

## Introduction to GitHub Pull Request Flow

### I don't know programming but just want to modify some JSON files/documents, how can I do it?

Welcome to the [Web-based PR Tutorial](./pr-tutorial.md) that anyone can understand (purely web-based on Github.com)

### I can program, but I've never used GitHub/C++/..., how do I get started?

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
   _(maadeps-download.py is located in the project root)_

    ```cmd
    python maadeps-download.py
    ```

5. Configure development environment

    - Download and install `Visual Studio 2022 Community`, selecting `Desktop development with C++` and `.NET Desktop Development` during installation.

6. Double-click `MAA.sln` to open the project in Visual Studio.
7. Configure Visual Studio settings

    - Select `RelWithDebInfo` and `x64` in the top configuration bar (Skip for Release builds or ARM platforms)
    - Right-click `MaaWpfGui` → Properties → Debug → Enable native debugging (This enables breakpoints in C++ Core)

8. Now you're ready to happily ~~mess around~~ start developing!
9. Commit regularly with meaningful messages during development  
   If you're not familiar with git usage, you might want to create a new branch for changes instead of committing directly to `dev`:

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    This keeps your changes isolated from upstream `dev` updates.

10. After development, push your local branch (e.g. `dev`) to your remote repository:

    ```bash
    git push origin dev
    ```

11. Submit a Pull Request at the [MAA main repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights). Ensure your changes are based on the `dev` branch, not `master`.
12. To sync upstream changes:

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

    4. Repeat steps 7, 8, 9, 10.

::: tip
After opening Visual Studio, Git operations can be performed using VS's built-in "Git Changes" instead of command-line tools.
:::

## MAA File Formatting Requirements

MAA uses a series of formatting tools to ensure that the code and resource files in the repository are visually unified for easy maintenance and reading.

Please ensure that it has been formatted before submission, or [enable Pre-commit Hooks for automatic formatting](#use-pre-commit-hooks-to-automatically-format-code).

The currently enabled formatting tools are as follows:

| File Type | Format Tool |
| --- | --- |
| C++ | [clang-format](https://clang.llvm.org/docs/ClangFormat.html) |
| Json/Yaml | [Prettier](https://prettier.io/) |
| Markdown | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

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

## Develop in cloud using GitHub codespace

Create GitHub codespace with pre-configured C++ dev environments:

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg?color=green)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights)

Then follow the instructions in vscode or [Linux tutorial](./linux-tutorial.md) to configure GCC 12 and the CMake project.
