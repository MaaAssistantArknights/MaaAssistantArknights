package com.iguigui.maaj.service

import com.iguigui.maaj.dto.*
import com.iguigui.maaj.easySample.MaaCore
import com.iguigui.maaj.logger
import com.sun.jna.Native
import io.ktor.websocket.*
import kotlinx.coroutines.runBlocking
import java.io.File
import java.security.MessageDigest
import java.util.*
import java.util.concurrent.ConcurrentHashMap


object MaaService {

    private var instancePool: ConcurrentHashMap<String, MaaInstance> = ConcurrentHashMap()

    private val wsConnection = Collections.synchronizedSet<Connection?>(LinkedHashSet())

    val maaCore: MaaCore by lazy {
        var maaPath = File(File("").absolutePath).parent
        logger.info("maaPath $maaPath")
//        maaPath = "C:\\Users\\atmzx\\Desktop\\MaaCoreArknights3"
        System.setProperty("jna.library.path", maaPath)
        val load = Native.load("MaaCore", MaaCore::class.java)
        load.AsstLoadResource(maaPath)
        load
    }

    fun connect(adbPath: String, host: String, detailJson: String): ConnectResponse {
        val id = sha1(host)
//        if (instancePool.containsKey(id)) {
//            return ConnectResponse(id, true)
//        }
        val maaInstance = MaaInstance(maaCore, id, adbPath, host, detailJson, ::callBackLog)
        maaInstance.pointer = maaCore.AsstCreateEx(maaInstance, maaInstance.id)
        val connect = maaInstance.connect()
        if (!connect) {
            return ConnectResponse("", false)
        }
        instancePool[id] = maaInstance
        return ConnectResponse(id, true)
    }

    fun appendTask(id: String, type: String, detailJson: String) =
        instancePool[id]?.appendTask(type, detailJson) ?: 0


    fun setTaskParams(id: String, taskId: Int, detailJson: String) =
        instancePool[id]?.setTaskParams(taskId, detailJson) ?: false

    fun start(id: String) = instancePool[id]?.start() ?: false

    fun stop(id: String) = instancePool[id]?.stop() ?: false

    fun getVersion(): String = maaCore.AsstGetVersion()

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
        instancePool.values.map { e -> MaaInstanceInfo(e.id, e.host, e.adbPath, e.uuid, e.status) }.toList()

    fun destroy(id: String) {
        instancePool[id]?.destroy()
        instancePool.remove(id)
    }


    fun callBackLog(message: CallBackLog) {
        runBlocking {
            wsConnection.forEach { it.session.send(message.wapperToWsResponse("callBack", 0).toJsonString()) }
        }
    }

    fun addConnection(connection: Connection) {
        this.wsConnection += connection
    }

    fun removeConnection(connection: Connection) {
        this.wsConnection -= connection
    }

    fun getImage(id: String): ByteArray? {
        return instancePool[id]?.getImage()
    }

}
