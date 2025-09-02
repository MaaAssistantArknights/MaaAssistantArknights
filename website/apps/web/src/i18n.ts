import i18n from "i18next";
import { initReactI18next } from "react-i18next";
import LanguageDetector from "i18next-browser-languagedetector";

import zh_cn from "./locales/zh-cn.json";
import zh_tw from "./locales/zh-tw.json";
import en_us from "./locales/en-us.json";
import ja_jp from "./locales/ja-jp.json";
import ko_kr from "./locales/ko-kr.json";

i18n
    .use(LanguageDetector) // 自动检测浏览器语言
    .use(initReactI18next)
    .init({
        resources: {
            "zh-CN": { translation: zh_cn },
            "zh-TW": { translation: zh_tw },
            "en-US": { translation: en_us },
            "ja-JP": { translation: ja_jp },
            "ko-KR": { translation: ko_kr },
        },
        fallbackLng: "zh-CN", // 找不到翻译时默认中文
        interpolation: {
            escapeValue: false, // React 已经处理 XSS，不需要 escape
        },
    });

export default i18n;
