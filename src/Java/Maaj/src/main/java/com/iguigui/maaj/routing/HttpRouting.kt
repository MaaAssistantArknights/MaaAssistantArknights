package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.service.MaaService
import com.iguigui.maaj.service.MaaService.appendTask
import io.ktor.server.application.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*
import kotlinx.serialization.json.jsonObject


fun Application.httpRouting() {

    routing {
        route("/v1") {
            get("/getVersion") {
                val version = MaaService.getVersion()
                call.respond(GetVersionResponse(version).wapperToResponse())
            }
            post("/connect") {
                with(call.receive<ConnectRequest>()) {
                    val connect = MaaService.connect(adbPath, host, detailJson)
                    call.respond(connect.wapperToResponse())
                }
            }
            post("/appendTask") {
                with(call.receive<AppendTaskRequest>()) {
                    val result = appendTask(
                        id,
                        type,
                        params.jsonObject.toString()
                    )
                    call.respond(AppendTaskResponse(id, result).wapperToResponse())
                }
            }
            post("/setTaskParams") {
                with(call.receive<SetTaskParamsRequest>()) {
                    val result = MaaService.setTaskParams(
                        id,
                        type,
                        taskId,
                        params.jsonObject.toString()
                    )
                    call.respond(SetTaskParamsResponse(id, result).wapperToResponse())
                }
            }
            post("/start") {
                val start = call.receive<Start>()
                val result = MaaService.start(start.id)
                call.respond(StartResponse(start.id, result).wapperToResponse())
            }
            post("/stop") {
                val stop = call.receive<Stop>()
                val result = MaaService.stop(stop.id)
                call.respond(StartResponse(stop.id, result).wapperToResponse())
            }
            get("/listInstance") {
                val list = MaaService.listInstance()
                call.respond(ListInstanceResponse(list).wapperToResponse())
            }
        }
    }
}
