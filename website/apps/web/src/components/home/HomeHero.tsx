import { PerformanceMonitor } from '@react-three/drei'
import { Canvas } from '@react-three/fiber'
import { ErrorBoundary } from '@sentry/react'
import { motion } from 'framer-motion'

import { FC, useRef, useState } from 'react'
import { useWindowSize } from 'react-use'
import { useTheme } from '@/contexts/ThemeContext'

import { AnimatedBlobs } from './AnimatedBlobs/AnimatedBlobs'
import { HomeActions } from './HomeActions/HomeActions'
import { HomeHeroHeader } from './HomeHeroHeader/HomeHeroHeader'
import { HomeLinks } from './HomeLinks/HomeLinks'
import { Screenshots } from './Screenshots/Screenshots'

export const HomeHero: FC = () => {
  const linkRef = useRef<HTMLDivElement | null>(null)
  const indicatorRef = useRef<HTMLDivElement | null>(null)
  const windowDimensions = useWindowSize()
  const { theme } = useTheme()
  // 添加控制友情链接显示的状态
  const [showLinks, setShowLinks] = useState(false)

  return (
    <div key={theme}>
      <AnimatedBlobs />
      <motion.div
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
                  <div className={`${theme === 'dark' ? 'text-white/50' : 'text-gray-800/70'} text-sm font-semibold`}>
                    MAA 截图渲染失败；是否禁用了 GPU 加速？
                  </div>
                </div>
              }
              onError={() => {
                if (linkRef.current) linkRef.current.style.opacity = '1'
                if (indicatorRef.current)
                  indicatorRef.current.style.opacity = '0'
              }}
            >
              <ScreenshotsCanvas
                sidebarRef={linkRef}
                indicatorRef={indicatorRef}
                showLinks={showLinks}
              />
            </ErrorBoundary>
          )}
        </section>
      </motion.div>
      
      <HomeHeroHeader />

      <HomeActions toggleLinks={() => setShowLinks(!showLinks)} showLinks={showLinks} />
      <HomeLinks 
        ref={linkRef} 
        showLinks={showLinks} 
        onClose={() => setShowLinks(false)} 
      />
      {/* 移除指示器组件，不再需要通过鼠标触发友情链接 */}
    </div>
  )
}

function ScreenshotsCanvas({
  sidebarRef,
  indicatorRef,
  showLinks = false,
}: {
  sidebarRef: React.MutableRefObject<HTMLDivElement | null>
  indicatorRef: React.MutableRefObject<HTMLDivElement | null>
  showLinks?: boolean
}) {
  const [dpr, setDpr] = useState(1.5)
  return (
    <Canvas camera={{ fov: 35, position: [0, -1, 14] }} flat linear dpr={dpr}>
      <PerformanceMonitor
        onIncline={() => setDpr(2)}
        onDecline={() => setDpr(1)}
        flipflops={5}
        onFallback={() => setDpr(1)}
      />
      <ambientLight intensity={1} />
      <Screenshots sidebarRef={sidebarRef} indicatorRef={indicatorRef} showLinks={showLinks} />
    </Canvas>
  )
}
