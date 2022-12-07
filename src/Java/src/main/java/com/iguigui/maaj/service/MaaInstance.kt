package com.iguigui.maaj.service

import com.iguigui.maaj.dto.CallBackLog
import com.iguigui.maaj.dto.*
import com.iguigui.maaj.easySample.MaaCore
import com.iguigui.maaj.logger
import com.iguigui.maaj.util.Json
import com.sun.jna.Pointer
import io.ktor.server.application.*
import java.util.concurrent.atomic.AtomicLong
import kotlin.reflect.KFunction1

class MaaInstance(
    private val instance: MaaCore,
    val id: String,
    val adbPath: String,
    val host: String,
    private val detailJson: String,
    val callBackAction: KFunction1<CallBackLog, Unit>
) :
    MaaCore.AsstApiCallback {

//    constructor(
//        instance: MaaCore,
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
        val callBackLog = CallBackLog(id, msgId.getAndIncrement(), msg, Json.parseToJsonElement(detail_json))
        logger.info("callBackLog = $callBackLog")
        callBackAction.call(callBackLog)
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

    fun setTaskParams(taskId: Int, params: String) = instance.AsstSetTaskParams(pointer, taskId, params)

    fun start(): Boolean {
        msgId.set(0)
        return instance.AsstStart(pointer)
    }

    fun stop() = instance.AsstStop(pointer)
    fun destroy() = instance.AsstDestroy(pointer)

    fun getImage(): ByteArray? {
        val size = 1024 * 1024 * 6
        val byteArray = ByteArray(size)
        val length = instance.AsstGetImage(pointer, byteArray, size.toLong())
        if (length == 0L) {
            return null
        }
        val target = ByteArray(length.toInt())
        System.arraycopy(byteArray,0,target,0,length.toInt())
        return target
    }

}
