import { useTheme } from '@/contexts/ThemeContext'
import { Canvas } from '@react-three/fiber'
import { ErrorBoundary } from '@sentry/react'

import { motion } from 'framer-motion'
import { FC, useRef, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { useWindowSize } from 'react-use'

import { AnimatedBlobs } from './AnimatedBlobs/AnimatedBlobs'
import { HomeActions } from './HomeActions/HomeActions'
import { HomeHeroHeader } from './HomeHeroHeader/HomeHeroHeader'
import { HomeLinks } from './HomeLinks/HomeLinks'
import { Screenshots } from './Screenshots/Screenshots'

export const HomeHero: FC = () => {
  const { t, i18n } = useTranslation()

  const linkRef = useRef<HTMLDivElement | null>(null)
  const indicatorRef = useRef<HTMLDivElement | null>(null)
  const windowDimensions = useWindowSize()
  const { theme } = useTheme()
  const [showLinks, setShowLinks] = useState(false)

  return (
    <div key={theme}>
      <AnimatedBlobs />
      <motion.div
        key={i18n.language}
        className="absolute h-full w-full flex items-center"
        initial={{ opacity: 0, y: 50, filter: 'blur(10px)' }}
        animate={{ opacity: 1, y: 0, filter: 'blur(0px)' }}
        transition={{ duration: 0.8, ease: 'easeOut', delay: 0.2 }}
      >
        <section className="h-[100vmin] w-full relative">
          {windowDimensions.height >= 768 && (
            <ErrorBoundary
              fallback={
                <div className="absolute inset-0 dark:bg-black/50 bg-gray-500/30 flex items-center justify-center">
                  <div
                    className={`${theme === 'dark' ? 'text-white/50' : 'text-gray-800/70'} text-sm font-semibold`}
                  >
                    {t('screenshots.render.failure')}
                  </div>
                </div>
              }
              onError={() => {
                if (linkRef.current) linkRef.current.style.opacity = '1'
              }}
            >
              <ScreenshotsCanvas />
            </ErrorBoundary>
          )}
        </section>
      </motion.div>

      <HomeHeroHeader />

      <HomeActions
        toggleLinks={() => setShowLinks(!showLinks)}
        showLinks={showLinks}
      />
      <HomeLinks
        ref={linkRef}
        showLinks={showLinks}
        onClose={() => setShowLinks(false)}
      />
    </div>
  )
}

function ScreenshotsCanvas() {
  return (
    <Canvas
      camera={{ fov: 35, position: [0, -1, 14] }}
      flat
      linear
      dpr={window.devicePixelRatio || 1.5}
    >
      <ambientLight intensity={1} />
      <Screenshots />
    </Canvas>
  )
}
