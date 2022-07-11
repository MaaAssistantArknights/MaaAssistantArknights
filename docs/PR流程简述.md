# Github Pull Request 流程简述

1. 如果很久以前 fork 过，先在自己仓库的 `Settings` 里，翻到最下面，删除
2. 打开 [MAA 主仓库](https://github.com/MaaAssistantArknights/MaaAssistantArknights)，点击 `Fork`，继续点击 `Create fork`
3. clone 仓库（你自己账号下） dev 分支到本地

    ```bash
    git clone <你的仓库 git 链接> -b dev
    ```

4. 配置编程环境

    1. 下载并安装 `Visual Studio 2022 community`, 安装的时候需要选中 `基于c++的桌面开发` 和 `.NET桌面开发`

5. 双击打开 `MeoAssistantArknights.sln` 文件。Visual Studio 会自动加载整个项目。
6. 测试一下是否成功搭建编程环境，选择参数 `Release`, `x64`, 右键 `MeoAsstGui` 设为启动项目；点击启动，选择继续调试。如果成功打开了 GUI，就说明成功搭建了环境。如果求稳，可以继续连接模拟器跑一下 MAA
7. 到这里，你就可以愉快地 ~~瞎 JB 改~~ 发电了
8. 开发过程中，每一定数量，记得提交一个 commit, 别忘了写上 message
9. 完成开发后，推送本地分支到远程（自己）

    ```bash
    git push origin dev
    ```

10. 打开 [MAA 仓库](https://github.com/MaaAssistantArknights/MaaAssistantArknights)。提交一个 pull request，等待管理员通过。别忘了你是在 dev 分支上修改，别提交到 master 分支去了
11. 当 MAA 原仓库出现更改（别人做的），你需要把这些更改同步到你 fork 的仓库
    1. 关联 MAA 原仓库

        ```bash
        git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
        ```

    2. 从 MAA 原仓库拉取更新

        ```bash
        git fetch upstream
        ```

    3. 合并修改

        ```bash
        git merge
        ```

    4. 重复上述 7, 8, 9, 10 中的操作

注：在打开 VS2022 之后，和 git 有关的操作可以不用命令行工具，直接使用 vs 自带的“Git 更改”即可
