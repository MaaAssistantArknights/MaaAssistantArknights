package com.iguigui.maaj.service

import com.iguigui.maaj.easySample.MeoAssistant
import com.sun.jna.Pointer

class MaaInstance(private val instance: MeoAssistant, val id: String, val adbPath: String, val host: String) :
    MeoAssistant.AsstApiCallback {

    constructor(
        instance: MeoAssistant,
        pointer: Pointer,
        id: String, adbPath: String,
        host: String
    ) : this(
        instance,
        id,
        adbPath,
        host
    ) {
        this.pointer = pointer
    }

    lateinit var pointer: Pointer

    override fun callback(msg: Int, detail_json: String?, custom_arg: String?) {
        System.out.printf("回调msg : %s , 回调 detail_json : %s ,回调 custom_arg : %s \n", msg, detail_json, custom_arg)
    }

    fun connect(): Boolean = instance.AsstConnect(pointer, adbPath, host, "");

    fun appendTask(type: String, params: String) = instance.AsstAppendTask(pointer, type, params)

    fun start() = instance.AsstStart(pointer)

    fun stop() = instance.AsstStop(pointer)

}
