import { FCC } from '@/types'

import clsx from 'clsx'
import { MotionProps, motion } from 'framer-motion'
import { MouseEventHandler, forwardRef } from 'react'

import { WithChildren } from '../../../types'

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
  const motionConfig: MotionProps = {
    whileHover: {
      scale: 1.03,
    },
    whileTap: {
      scale: 0.97,
    },
    exit: {
      scale: 0.4,
      opacity: 0,
    },
    initial: {
      scale: 0.4,
      opacity: 0,
    },
    animate: {
      scale: 1,
      opacity: 1,
    },
    transition: {
      type: 'spring',
      stiffness: 400,
      damping: 30,
      mass: 1.2,
    },
  }

  const inner = (
    <motion.button
      layout
      type="button"
      className={clsx(
        moduleStyles.root,
        !translucent && 'bg-zinc-900/80',
        translucent && 'bg-zinc-900/80 hover:bg-zinc-700/40',
        !bordered && 'border-none',
        'flex px-6 py-3 active:bg-zinc-800 rounded-lg hover:-translate-y-[1px] active:translate-y-[1px] text-2xl text-white/80 whitespace-nowrap',
      )}
      onClick={onClick}
      {...motionConfig}
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
