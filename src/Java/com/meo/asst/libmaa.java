package com.meo.asst;

public class libmaa {

    static {
        // System.loadLibrary("MeoAssistant");
        System.loadLibrary("MeoAssistantJni");
    }

    public static native long create(String dirname);
    public static native void destroy(long handle);
}
