package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.service.MaaService
import io.ktor.server.application.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*
import kotlinx.serialization.json.jsonPrimitive


fun Application.httpRouting() {

    routing {
        route("/v1") {
            get("/getVersion") {
                val version = MaaService.getVersion()
                call.respond(GetVersion(version).wapperToResponse())
            }
            post("/connect") {
                val connect = call.receive<ConnectRequest>()
                val id = MaaService.connect(connect.adbPath, connect.host, "")
                call.respond(HttpResponse(ConnectResponse(id).toJsonElement()))
            }
            post("/appendTask") {
                with(call.receive<AppendTaskRequest>()) {
                    MaaService.appendTask(
                        host,
                        type,
                        params.jsonPrimitive.content
                    )
                }
                call.respond(EmptyBaseData.wapperToResponse())
            }
            post("/setTaskParams") {
                with(call.receive<SetTaskParamsRequest>()) {
                    MaaService.setTaskParams(
                        host,
                        type,
                        taskId,
                        params.jsonPrimitive.content
                    )
                }
                call.respond(EmptyBaseData.wapperToResponse())
            }
            post("/start") {
                val start = call.receive<Start>()
                MaaService.start(start.host)
                call.respond(EmptyBaseData.wapperToResponse())
            }
            post("/stop") {
                val stop = call.receive<Stop>()
                MaaService.stop(stop.host)
                call.respond(EmptyBaseData.wapperToResponse())
            }
            get("/listInstance") {
                val list = MaaService.listInstance()
                call.respond(ListInstance(list).wapperToResponse())
            }
            get("/getLog") {
                val list = MaaService.listInstance()
                call.respond(ListInstance(list).wapperToResponse())
            }
        }
    }
}
