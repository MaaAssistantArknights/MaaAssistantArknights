import { motion } from 'framer-motion'
import { FC } from 'react'

import { useTheme } from '@/contexts/ThemeContext'

import { Trans, useTranslation } from "react-i18next";

export const HomeHeroHeader: FC = () => {
  const { t, i18n } = useTranslation();
  const { theme } = useTheme();

  return (
    <motion.div
      initial={{ opacity: 0, y: 30, filter: 'blur(10px)' }}
      animate={{ opacity: 1, y: 0, filter: 'blur(0px)' }}
      transition={{ duration: 0.8, ease: 'easeOut', delay: 0.3 }}
    >
      <header className={`h-full w-full pt-24 md:pt-[10vh] ${theme === 'dark' ? 'text-white opacity-80' : 'text-gray-800 opacity-90'}`}>
        <div className="flex flex-col items-center justify-center px-4">
          <motion.h1
            className="text-3xl md:text-5xl font-logo font-light mb-4 select-none"
            initial={{ opacity: 0, scale: 0.8 }}
            animate={{ opacity: 1, scale: 1 }}
            transition={{ duration: 0.6, ease: 'easeOut', delay: 0.5 }}
          >
            MaaAssistantArknights
          </motion.h1>
          <motion.p
            className="text-sm md:text-base px-4 text-center"
            initial={{ opacity: 0, x: -20 }}
            animate={{ opacity: 1, x: 0 }}
            transition={{ duration: 0.6, ease: 'easeOut', delay: 0.7 }}
          >
            <Trans key={i18n.language} i18nKey="header.description" components={{
              1: <motion.span
                className="whitespace-nowrap"
                initial={{ opacity: 0, y: 10, rotate: -10 }}
                animate={{ opacity: 1, y: 0, rotate: 0 }}
                transition={{ duration: 0.5, ease: 'easeOut', delay: 0.8 }}
              />,
              2: <motion.span
                className="whitespace-nowrap"
                initial={{ opacity: 0, y: 10, rotate: 10 }}
                animate={{ opacity: 1, y: 0, rotate: 0 }}
                transition={{ duration: 0.5, ease: 'easeOut', delay: 1.0 }}
              />,
              3: <motion.span
                className="whitespace-nowrap"
                initial={{ opacity: 0, y: 10, scale: 0.8 }}
                animate={{ opacity: 1, y: 0, scale: 1 }}
                transition={{ duration: 0.5, ease: 'easeOut', delay: 1.2 }}
              />
            }} />
          </motion.p>
        </div>
      </header>
    </motion.div>
  )
}
