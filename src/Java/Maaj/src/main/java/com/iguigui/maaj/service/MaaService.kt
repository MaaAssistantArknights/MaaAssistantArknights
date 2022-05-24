package com.iguigui.maaj.service

import com.iguigui.maaj.easySample.MeoAssistant
import com.sun.jna.Native
import java.io.File
import java.util.*
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicBoolean

object MaaService {

    private var instancePool: ConcurrentHashMap<String, MaaInstance> = ConcurrentHashMap()

    val meoAssistant: MeoAssistant by lazy {
        val f = File(this.javaClass.getResource("")?.path ?: "")
        var maaPath = f.path
        println(maaPath)
        maaPath = "C:\\Users\\atmzx\\Desktop\\MeoAssistantArknights3"
        System.setProperty("jna.library.path", maaPath)
        val load = Native.load("MeoAssistant", MeoAssistant::class.java)
        load.AsstLoadResource(maaPath)
        load
    }

    fun connect(adbPath: String, host: String, detailJson: String): Boolean {
        if (instancePool.containsKey(host)) {
            return false
        }
        val maaInstance = MaaInstance(meoAssistant, host, adbPath, host)
        maaInstance.pointer = meoAssistant.AsstCreateEx(maaInstance, maaInstance.id)
        maaInstance.connect()
        instancePool.putIfAbsent(host, maaInstance)
        return true
    }

    fun appendTask(host: String,type:String, detailJson: String): Int {
        val appendTask = instancePool[host]?.appendTask(type, detailJson)
        return appendTask ?: 0
    }

    fun start(host: String) = instancePool[host]?.start()

    fun stop(host: String) = instancePool[host]?.stop()

    private fun generatorId() = UUID.randomUUID().toString()

}
