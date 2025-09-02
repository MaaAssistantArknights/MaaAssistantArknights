import { motion } from 'framer-motion';
import { useEffect } from 'react'
import { useTranslation } from 'react-i18next'
import { HomeHero } from './components/home/HomeHero'
import { ThemeProvider } from './contexts/ThemeContext'
import { ThemeToggle } from './components/ThemeToggle'
import { LanguageToggle } from './components/LanguageToggle'

function App() {
  const { t, i18n } = useTranslation()

  const langMap: Record<string, string> = {
    "zh-CN": "zh-CN",
    "zh-TW": "zh-TW",
    "en-US": "en",
    "ja-JP": "ja",
    "ko-KR": "ko",
  };

  useEffect(() => {
    document.documentElement.lang = langMap[i18n.language] || "zh-CN";

    document.title = t('meta.title')
    const metaDesc = document.querySelector('meta[name="description"]')
    if (metaDesc) {
      metaDesc.setAttribute('content', t('meta.description', { interpolation: { escapeValue: false } }))
    }
  }, [t, i18n])
  return (
    <ThemeProvider>
      <main className="h-full max-h-screen overflow-hidden dark:bg-[#080808] bg-[#f5f5f5] transition-colors duration-300">
        <motion.div
          className="fixed top-4 right-4 flex items-center gap-2 z-50"
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.5, ease: "easeOut" }}
        >
          <LanguageToggle />
          <ThemeToggle />
        </motion.div>
        <section className="h-screen min-h-[20rem] w-full relative">
          <HomeHero />
        </section>
      </main>
    </ThemeProvider>
  )
}

export default App
