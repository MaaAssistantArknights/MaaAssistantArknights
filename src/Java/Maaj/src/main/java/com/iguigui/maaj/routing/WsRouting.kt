package com.iguigui.maaj.routing

import com.iguigui.maaj.dto.*
import io.ktor.serialization.kotlinx.json.*
import io.ktor.server.application.*
import io.ktor.server.plugins.contentnegotiation.*
import io.ktor.server.request.*
import io.ktor.server.routing.*
import io.ktor.server.websocket.*
import io.ktor.websocket.*


fun Application.wsRouting() {

    routing {
        webSocket("/v1") {
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
