package com.iguigui.maaj

import com.iguigui.maaj.routing.httpRouting
import com.iguigui.maaj.routing.wsRouting
import io.ktor.serialization.kotlinx.json.*
import io.ktor.server.netty.*
import io.ktor.server.application.*
import io.ktor.server.plugins.contentnegotiation.*
import io.ktor.server.plugins.cors.routing.*
import io.ktor.server.websocket.*
import kotlinx.serialization.json.Json

fun main(args: Array<String>): Unit = EngineMain.main(args)

fun Application.module() {
    configure()
    httpRouting()
    wsRouting()
}

fun Application.configure(){
    install(ContentNegotiation) {
        json(Json {
            encodeDefaults = true
            isLenient = true
            allowSpecialFloatingPointValues = true
            allowStructuredMapKeys = true
            prettyPrint = false
            useArrayPolymorphism = false
        })
    }
    install(CORS) {
        allowCredentials = true
        anyHost()
    }
    install(WebSockets) {
        pingPeriod = java.time.Duration.ofSeconds(15)
        timeout = java.time.Duration.ofSeconds(15)
        maxFrameSize = Long.MAX_VALUE
        masking = false
    }
}
