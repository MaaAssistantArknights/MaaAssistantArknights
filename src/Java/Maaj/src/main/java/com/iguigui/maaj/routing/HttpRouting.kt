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
        route("/API/V1") {
            get("/getVersion") {
                val version = MaaService.getVersion()
                call.respond(GetVersionResponse(version).wapperToHttpResponse())
            }
            post("/connect") {
                with(call.receive<ConnectRequest>()) {
                    val connect = MaaService.connect(adbPath, host, detailJson)
                    call.respond(connect.wapperToHttpResponse())
                }
            }
            post("/appendTask") {
                with(call.receive<AppendTaskRequest>()) {
                    val result = appendTask(
                        id,
                        type,
                        params.jsonObject.toString()
                    )
                    call.respond(AppendTaskResponse(id, result).wapperToHttpResponse())
                }
            }
            post("/setTaskParams") {
                with(call.receive<SetTaskParamsRequest>()) {
                    val result = MaaService.setTaskParams(
                        id,
                        taskId,
                        params.jsonObject.toString()
                    )
                    call.respond(SetTaskParamsResponse(id, result).wapperToHttpResponse())
                }
            }
            post("/start") {
                val startRequest = call.receive<StartRequest>()
                val result = MaaService.start(startRequest.id)
                call.respond(StartResponse(startRequest.id, result).wapperToHttpResponse())
            }
            post("/stop") {
                val stopRequest = call.receive<StopRequest>()
                val result = MaaService.stop(stopRequest.id)
                call.respond(StartResponse(stopRequest.id, result).wapperToHttpResponse())
            }
            post("/destroy") {
                val destroy = call.receive<DestroyRequest>()
                MaaService.destroy(destroy.id)
                call.respond(EmptyBaseData.wapperToHttpResponse())
            }
            get("/listInstance") {
                val list = MaaService.listInstance()
                call.respond(ListInstanceResponse(list).wapperToHttpResponse())
            }
        }
    }
}
