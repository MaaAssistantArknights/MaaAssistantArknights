package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.logger
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
        webSocket("/API/V1") {
            val connection = Connection(this)
            logger.info("Ws connection connected ! connection : ${connection.session} ")
            addConnection(connection)
            try {
                for (frame in incoming) {
                    frame as? Frame.Text ?: continue
                    val receivedText = frame.readText()
                    logger.info(receivedText)
                    val wsRequest = Json.decodeFromString(WsRequest.serializer(), receivedText)
                    var response: BaseData? = null
                    when (wsRequest.command) {
                        "getVersion" -> {
                            response = GetVersionResponse(MaaService.getVersion())
                        }
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
                            MaaService.destroy(destroyRequest.id)
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

                }
            } catch (e: Exception) {
                logger.warn("Ws Exception $e.message")
                logger.warn(e.stackTraceToString())
            } finally {
                logger.info("Ws connection disconnected ! connection : $connection ")
                removeConnection(connection)
            }
        }
    }
}
