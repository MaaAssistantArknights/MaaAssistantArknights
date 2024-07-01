---
order: 8
icon: mdi:remote-desktop
---

# Remote Control Schema

To achieve remote control of MAA, you need to provide a service that must be an HTTP(S) service and provide the following two anonymously accessible endpoints. These endpoints must be HTTP(S) web endpoints.

::: warning
If the endpoint is an HTTP protocol, MAA will issue a security warning with each connection. **Deploying plaintext transmission services on the public network is highly discouraged and dangerous, for testing purposes only.**
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
    "tasks":                            // A list of Tasks that need to be allowed to be executed by the MAA, the types supported currently are shown in the example, and the connection is considered invalid if the tasks do not exist.
    [
        // Sequential tasks: the following tasks are queued for execution in the order in which they are issued.
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //A unique uuid for the task, type : string, which will be used when reporting on the task.
            "type": "CaptureImage",                         //A screenshot task that takes a screenshot of the current emulator and puts it in the payload of the reporting task as a Base64 string. If you need to issue this type of task, be sure to pay attention to the maximum request size that your endpoint can accept, as the screenshot size will lager than 10MB and exceed the default size limit of a typical gateway.
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   //A unique uuid for the task, type : string, which will be used when reporting on the task.
            "type": "LinkStart",                            //LinkStart😄
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   // A unique uuid, used in the same way as above.
            "type": "LinkStart-Recruiting",                 // Immediately executes the corresponding sub-function of ‘LinkStart’ individually according to the current configuration, ignoring the tick box of this function on the GUI. The optional values for this type of Type are detailed below.
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // Same as ‘id’ above
            "type": "Toolbox-GachaOnce",                    //The Gacha function in the toolbox, with optional values for this class Type:Toolbox-GachaOnce, Toolbox-GachaTenTimes
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // A unique uuid, used in the same way as above.
            "type": "Settings-ConnectionAddress",           //The task of modifying a configuration item is equivalent to executing the ConfigurationHelper.SetValue("ConnectionAddress", params); For security reasons, not every configuration can be modified, and those that can are detailed below.
            "params": "value"                               //The value you want to config
        },
        // Immediate Execution Tasks: these following tasks can be executed in a Sequential Execution Task run and the MAA guarantees that any of the following tasks will return results as soon as possible, and are typically used for control of the remote control function itself.
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // A unique uuid, used in the same way as above.
            "type": "CaptureImageNow",                      //The Immediate Screenshot task is basically the same as the Screenshot task above, the only difference is that this task will be run immediately without waiting for other tasks.
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // A unique uuid, used in the same way as above.
            "type": "StopTask",                             //The "Stop current task" task will attempt to end the currently running task. If there are other tasks in the task list it will continue with the next one. This task does not wait to confirm that the current task has stopped before returning, so use the "HeartBeat" task to confirm that the stop command has taken effect.
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // A unique uuid, used in the same way as above.
            "type": "HeartBeat",                            // "Heartbeat" task, the task will immediately return the current ‘sequential task’ queue in the task is executing as the Payload, if there is currently no task execution, return the empty string.
        },
    ],
    ...     // If you have other uses for this endpoint, you can add other return values of your own, but MAA will only read tasks.
}
```

This endpoint must return a JSON formatted Response and must at least meet the following format:

::: note

- The `LinkStart-[TaskName]` series of tasks have optinal `type` : `LinkStart-Base`，`LinkStart-WakeUp`，`LinkStart-Combat`，`LinkStart-Recruiting`，`LinkStart-Mall`，`LinkStart-Mission`，`LinkStart-AutoRoguelike`，`LinkStart-ReclamationAlgorithm`
- The `Settings-[SettingsName]` series of tasks have optinal `type` : `Settings-ConnectionAddress`, `Settings-Stage1`
- The Settings series of tasks are still meant to be executed sequentially, not immediately upon receipt, but after the previous task.
- Multiple immediately executable tasks are also executed in the order in which they are issued, except that they are all executed so quickly that, in general, it is not necessary to be concerned about their order.
    :::

## Report Status Endpoint

When MAA completes a task, it will report the result to this endpoint.

The endpoint path is arbitrary but must be an HTTP(S) endpoint. For example: `https://your-control-host.net/maa/reportStatus`

The controlled MAA needs to fill this endpoint into the `Report Status Endpoint` text box in the MAA configuration.

This endpoint must be able to accept a `Content-Type=application/json` POST request and must be able to accept the following JSON as the request content:

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // The "User Identifier" you filled in the MAA settings.
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec",        // The "Device Identifier" automatically generated in the MAA.
    "task":"15be4725-5bd3-443d-8ae3-0a5ae789254c",          // The Id of the task to be reported on, corresponding to the Id when 'getTask'.
    "status":"SUCCESS",                                     // The result of the task execution, SUCCESS or FAILED. generally, regardless of the success of the task execution will only return SUCCESS, only in special circumstances will return FAILED, will return FAILED situation, will be explicitly described in the above task introduction.
    "payload":"",                                           //The data to carry when reporting, string type. Depends on the task type, for example, when reporting on a screenshot task, the Base64 string of the screenshot will be carried here.
    ...     // If you have other uses for this endpoint, you can add other return values of your own, but MAA will only post upper value.
}
```

The content returned by this endpoint is arbitrary, but if you do not return `200 OK`, a notification will pop up on the MAA side displaying `Upload failed`.

## Example Workflow : Controlling MAA with QQBot

A developer wants to control MAA with their QQBot (QQ is an instant messaging software,like WhatsApp or Telegram), so they develop a backend exposed on the public network, providing two endpoints:

```text
https://myqqbot.com/maa/getTask
https://myqqbot.com/maa/reportStatus
```

To make it more convenient for users, their `getTask` interface always returns `200 OK` and an empty tasks list regardless of the parameters received.
Each time they receive a request, they check the database for duplicate devices, and if none, they record the device and user in the database.
In this workflow, this interface also serves as a user registration function.

They provide a command on the QQBot for users to submit their deviceId.

In the QQBot's usage instructions, they tell users to fill in their QQ number in the `User Identifier` field of MAA and send the `Device Identifier` to the Bot via QQ chat.

Upon receiving the identifier, the QQBot checks the database for corresponding data based on the user's QQ number in the message. If none is found, it tells the user to configure MAA first.

Since MAA continuously sends requests once configured, if the user has configured MAA, there should be matching records in the database when they submit via QQ.

At this point, the Bot marks the record in the database as verified, so future requests from getTask with this device and user will return the real task list.

When the user submits a command via QQBot, the Bot writes a task into the database. Shortly after, getTask will return this task. Additionally, the QQBot thoughtfully adds a screenshot task each time the user submits a command.

MAA will call reportStatus to report the result after completing the task. The Bot will send a message notifying the user and display the screenshot on QQ.

## Example Workflow : Controlling MAA with a Website

Developer B wrote a website to manage MAA in bulk through a website, so they have their own user management system. However, their backend is publicly accessible, providing two anonymously accessible endpoints:

```text
https://mywebsite.com/maa/getTask
https://mywebsite.com/maa/reportStatus
```

On the website, there is an interface to connect MAA instances, will generate a random string called `User Key`, along with a text box for entering the `Device Identifier`.

The website requires users to fill in their `User Key` in the `User Identifier` field of MAA GUI and then enter the `Device Identifier` on the website.

Only after successfully creating a connection to MAA on the website, `getTask` will return `200 OK`. Otherwise, it returns `401 Unauthorized`.

If the user fills it incorrectly on MAA and presses the test connection button, they will get a test failure prompt.

Users can issue tasks on the website, queue tasks, view screenshots, and more. The implementation of these functions is similar to the QQBot example above, achieved through a combination of `getTask` and `reportStatus`.
