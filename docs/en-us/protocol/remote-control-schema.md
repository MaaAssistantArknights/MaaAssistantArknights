---
order: 8
icon: mdi:remote-desktop
---

# Remote Control Schema

To achieve remote control of MAA, you need to provide a service that must be an HTTP(S) service and provide the following two anonymously accessible endpoints. These endpoints must be HTTP(S) web endpoints.

::: warning
If the endpoint uses HTTP protocol, MAA will issue a security warning with each connection. **Deploying plaintext transmission services on the public network is highly discouraged and dangerous, for testing purposes only.**
:::

::: tip
Please note that JSON files do not support comments. The comments in the text are for demonstration purposes only and should NOT be copied directly.
:::

## Task Retrieval Endpoint

MAA will continuously poll this endpoint at 1-second intervals, attempting to retrieve tasks it needs to perform and execute them in the order they are retrieved.

The endpoint path is arbitrary but must be an HTTP(S) endpoint. For example: `https://your-control-host.net/maa/getTask`

The controlled MAA needs to fill this endpoint into the `Task Retrieval Endpoint` text box in the MAA configuration.

This endpoint must be able to accept a `Content-Type=application/json` POST request and must be able to accept the following JSON as the request content:

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // The "User Identifier" you filled in the MAA settings.
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec"         // The "Device Identifier" automatically generated in the MAA.
    ...     // If you have other uses for this endpoint, you can add optional parameters, but MAA will only send user and device
}
```

This endpoint must return a JSON formatted Response that meets at least the following format:

```json
{
    "tasks":                            // List of tasks for MAA to execute. Currently supported types are shown in the example. Connection is considered invalid if tasks don't exist.
    [
        // Sequential tasks: these tasks are queued for execution in the order issued
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Unique task ID (string type), used when reporting task results
            "type": "CaptureImage",                         // Screenshot task - captures current emulator screen and includes it as Base64 string in report payload. If issuing this task, note that your endpoint must handle large requests (screenshots can be tens of MB, exceeding default gateway size limits).
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   // Unique task ID (string type), used when reporting task results
            "type": "LinkStart",                            // Start one-click auto-battle
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   // Unique task ID (string type), used when reporting task results
            "type": "LinkStart-Recruiting",                 // Immediately execute the corresponding sub-function according to current configuration, ignoring the main interface checkbox. See below for available options.
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Unique task ID (string type), used when reporting task results
            "type": "Toolbox-GachaOnce",                    // Toolbox gacha simulation task. Available options: Toolbox-GachaOnce, Toolbox-GachaTenTimes
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Unique task ID (string type), used when reporting task results
            "type": "Settings-ConnectionAddress",           // Configuration modification task, equivalent to executing ConfigurationHelper.SetValue("ConnectionAddress", params). For security, not all configurations can be modified. See below for available options.
            "params": "value"                               // Value to be set
        },
        // Immediate Execution Tasks: can execute during sequential task execution, guaranteed to return results quickly. Typically used to control the remote control function itself.
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Unique task ID (string type), used when reporting task results
            "type": "CaptureImageNow",                      // Immediate screenshot task, similar to CaptureImage but executes immediately without waiting for other tasks.
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Unique task ID (string type), used when reporting task results
            "type": "StopTask",                             // "Stop current task" - attempts to end currently running task. If other tasks remain in queue, will proceed to next one. Does not wait to confirm task has stopped before returning, so use HeartBeat task to verify stop command effectiveness.
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Unique task ID (string type), used when reporting task results
            "type": "HeartBeat",                            // Heartbeat task - returns immediately with ID of currently executing sequential task as payload. Returns empty string if no task is running.
        },
    ],
    ...     // If you have other uses for this endpoint, you can add optional return values, but MAA will only read tasks
}
```

These tasks will be executed in sequence, meaning if you send a recruitment task followed by a screenshot task, the screenshot will execute after the recruitment task completes.
This endpoint should be reentrant and consistently return tasks to execute. MAA automatically records task IDs and will not execute the same ID multiple times.

::: note

- LinkStart-[TaskName] task types include: LinkStart-Base, LinkStart-WakeUp, LinkStart-Combat, LinkStart-Recruiting, LinkStart-Mall, LinkStart-Mission, LinkStart-AutoRoguelike, LinkStart-ReclamationAlgorithm
- Settings-[SettingsName] task types include: Settings-ConnectionAddress, Settings-Stage1
- Settings tasks are still executed sequentially, not immediately upon receipt, but queued after the previous task
- Multiple immediate execution tasks are also executed in their issue order, but since they execute quickly, their ordering is generally not a concern
:::

## Task Report Endpoint

When MAA completes a task, it reports the result to the remote endpoint through this endpoint.

The endpoint path is arbitrary but must be an HTTP(S) endpoint. For example: `https://your-control-host.net/maa/reportStatus`

The controlled MAA needs to fill this endpoint into the `Task Report Endpoint` text box in the MAA configuration.

This endpoint must be able to accept a `Content-Type=application/json` POST request and must be able to accept the following JSON as the request content:

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // The "User Identifier" you filled in the MAA settings.
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec",        // The "Device Identifier" automatically generated in the MAA.
    "task":"15be4725-5bd3-443d-8ae3-0a5ae789254c",          // Task ID being reported, corresponding to the ID from getTask.
    "status":"SUCCESS",                                     // Task execution result: SUCCESS or FAILED. Generally returns SUCCESS regardless of execution outcome; FAILED only in special cases as described in task documentation.
    "payload":"",                                           // Reported data (string type). Content depends on task type. For screenshot tasks, contains the Base64 string of the image.
    ...     // If you have other uses for this endpoint, you can add optional parameters, but MAA will only send user, device, task, status, and payload
}
```

The response content from this endpoint is arbitrary, but if you don't return 200 OK, MAA will display an `Upload failed` notification.

## Example Workflow: Controlling MAA with a QQ Bot

Developer A wants to control MAA using their QQ Bot, so they develop a backend exposed on the public internet with two endpoints:

```text
https://myqqbot.com/maa/getTask
https://myqqbot.com/maa/reportStatus
```

For user convenience, their getTask interface always returns 200 OK with an empty tasks list regardless of parameters received.
Each time they receive a request, they check if the device exists in their database. If not, they record the device and user.
In effect, this interface also handles user registration.

They provide a command in their QQ Bot for users to submit their deviceId.

In the Bot's usage instructions, they tell users to enter their QQ number in MAA's `User Identifier` field, then send the `Device Identifier` to the Bot via QQ chat.

When the Bot receives an identifier, it checks if there's a matching record in the database based on the QQ number. If not, it asks the user to configure MAA first.

Since properly configured MAA sends continuous requests, if a user has set up MAA correctly, a matching record should already exist in the database when they submit via QQ.

The Bot then marks that database record as verified, so future getTask requests with this device and user will receive actual task lists.

When a user submits a command via QQ Bot, the Bot writes a task to the database, which getTask will then return. Thoughtfully, the Bot also adds a screenshot task after each user command.

After completing a task, MAA calls reportStatus with the results, and the Bot notifies the user and displays any screenshots in QQ.

## Example Workflow: Controlling MAA with a Website

Developer B created a website for batch management of MAA instances, with their own user management system. Their backend is publicly accessible with two anonymously accessible endpoints:

```text
https://mywebsite.com/maa/getTask
https://mywebsite.com/maa/reportStatus
```

On the website, a page for connecting MAA instances displays a random string called a `User Key` and provides a text field for the device ID.

Users are instructed to enter their User Key in MAA's `User Identifier` field and their `Device Identifier` on the website.

The getTask endpoint only returns 200 OK if a connection has been successfully created on the website. Otherwise, it returns 401 Unauthorized.

This means users who enter incorrect information in MAA will see a connection test failure message.

Users can issue tasks, queue tasks, view screenshots, and more through the website. These features work similarly to the QQ Bot example, using getTask and reportStatus in combination.
