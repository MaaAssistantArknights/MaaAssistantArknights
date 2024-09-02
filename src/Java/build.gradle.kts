val ktor_version = "2.3.12"
val kotlin_version = "2.0.10"
val logback_version = "1.5.7"
val appName = "Maa-HTTP"

plugins {
    application
    kotlin("jvm") version "2.0.10"
    kotlin("plugin.serialization") version "2.0.10"
    id("edu.sc.seis.launch4j") version "3.0.6"
}

group = "com.iguigui"
version = "0.0.1"
application {
    mainClass.set("com.iguigui.maaj.ApplicationKt")
    val isDevelopment: Boolean = project.ext.has("development")
    applicationDefaultJvmArgs = listOf("-Dio.ktor.development=$isDevelopment")
}

repositories {
    mavenCentral()
    maven { url = uri("https://maven.pkg.jetbrains.space/public/p/ktor/eap") }
}

dependencies {
    implementation("net.java.dev.jna:jna:5.14.0")
    implementation("io.ktor:ktor-server-core-jvm:$ktor_version")
    implementation("io.ktor:ktor-server-netty-jvm:$ktor_version")
    implementation("io.ktor:ktor-server-content-negotiation:$ktor_version")
    implementation("io.ktor:ktor-serialization-kotlinx-json:$ktor_version")
    implementation("io.ktor:ktor-server-cors:$ktor_version")
    implementation("io.ktor:ktor-server-websockets:$ktor_version")
    implementation("ch.qos.logback:logback-classic:$logback_version")
    implementation("io.ktor:ktor-server-call-logging:$ktor_version")
    implementation("io.ktor:ktor-server-double-receive:$ktor_version")
    testImplementation("io.ktor:ktor-server-tests-jvm:$ktor_version")
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit:$kotlin_version")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.7.1")
}



tasks.jar {

    duplicatesStrategy = DuplicatesStrategy.EXCLUDE

    manifest {
        attributes(
            mapOf(
                "Main-Class" to "com.iguigui.maaj.ApplicationKt",
                "Implementation-Title" to "Maa-HTTP",
                "Manifest-Version" to "1.0.0"
            )
        )
    }

    from(configurations.runtimeClasspath.get().map {
        if (it.isDirectory) it else zipTree(it)
    })
    val sourcesMain = sourceSets.main.get()
    sourcesMain.allSource.forEach { println("add from sources: ${it.name}") }
    from(sourcesMain.output)
}

launch4j {
    mainClassName = "com.iguigui.maaj.ApplicationKt"
    icon = "${projectDir}/icons/myApp.ico"
}

tasks.withType<edu.sc.seis.launch4j.tasks.DefaultLaunch4jTask> {
    outfile = "${appName}.exe"
    mainClassName = "com.iguigui.maaj.ApplicationKt"
    icon = "$projectDir/icon.ico"
    productName = "${appName}"
}

tasks.register<edu.sc.seis.launch4j.tasks.Launch4jLibraryTask>("createFastStart") {
    outfile = "${appName}.exe"
    mainClassName = "com.iguigui.maaj.ApplicationKt"
    icon = "$projectDir/icon.ico"
    fileDescription = "The lightning fast implementation"
}
tasks.register<edu.sc.seis.launch4j.tasks.Launch4jLibraryTask>("MyApp-memory") {
    fileDescription = "The default implementation with increased heap size"
    maxHeapPercent = 50
}


tasks.withType<JavaCompile> {
    options.encoding = "UTF-8"
}

val compileKotlin: org.jetbrains.kotlin.gradle.tasks.KotlinCompile by tasks
compileKotlin.kotlinOptions {
    jvmTarget = "1.8"
}


val compileTestKotlin: org.jetbrains.kotlin.gradle.tasks.KotlinCompile by tasks
compileTestKotlin.kotlinOptions {
    jvmTarget = "1.8"
}
