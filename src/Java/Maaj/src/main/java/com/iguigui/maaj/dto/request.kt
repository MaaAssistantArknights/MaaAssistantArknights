package com.iguigui.maaj.dto

import com.iguigui.maaj.util.Json
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.encodeToString

sealed class WsRequest

@Serializable
data class Connect(
    @SerialName("adbPath")
    val adbPath: String,
    @SerialName("host")
    val host: String
)

@Serializable
data class AppendTask(
    @SerialName("host")
    val host: String,
    @SerialName("type")
    val type: String,
    @SerialName("params")
    val params: Task
)

@Serializable
sealed class Task

@Serializable
data class Fight(
    @SerialName("stage")
    val stage: String
) : Task()


@Serializable
data class Start(
    @SerialName("host")
    val host: String
)

@Serializable
data class Stop(
    @SerialName("host")
    val host: String
)

@SerialName("connect")
@Serializable
data class ConnectWs(
    @SerialName("data")
    val `data`: Connect,
    @SerialName("type")
    val type: String
) : WsRequest()


fun Task.toJsonString() = Json.encodeToString(this)



