package com.iguigui.maaj.service

import com.iguigui.maaj.easySample.MeoAssistant
import com.sun.jna.Pointer

class MaaInstance(private val instance: MeoAssistant, val id: String, val adbPath: String, val host: String) :
    MeoAssistant.AsstApiCallback {

    constructor(
        instance: MeoAssistant,
        id: String,
        adbPath: String,
        host: String,
        pointer: Pointer
    ) : this(
        instance,
        id,
        adbPath,
        host
    ) {
        this.pointer = pointer
    }

    lateinit var pointer: Pointer

    var status = 0

    override fun callback(msg: Int, detail_json: String?, custom_arg: String?) {
        System.out.printf("回调msg : %s , 回调 detail_json : %s ,回调 custom_arg : %s \n", msg, detail_json, custom_arg)
    }

    fun connect(): Boolean = instance.AsstConnect(pointer, adbPath, host, "");

    fun appendTask(type: String, params: String) = instance.AsstAppendTask(pointer, type, params)

    fun setTaskParams(type: String, params: String) = instance.AsstSetTaskParams(pointer, 1, params)

    fun start() = instance.AsstStart(pointer)

    fun stop() = instance.AsstStop(pointer)

}
