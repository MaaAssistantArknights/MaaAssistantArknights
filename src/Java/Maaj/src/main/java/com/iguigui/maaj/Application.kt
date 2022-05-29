package com.iguigui.maaj

import com.iguigui.maaj.routing.httpRouting
import com.iguigui.maaj.routing.wsRouting
import io.ktor.http.*
import io.ktor.serialization.kotlinx.json.*
import io.ktor.server.netty.*
import io.ktor.server.application.*
import io.ktor.server.plugins.callloging.*
import io.ktor.server.plugins.contentnegotiation.*
import io.ktor.server.plugins.cors.routing.*
import io.ktor.server.plugins.doublereceive.*
import io.ktor.server.request.*
import io.ktor.server.websocket.*
import io.ktor.util.logging.*
import kotlinx.coroutines.runBlocking
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonElement
import org.slf4j.event.Level


fun main(args: Array<String>): Unit = EngineMain.main(args)

fun Application.module() {
    configure()
    httpRouting()
    wsRouting()
}


lateinit var logger : Logger

fun Application.configure() {
    logger = log
    //Input and output serialize
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
    //Provides the ability to receive a request body several times.
    install(DoubleReceive) {
    }
    //Logging request body.
    install(CallLogging) {
        level = Level.INFO
        format { call ->
            val status = call.response.status()
            val httpMethod = call.request.httpMethod
            val queryParameters = call.request.rawQueryParameters
            when (httpMethod) {
                HttpMethod.Get -> "Status: $status, HTTP method: $httpMethod, Uri: ${call.request.uri}, QueryParameters: $queryParameters "
                HttpMethod.Post -> runBlocking {
                    "Status: $status, HTTP method: $httpMethod, Uri: ${call.request.uri} ,Body: ${
                        call.receive<JsonElement>().toString()
                    } "
                }
                else -> {
                    "Status: $status, HTTP method: $httpMethod, Uri: ${call.request.uri}, QueryParameters: $queryParameters "
                }
            }
        }
    }

}
