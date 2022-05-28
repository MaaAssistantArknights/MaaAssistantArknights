package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.service.Connection
import com.iguigui.maaj.service.MaaService
import com.iguigui.maaj.service.MaaService.addConnection
import com.iguigui.maaj.service.MaaService.appendTask
import com.iguigui.maaj.service.MaaService.removeConnection
import com.iguigui.maaj.util.Json
import io.ktor.websocket.*
import io.ktor.server.application.*
import io.ktor.server.routing.*
import io.ktor.server.websocket.*
import io.ktor.websocket.*
import kotlinx.serialization.json.jsonObject
import java.util.*
import kotlin.collections.LinkedHashSet


fun Application.wsRouting() {
    routing {
        webSocket("/v1") {
            println("Adding user!")
            val connection = Connection(this)
            addConnection(connection)
            for (frame in incoming) {
                frame as? Frame.Text ?: continue
                try {
                    val receivedText = frame.readText()
                    val wsRequest = Json.decodeFromString(WsRequest.serializer(), receivedText)
                    var response: BaseData? = null
                    when (wsRequest.command) {
                        "connect" -> {
                            with(Json.decodeFromJsonElement(ConnectRequest.serializer(), wsRequest.data)) {
                                response = MaaService.connect(adbPath, host, detailJson)
                            }
                        }
                        "appendTask" -> {
                            with(Json.decodeFromJsonElement(AppendTaskRequest.serializer(), wsRequest.data)) {
                                val result = appendTask(
                                    id,
                                    type,
                                    params.jsonObject.toString()
                                )
                                response = AppendTaskResponse(id, result)
                            }
                        }
                        "setTaskParams" -> {
                            with(Json.decodeFromJsonElement(SetTaskParamsRequest.serializer(), wsRequest.data)) {
                                val result = MaaService.setTaskParams(
                                    id,
                                    type,
                                    taskId,
                                    params.jsonObject.toString()
                                )
                                response = SetTaskParamsResponse(id, result)
                            }
                        }
                        "start" -> {
                            val startRequest =
                                Json.decodeFromJsonElement(StartRequest.serializer(), wsRequest.data)
                            val result = MaaService.start(startRequest.id)
                            response = StartResponse(startRequest.id, result)
                        }
                        "stop" -> {
                            val stopRequest =
                                Json.decodeFromJsonElement(StopRequest.serializer(), wsRequest.data)
                            val result = MaaService.stop(stopRequest.id)
                            response = StopResponse(stopRequest.id, result)
                        }
                        "destroy" -> {
                            val destroyRequest =
                                Json.decodeFromJsonElement(DestroyRequest.serializer(), wsRequest.data)
                            MaaService.stop(destroyRequest.id)
                            response = EmptyBaseData
                        }
                        "listInstance" -> {
                            val list = MaaService.listInstance()
                            response = ListInstanceResponse(list)
                        }
                        else -> {}
                    }
                    response?.let {
                        send(it.wapperToWsResponse(wsRequest.command, wsRequest.msgId).toJsonString())
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                    println(e.message)
                } finally {
                    println("Removing $connection!")
                    removeConnection(connection)
                }
            }
        }
    }
}
