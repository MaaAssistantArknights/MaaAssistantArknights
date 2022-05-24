package com.iguigui.maaj

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.service.MaaService
import io.ktor.server.netty.*
import io.ktor.server.application.*
import io.ktor.server.plugins.contentnegotiation.*
import io.ktor.server.request.*
import io.ktor.server.routing.*
import io.ktor.server.websocket.*
import io.ktor.websocket.*
import java.time.Duration
import io.ktor.serialization.kotlinx.json.*

fun main(args: Array<String>): Unit = EngineMain.main(args)

fun Application.module() {
    configureRouting()
}

fun Application.configureRouting() {
    install(ContentNegotiation) {
        json()
    }
    install(WebSockets) {
        pingPeriod = Duration.ofSeconds(15)
        timeout = Duration.ofSeconds(15)
        maxFrameSize = Long.MAX_VALUE
        masking = false
    }
    routing {

        post("/connect") {
            val connect = call.receive<Connect>()
            MaaService.connect(connect.adbPath, connect.host, "")
        }

        post("/appendTask") {
            val appendTask = call.receive<AppendTask>()
            MaaService.appendTask(appendTask.host, appendTask.type, appendTask.params.toJsonString())
        }

        post("/start") {
            val start = call.receive<Start>()
            MaaService.start(start.host)
        }

        post("/stop") {
            val stop = call.receive<Stop>()
            MaaService.stop(stop.host)
        }


        webSocket("/") {
            send("You are connected!")
            for (frame in incoming) {
                frame as? Frame.Text ?: continue
                val receivedText = frame.readText()
                println("接收到信息 : $receivedText")
                send("You said: $receivedText")
            }
        }
    }
}
