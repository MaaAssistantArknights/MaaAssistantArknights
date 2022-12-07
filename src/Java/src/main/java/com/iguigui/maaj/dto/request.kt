package com.iguigui.maaj.dto

import com.iguigui.maaj.util.Json
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.JsonElement


@Serializable
data class ConnectRequest(
    @SerialName("adbPath")
    val adbPath: String,
    @SerialName("host")
    val host: String,
    @SerialName("detailJson")
    val detailJson: String = ""
)

@Serializable
data class AppendTaskRequest(
    @SerialName("id")
    val id: String,
    @SerialName("type")
    val type: String,
    @SerialName("params")
    val params: JsonElement
)

@Serializable
data class SetTaskParamsRequest(
    @SerialName("id")
    val id: String,
    @SerialName("type")
    val type: String,
    @SerialName("taskId")
    val taskId: Int,
    @SerialName("params")
    val params: JsonElement
)

@Serializable
sealed class Task

@Serializable
data class Fight(
    @SerialName("medicine")
    val medicine: Int?,
    @SerialName("penguin_id")
    val penguinId: String?,
    @SerialName("report_to_penguin")
    val reportToPenguin: Boolean?,
    @SerialName("server")
    val server: String?,
    @SerialName("stage")
    val stage: String?,
    @SerialName("stone")
    val stone: Int?,
    @SerialName("times")
    val times: Int?
) : Task()


@Serializable
data class Recruit(
    @SerialName("confirm")
    val confirm: List<Int>,
    @SerialName("expedite")
    val expedite: Boolean?,
    @SerialName("expedite_times")
    val expediteTimes: Int?,
    @SerialName("refresh")
    val refresh: Boolean?,
    @SerialName("select")
    val select: List<Int>,
    @SerialName("set_time")
    val setTime: Boolean?,
    @SerialName("times")
    val times: Int?
) : Task()


@Serializable
data class Infrast(
    @SerialName("drones")
    val drones: String?,
    @SerialName("facility")
    val facility: List<String>,
    @SerialName("mode")
    val mode: Int?,
    @SerialName("replenish")
    val replenish: Boolean?,
    @SerialName("threshold")
    val threshold: Double?
) : Task()


@Serializable
class Visit : Task()

@Serializable
data class Mall(
    @SerialName("blacklist")
    val blacklist: List<String>?,
    @SerialName("buy_first")
    val buyFirst: List<String>?,
    @SerialName("shopping")
    val shopping: Boolean?
) : Task()


@Serializable
class Award : Task()

@Serializable
data class Roguelike(
    @SerialName("mode")
    val mode: Int?,
    @SerialName("opers")
    val opers: List<Oper>
) : Task()


@Serializable
data class Oper(
    @SerialName("name")
    val name: String,
    @SerialName("skill")
    val skill: Int?,
    @SerialName("skill_usage")
    val skillUsage: Int?
)

@Serializable
data class Copilot(
    @SerialName("filename")
    val filename: String,
    @SerialName("formation")
    val formation: Boolean,
    @SerialName("stage_name")
    val stageName: String?
)



@Serializable
data class StartRequest(
    @SerialName("id")
    val id: String,
)

@Serializable
data class StopRequest(
    @SerialName("id")
    val id: String,
)

@Serializable
data class DestroyRequest(
    @SerialName("id")
    val id: String,
)


@Serializable
data class GetImage(
    @SerialName("id")
    val id: String,
)

@SerialName("connect")
@Serializable
data class WsRequest(
    @SerialName("data")
    val `data`: JsonElement,
    @SerialName("command")
    val command: String,
    @SerialName("msgId")
    val msgId: Int = 0
)


fun Task.toJsonString() = Json.encodeToString(this)



