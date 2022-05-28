package com.iguigui.maaj.service

import com.iguigui.maaj.dto.CallBackLog
import com.iguigui.maaj.dto.*
import com.iguigui.maaj.easySample.MeoAssistant
import com.iguigui.maaj.util.Json
import com.sun.jna.Pointer
import java.util.concurrent.atomic.AtomicLong
import kotlin.reflect.KFunction1

class MaaInstance(
    private val instance: MeoAssistant,
    val id: String,
    val adbPath: String,
    val host: String,
    private val detailJson: String,
    val callBackAction: KFunction1<CallBackLog, Unit>
) :
    MeoAssistant.AsstApiCallback {

//    constructor(
//        instance: MeoAssistant,
//        id: String,
//        adbPath: String,
//        host: String,
//        pointer: Pointer,
//        detailJson: String
//    ) : this(
//        instance,
//        id,
//        adbPath,
//        host,
//        detailJson
//    ) {
//        this.pointer = pointer
//    }

    lateinit var pointer: Pointer

    var uuid = ""

    //状态码，0初始化未运行 1运行中
    var status = 0

    //    private var logQueue: ArrayBlockingQueue<CallBackLog> = ArrayBlockingQueue(100)
//
    private var msgId = AtomicLong(0)


    override fun callback(msg: Int, detail_json: String, custom_arg: String) {
        System.out.printf("回调msg : %s , 回调 detail_json : %s ,回调 custom_arg : %s \n", msg, detail_json, custom_arg)
        val callBackLog = CallBackLog(id, msgId.getAndIncrement(), msg, Json.parseToJsonElement(detail_json))
        callBackAction?.call(callBackLog)
        when (msg) {
            INTERNAL_ERROR,
            INIT_FAILED,
            ALL_TASKS_COMPLETED,
            TASK_CHAIN_ERROR,
            TASK_CHAIN_COMPLETED,
            SUB_TASK_ERROR,
            SUB_TASK_COMPLETED -> status = 0
            //也不知道啥算正在运行，姑且认为任务链开始和原子任务开始肯定是在运行中吧
            TASK_CHAIN_START,
            SUB_TASK_START -> status = 1
            else -> {}
        }
    }

    fun connect(): Boolean = instance.AsstConnect(pointer, adbPath, host, detailJson)

    fun appendTask(type: String, params: String) = instance.AsstAppendTask(pointer, type, params)

    fun setTaskParams(type: String, taskId: Int, params: String) = instance.AsstSetTaskParams(pointer, taskId, params)

    fun start(): Boolean {
        msgId.set(0)
        return instance.AsstStart(pointer)
    }

    fun stop() = instance.AsstStop(pointer)
    fun destroy() = instance.AsstDestroy(pointer)

    //todo
    private fun addLog(callBackLog: CallBackLog) {

    }

}
