package com.iguigui.maaj.service

import com.iguigui.maaj.dto.CallBackLog
import com.iguigui.maaj.easySample.MeoAssistant
import com.iguigui.maaj.util.Json
import com.sun.jna.Pointer
import java.util.LinkedList
import java.util.Queue
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong

class MaaInstance(
    private val instance: MeoAssistant,
    val id: String,
    val adbPath: String,
    val host: String,
    val detailJson: String
) :
    MeoAssistant.AsstApiCallback {

    constructor(
        instance: MeoAssistant,
        id: String,
        adbPath: String,
        host: String,
        pointer: Pointer,
        detailJson: String
    ) : this(
        instance,
        id,
        adbPath,
        host,
        detailJson
    ) {
        this.pointer = pointer
    }

    lateinit var pointer: Pointer

    var status = 0

    var logQueue: Queue<CallBackLog> = LinkedBlockingQueue(1000)

    var msgId = AtomicLong(0)


    override fun callback(msg: Int, detail_json: String, custom_arg: String) {
        System.out.printf("回调msg : %s , 回调 detail_json : %s ,回调 custom_arg : %s \n", msg, detail_json, custom_arg)
        logQueue.offer(CallBackLog(id, msgId.getAndIncrement(), msg, Json.parseToJsonElement(detail_json)))
    }

    fun connect(): Boolean = instance.AsstConnect(pointer, adbPath, host, detailJson)

    fun appendTask(type: String, params: String) = instance.AsstAppendTask(pointer, type, params)

    fun setTaskParams(type: String, taskId: Int, params: String) = instance.AsstSetTaskParams(pointer, taskId, params)

    fun start() = instance.AsstStart(pointer)

    fun stop() = instance.AsstStop(pointer)

}
