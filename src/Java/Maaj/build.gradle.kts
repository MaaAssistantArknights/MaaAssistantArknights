plugins {
    id("java")
}

version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

dependencies {
    //JNA
    implementation("net.java.dev.jna:jna:5.11.0")
}

tasks.getByName<Test>("test") {
    useJUnitPlatform()
}
