#include <jni.h>

#ifdef __ANDROID__
#include <android/log.h>

#define LOG "maa_jni"
#define LOG_D(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG, __VA_ARGS__)
#define LOG_I(...)  __android_log_print(ANDROID_LOG_INFO, LOG, __VA_ARGS__)
#define LOG_W(...)  __android_log_print(ANDROID_LOG_WARN, LOG, __VA_ARGS__)
#define LOG_E(...)  __android_log_print(ANDROID_LOG_ERROR, LOG, __VA_ARGS__)
#define LOG_F(...)  __android_log_print(ANDROID_LOG_FATAL, LOG, __VA_ARGS__)
#else
#define LOG_D(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#define LOG_F(...)
#endif

#include "AsstCaller.h"

#ifdef __cplusplus
extern "C" {
#endif

    JNIEXPORT jlong JNICALL Java_com_meo_asst_libmaa_create(JNIEnv* env, jclass c, jstring jstr_dirname)
    {
        LOG_D(__FUNCTION__);

        if (env == NULL) {
            return (jlong)NULL;
        }

        const char* cstr_dirname = NULL;
        if (jstr_dirname) {
            cstr_dirname = env->GetStringUTFChars(jstr_dirname, NULL);
        }

        asst::Assistant* p_asst = AsstCreate(cstr_dirname);
        LOG_D("after AsstCreate, p_asst = %p", p_asst);

        if (cstr_dirname) {
            env->ReleaseStringUTFChars(jstr_dirname, cstr_dirname);
        }

        return (jlong)p_asst;
    }

    JNIEXPORT void JNICALL Java_com_meo_asst_libmaa_destroy(JNIEnv* env, jclass c, jlong jlong_asst)
    {
        LOG_D(__FUNCTION__);

        if (env == NULL) {
            return;
        }

        AsstDestroy((asst::Assistant*)jlong_asst);
    }

#ifdef __cplusplus
}
#endif
