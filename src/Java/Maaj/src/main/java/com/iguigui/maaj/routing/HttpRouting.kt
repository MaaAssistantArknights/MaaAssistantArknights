package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.service.MaaService
import io.ktor.server.application.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive


fun Application.httpRouting() {

    routing {
        route("/v1") {
            get("/getVersion") {
                val version = MaaService.getVersion()
                call.respond(GetVersion(version).wapperToResponse())
            }
            post("/connect") {
                with(call.receive<ConnectRequest>()) {
                    val connect = MaaService.connect(adbPath, host, detailJson)
                    call.respond(HttpResponse(connect.toJsonElement()))
                }
            }
            post("/appendTask") {
                with(call.receive<AppendTaskRequest>()) {
                    MaaService.appendTask(
                        id,
                        type,
                        params.jsonObject.toString()
                    )
                }
                call.respond(EmptyBaseData.wapperToResponse())
            }
            post("/setTaskParams") {
                with(call.receive<SetTaskParamsRequest>()) {
                    MaaService.setTaskParams(
                        id,
                        type,
                        taskId,
                        params.jsonObject.toString()
                    )
                }
                call.respond(EmptyBaseData.wapperToResponse())
            }
            post("/start") {
                val start = call.receive<Start>()
                MaaService.start(start.id)
                call.respond(EmptyBaseData.wapperToResponse())
            }
            post("/stop") {
                val stop = call.receive<Stop>()
                MaaService.stop(stop.id)
                call.respond(EmptyBaseData.wapperToResponse())
            }
            get("/listInstance") {
                val list = MaaService.listInstance()
                call.respond(ListInstance(list).wapperToResponse())
            }
        }
    }
}
