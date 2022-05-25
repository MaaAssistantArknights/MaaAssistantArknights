package com.iguigui.maaj.service

import com.iguigui.maaj.dto.MaaInstanceInfo
import com.iguigui.maaj.easySample.MeoAssistant
import com.sun.jna.Native
import java.io.File
import java.security.MessageDigest
import java.util.concurrent.ConcurrentHashMap


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

    fun connect(adbPath: String, host: String, detailJson: String): String {
        if (instancePool.containsKey(host)) {
            return sha1(host)
        }
        val maaInstance = MaaInstance(meoAssistant, host, adbPath, host)
        maaInstance.pointer = meoAssistant.AsstCreateEx(maaInstance, maaInstance.id)
        maaInstance.connect()
        instancePool.putIfAbsent(host, maaInstance)
        return sha1(host)
    }

    fun appendTask(host: String, type: String, detailJson: String): Int {
        val appendTask = instancePool[host]?.appendTask(type, detailJson)
        return appendTask ?: 0
    }

    fun start(host: String) = instancePool[host]?.start()

    fun stop(host: String) = instancePool[host]?.stop()

    fun getVersion() = "3.9.0"

    private fun sha1(password: String): String {
        val messageDigest = MessageDigest.getInstance("SHA")
        val digests = messageDigest.digest(password.toByteArray())
        val stringBuilder = StringBuilder()
        for (i in digests.indices) {
            var halfbyte = digests[i].toInt() ushr 4 and 0x0F
            for (j in 0..1) {
                stringBuilder.append(
                    if (0 <= halfbyte && halfbyte <= 9) ('0'.code + halfbyte).toChar() else ('a'.code + (halfbyte - 10)).toChar()
                )
                halfbyte = digests[i].toInt() and 0x0F
            }
        }
        return stringBuilder.toString()
    }

    fun listInstance(): List<MaaInstanceInfo> =
        instancePool.values.map { e -> MaaInstanceInfo(e.id, e.host, e.id, e.status) }.toList()

}
