package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.service.MaaService
import io.ktor.server.application.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*


fun Application.httpRouting() {

    routing {
        route("/v1") {
            get("/getVersion") {
                val version = MaaService.getVersion()
                call.respond(HttpResponse(GetVersion(version).toJsonElement()))
            }
            get("/listInstance") {
                val list = MaaService.listInstance()
                call.respond(HttpResponse(ListInstance(list).toJsonElement()))
            }
            post("/connect") {
                val connect = call.receive<ConnectRequest>()
                val id = MaaService.connect(connect.adbPath, connect.host, "")
                call.respond(HttpResponse(ConnectResponse(id).toJsonElement()))
            }
            post("/appendTask") {
                val appendTask = call.receive<AppendTaskRequest>()
                MaaService.appendTask(appendTask.host, appendTask.type, appendTask.params.toJsonString())
                call.respond(HttpResponse())
            }
            post("/setTaskParams") {
                val appendTask = call.receive<AppendTaskRequest>()
                MaaService.setTaskParams(appendTask.host, appendTask.type, appendTask.params.toJsonString())
                call.respond(HttpResponse())
            }
            post("/start") {
                val start = call.receive<Start>()
                MaaService.start(start.host)
                call.respond(HttpResponse())
            }
            post("/stop") {
                val stop = call.receive<Stop>()
                MaaService.stop(stop.host)
                call.respond(HttpResponse())
            }
        }
    }
}
