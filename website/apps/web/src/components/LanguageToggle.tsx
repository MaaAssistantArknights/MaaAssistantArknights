import React, { useState, useRef, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { useTranslation } from 'react-i18next'
import { languages, getLanguageOption } from '@/i18n'

export const LanguageToggle: React.FC = () => {
  const { i18n } = useTranslation()
  const [open, setOpen] = useState(false)
  const dropdownRef = useRef<HTMLDivElement>(null)

  const toggleDropdown = () => setOpen((prev) => !prev)
  const changeLanguage = (lng: string) => {
    i18n.changeLanguage(lng)
    setOpen(false)
  }

  // 点击外部关闭下拉
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (
        dropdownRef.current &&
        !dropdownRef.current.contains(event.target as Node)
      ) {
        setOpen(false)
      }
    }
    document.addEventListener('mousedown', handleClickOutside)
    return () => document.removeEventListener('mousedown', handleClickOutside)
  }, [])

  return (
    <motion.div ref={dropdownRef} className="relative">
      {/* 按钮 */}
      <motion.button
        lang={getLanguageOption(i18n.language).htmlLang}
        onClick={toggleDropdown}
        className="px-4 py-2 rounded-lg border backdrop-blur-sm shadow-lg
                   dark:bg-slate-900/90 bg-stone-100/90
                   text-gray-800 dark:text-white/90
                   border-white/20 dark:border-white/20
                   hover:bg-stone-200/90 dark:hover:bg-slate-800/90
                   active:bg-stone-300/90 dark:active:bg-slate-700/90
                   hover:-translate-y-[1px] active:translate-y-[1px]
                   transition-all duration-200"
        whileTap={{ scale: 0.95 }}
        whileHover={{
          scale: 1.05,
          boxShadow: '0 5px 25px rgba(100, 100, 255, 0.15)',
        }}
        style={{
          boxShadow: '0 0 15px rgba(100, 100, 255, 0.1)',
        }}
      >
        {getLanguageOption(i18n.language).label}
      </motion.button>

      {/* 下拉菜单 */}
      <AnimatePresence>
        {open && (
          <motion.div
            key="dropdown"
            initial={{ opacity: 0, y: -10, scale: 0.95 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, y: -10, scale: 0.95 }}
            transition={{ duration: 0.2, ease: 'easeOut' }}
            className="absolute right-0 mt-2 min-w-[140px] rounded-lg border backdrop-blur-sm shadow-xl z-50
                       dark:bg-slate-900/95 bg-stone-100/95
                       border-white/20 dark:border-white/20
                       text-gray-800 dark:text-white/90"
            style={{
              boxShadow:
                '0 0 0 1px rgba(255, 255, 255, 0.1), 0 0px 4px rgba(0, 0, 0, 0.3), 0 0px 24px rgba(0, 0, 0, 0.2)',
            }}
          >
            {languages.map((lang) => (
              <motion.button
                lang={lang.htmlLang}
                key={lang.code}
                onClick={() => changeLanguage(lang.code)}
                disabled={i18n.language === lang.code}
                className="block w-full px-4 py-3 text-left rounded-lg transition-all duration-200
                           hover:bg-stone-200/80 dark:hover:bg-slate-800/80
                           active:bg-stone-300/80 dark:active:bg-slate-700/80
                           disabled:opacity-50 disabled:cursor-not-allowed
                           first:rounded-t-lg last:rounded-b-lg"
                whileTap={{ scale: 0.98 }}
                whileHover={{ x: 2 }}
              >
                {lang.label}
              </motion.button>
            ))}
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  )
}
