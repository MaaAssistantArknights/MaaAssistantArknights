---
order: 1
icon: ic:round-rocket-launch
---

# Startup

Automatically starts the emulator, launches the client, and enters the game.

Automatic emulator startup requires additional configuration in the startup settings and enabling `Retry launching the emulator when ADB connection fails` in the connection settings.

If the game is showing the disconnection popup, Startup taps confirm to reconnect and then continues into the game.

## Startup Recognition Timeout

In the general settings for `Startup`, set `Startup recognition timeout (s)` to an integer from `10` to `600`. The default is `120` seconds. Increase it if the emulator remains on a black screen for a long time during a cold start.

This setting limits the elapsed time spent continuously failing to recognize the screen at each step of startup navigation and account-switch navigation. Recognition success advances immediately, and the next step gets a new timeout. It does not add a fixed delay. Recognized loading or update screens are not continuous recognition failures, so this is not a time limit for the entire startup process.

When the timeout expires, the existing error or game-restart handling still applies. The setting only affects Startup; it does not change waiting times for recruitment, infrastructure, or other tasks.

## Account Switch

This feature needs to be used together with configuration switching or scheduled execution. An account switch operation will be performed each time the `Startup` task is executed.

Only supports switching to already logged-in accounts, using the login name for identification. Please ensure that the input content is unique among all logged-in accounts.

- Examples:
  - Official server: `123****8901` can be entered as `123****8901`, `123`, `8901`, or `3****8`
  - Bilibili server: `Zhang San` can be entered as `Zhang San`, `Zhang`, or `San`
  - Traditional Chinese server: Email-based, e.g. `ab****01@gmail.com`. Entering the plain-text portion without asterisks is recommended, e.g. `01@gmail`
  - YostarKR server: Email-based, e.g. `ab****01@gmail.com`. Entering the plain-text portion without asterisks is recommended, e.g. `01@gmail`

## Connection Settings

[Click here](../connection.md)
