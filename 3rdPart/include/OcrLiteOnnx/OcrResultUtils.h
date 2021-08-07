#ifdef __JNI__
#ifndef __OCR_RESULT_UTILS_H__
#define __OCR_RESULT_UTILS_H__
#include <jni.h>
#include "OcrStruct.h"

class OcrResultUtils {
public:
    OcrResultUtils(JNIEnv *env, OcrResult &ocrResult);

    ~OcrResultUtils();

    jobject getJObject();

private:
    JNIEnv *jniEnv;
    jobject jOcrResult;

    jclass newJListClass();

    jmethodID getListConstructor(jclass clazz);

    jobject getTextBlock(TextBlock &textBlock);

    jobject getTextBlocks(std::vector<TextBlock> &textBlocks);

    jobject newJPoint(cv::Point &point);

    jobject newJBoxPoint(std::vector<cv::Point> &boxPoint);

    jfloatArray newJScoreArray(std::vector<float> &scores);

};
#endif //__OCR_RESULT_UTILS_H__
#endif
