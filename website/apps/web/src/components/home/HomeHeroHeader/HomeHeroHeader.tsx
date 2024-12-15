import { motion } from 'framer-motion'
import { FC } from 'react'

export const HomeHeroHeader: FC = () => {
  return (
    <motion.div
      initial={{ opacity: 0, y: 30, filter: 'blur(10px)' }}
      animate={{ opacity: 1, y: 0, filter: 'blur(0px)' }}
      transition={{ duration: 0.8, ease: 'easeOut', delay: 0.3 }}
    >
      <header className="h-full w-full text-white opacity-80 pt-24 md:pt-[10vh]">
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
            <motion.span
              className="whitespace-nowrap"
              initial={{ opacity: 0, y: 10, rotate: -10 }}
              animate={{ opacity: 1, y: 0, rotate: 0 }}
              transition={{ duration: 0.5, ease: 'easeOut', delay: 0.8 }}
            >
              《明日方舟》小助手，
            </motion.span>
            <motion.span
              className="whitespace-nowrap"
              initial={{ opacity: 0, y: 10, rotate: 10 }}
              animate={{ opacity: 1, y: 0, rotate: 0 }}
              transition={{ duration: 0.5, ease: 'easeOut', delay: 1.0 }}
            >
              自动刷图、智能基建换班，
            </motion.span>
            <motion.span
              className="whitespace-nowrap"
              initial={{ opacity: 0, y: 10, scale: 0.8 }}
              animate={{ opacity: 1, y: 0, scale: 1 }}
              transition={{ duration: 0.5, ease: 'easeOut', delay: 1.2 }}
            >
              全日常一键长草
            </motion.span>
          </motion.p>
        </div>
      </header>
    </motion.div>
  )
}
