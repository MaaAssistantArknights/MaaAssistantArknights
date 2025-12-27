import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.toKString

@OptIn(ExperimentalForeignApi::class)
fun main() {
    println("hello kotlin native")
    val v = MaaCore.AsstGetVersion()?.toKString()
    println("version: $v") // version: v5.25.0-beta.4
}
