---
order: 1
icon: ri:guide-fill
---

# Getting Started

## Preparation

1. Confirm system version

   MAA only supports Windows 10 / 11. For earlier versions of Windows, please refer to [FAQ](./faq.md) at the top.

2. Install the runtime library

   MAA requires VCRedist x64 and .NET 8. Right-click the start button to open the terminal, paste the following command in it and Enter to install.

   ```sh
   winget install Microsoft.VCRedist.2015+.x64
   winget install Microsoft.DotNet.DesktopRuntime.8
   ```

   Refer to [FAQ](faq.md#possible-cause-2-missing-runtime-libraries) for more information

3. Download the correct version

   [MAA official website](https://maa.plus/) will automatically selects the correct version architecture, which for most users reading this article should be Windows x64.

4. Unzip correctly

   Verify that the extraction is complete and make sure to extract the MAA to a separate folder. Do not extract MAA to a path that requires UAC permissions such as `C:\`, `C:\Program Files\`, etc.

5. Confirm simulator support

   Check [Emulator Supports](./device/) to confirm the support of the emulator you are using.

6. Correctly set the emulator resolution

   The simulator resolution should be `16:9` ratio, with a minimum of `1280x720`; for YostarEN, a minimum of `1920x1080`.

## Initial

0. If you'd like to use automatic detection, run **one** emulator and make sure no other Android devices are connected to the computer.

1. Follow the setup guide to configure, and MAA will automatically detect the running emulator. If you'd like to run multiple emulator or customize your connection, please check [Connection](./connection.md).

2. A hot update will be performed when MAA is run for the first time. Please close MAA and restart after prompting in the log on the right.

3. Drag the task list on the left to sort the tasks, and check or uncheck the checkboxes to select
   the tasks to be run.

4. Link Start!

## Advanced configuration

Check out the documentation.

## Other

- **When encountering problems, read the documents please.**

1. The log files are located in the folder under the MAA folder which named `debug`. Logs are important. When asking others for help, be sure to take `asst.log` and `gui.log` with you.

2. In order to meet the various needs of all users as much as possible, MAA provides a large number of customization options. When you find that MAA lacks a certain function, it may be because it is not easily discoverable rather than non-existent, such as `Manual entry of stage names` and `Do not put stationed operators in Dormitory`.

3. Placing the mouse pointer on some options will give detailed instructions, such as `Do not put stationed operators in Dormitory`.

4. For some check boxes, right-click on them to change them to a half-selected state. The check boxes in this state will be automatically cleared the next time MAA is started, which can be understood as `only once`.

5. If you're uncertain about your needs, don't select the Nightly Release.
