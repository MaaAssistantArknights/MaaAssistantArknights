import i18n from "i18next";
import { initReactI18next } from "react-i18next";
import LanguageDetector from "i18next-browser-languagedetector";

import zh_cn from "./locales/zh-cn.json";
import en_us from "./locales/en-us.json";

i18n
    .use(LanguageDetector) // 自动检测浏览器语言
    .use(initReactI18next)
    .init({
        resources: {
            zh_cn: { translation: zh_cn },
            en_us: { translation: en_us },
        },
        fallbackLng: "zh_cn", // 找不到翻译时默认中文
        interpolation: {
            escapeValue: false, // React 已经处理 XSS，不需要 escape
        },
    });

export default i18n;
