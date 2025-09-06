import i18n from "i18next"
import { initReactI18next } from "react-i18next"
import LanguageDetector from "i18next-browser-languagedetector"

import zh_cn from "./locales/zh-cn.json"
import zh_tw from "./locales/zh-tw.json"
import en_us from "./locales/en-us.json"
import ja_jp from "./locales/ja-jp.json"
import ko_kr from "./locales/ko-kr.json"


export interface LanguageOption {
  /** i18n 语言代码 */
  code: string
  /** 前端按钮显示文本 */
  label: string
  /** HTML lang 属性 */
  htmlLang: string
}

/** 所有语言选项 */
export const languages: LanguageOption[] = [
  { code: "zh-CN", label: "简体中文", htmlLang: "zh-CN" },
  { code: "zh-TW", label: "繁體中文", htmlLang: "zh-TW" },
  { code: "en-US", label: "English", htmlLang: "en-US" },
  { code: "ja-JP", label: "日本語", htmlLang: "ja-JP" },
  { code: "ko-KR", label: "한국어", htmlLang: "ko-KR" },
]

/** fallback 配置 */
export const fallback: LanguageOption = {
  code: "zh-CN" as string,
  label: "简体中文" as string,
  htmlLang: "zh-CN" as string,
}

/** 根据 i18n 语言代码获取 LanguageOption */
export const getLanguageOption = (code: string): LanguageOption => {
  return languages.find(l => l.code === code) || fallback
}

/** ===== i18n 初始化 ===== */
const resources = {
  "zh-CN": { translation: zh_cn },
  "zh-TW": { translation: zh_tw },
  "en-US": { translation: en_us },
  "ja-JP": { translation: ja_jp },
  "ko-KR": { translation: ko_kr },
}

i18n
  .use(LanguageDetector) // 自动检测浏览器语言
  .use(initReactI18next)
  .init({
    resources,
    fallbackLng: fallback.code,
    interpolation: {
      escapeValue: false, // React 已经处理 XSS，不需要 escape
    },
  })

export default i18n
