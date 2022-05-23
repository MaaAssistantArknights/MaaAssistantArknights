package com.iguigui.maaj.service

import com.iguigui.maaj.easySample.MeoAssistant
import org.springframework.context.annotation.Bean
import org.springframework.stereotype.Component
import org.springframework.stereotype.Service
import java.util.concurrent.atomic.AtomicBoolean

@Service
class MaaService {

    lateinit var meoAssistant: MeoAssistant

    var initFlag: AtomicBoolean = AtomicBoolean(false)

    fun loadSource() {

    }

}
