# Contributing to MaaAssistantArknights

1. If you have forked this repository before, please go to `Settings`, scroll down to the bottom, and click `Delete Repository`.
2. Go to [MAA main repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights) and click `Fork`, then `Create fork`.
3. Clone the `dev` branch of the repository (of your account) to local:

    ```bash
    git clone "your git repo link" -b dev
    ```

4. Configure development environment

    1. Download and install `Visual Studio 2022 Community`. Select `Desktop development with C++` and `.NET Desktop Development`.

5. Double-click to open the file `MeoAssistantArknights.sln`. Visual Studio will load the project automatically.
6. Run a build to test whether the development environment has been configured correctly. Chosse `Release` & `x64`, right-click `MeoAsstGui` to set it as the startup project, and run it. If the build is successful, the `MeoAsstGui` window will appear. You can connect to the emulator in order to confirm again that the development environment has been configured correctly.
7. Now you are free to contribute to MAA.
8. Remember to commit once you have proper number of changes. Don't forget to write a commit message.
9. After development, push your local changes to the remote (of your repository).

    ```bash
    git push origin dev
    ```

10. Go to [MAA repository](https://github.com/MaaAssistantArknights/MaaAssistantArknights) and submit a Pull Request. Wait until the administrator approves it. Please note that the changes should be based on `dev` instead of `master` branch, and should be merged to `dev` branch as well.
11. When there are changes from other contributors on original MAA repository, it is necessary for you to merge them to your forked repository.
    1. Add MAA upstream repository.

        ```bash
        git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
        ```

    2. Fetch upstream changes from the repository.

        ```bash
        git fetch upstream
        ```

    3. Merge changes.

        ```bash
        git merge
        ```

    4. Repeat step 7-10.

Note: operations regarding Git can be done by VS2022 instead of command line tools, using the Git changes tab.
