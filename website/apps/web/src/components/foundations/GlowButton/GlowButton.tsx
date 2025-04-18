import { FCC } from '@/types'

import clsx from 'clsx'
import { MotionProps, motion } from 'framer-motion'
import { MouseEventHandler, forwardRef } from 'react'

import { WithChildren } from '@/types'
import { useTheme } from '@/contexts/ThemeContext'

import moduleStyles from './GlowButton.module.css'

type GlowButtonProps = WithChildren<{
  translucent?: boolean
  bordered?: boolean
  href?: string
  onClick?: MouseEventHandler<HTMLButtonElement>
}>

export const GlowButton: FCC<GlowButtonProps> = forwardRef<
  HTMLButtonElement,
  GlowButtonProps
>(({ children, translucent, bordered, href, onClick }, ref) => {
  const { theme } = useTheme();

  const motionConfig: MotionProps = {
    whileHover: {
      scale: 1.05,
      backgroundColor: theme === 'dark'
        ? (translucent ? 'rgba(39, 39, 42, 0.6)' : 'rgba(39, 39, 42, 0.8)')
        : (translucent ? 'rgba(229, 229, 229, 0.6)' : 'rgba(229, 229, 229, 0.8)')
    },
    whileTap: {
      scale: 0.95,
      backgroundColor: theme === 'dark'
        ? 'rgba(39, 39, 42, 0.7)'
        : 'rgba(229, 229, 229, 0.7)',
    },
    exit: {
      scale: 0.4,
      opacity: 0,
      filter: 'blur(10px)',
    },
    initial: {
      scale: 0.4,
      opacity: 0,
      filter: 'blur(10px)',
    },
    animate: {
      scale: 1,
      opacity: 1,
      filter: 'blur(0px)',
    },
    transition: {
      type: 'spring',
      stiffness: 200,
      damping: 30,
      mass: 0.8,
      background: { type: 'tween', duration: 0.2 },
      color: { type: 'tween', duration: 0.2 },
    },
  }

  const inner = (
    <motion.button
      layout
      type="button"
      className={clsx(
        moduleStyles.root,
        !translucent && 'dark:bg-slate-900/90 bg-stone-100/90',
        translucent && 'dark:bg-slate-900/90 bg-stone-100/90',
        !bordered && 'border-none',
        'flex px-6 py-3 dark:active:bg-slate-800 active:bg-stone-200 rounded-lg hover:-translate-y-[1px] active:translate-y-[1px] text-2xl dark:text-white/90 text-stone-800 whitespace-nowrap transition-colors transition-transform transition-all duration-200',
      )}
      onClick={onClick}
      {...motionConfig}
      key={theme}
      ref={ref}
    >
      {children}
    </motion.button>
  )

  if (href) {
    return (
      <a
        href={href}
        target="_blank"
        rel="noreferrer noopener"
        className={moduleStyles.link}
      >
        {inner}
      </a>
    )
  }
  return inner
})
