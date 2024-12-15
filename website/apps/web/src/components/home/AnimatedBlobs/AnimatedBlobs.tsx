import clsx from 'clsx'
import { motion } from 'framer-motion'

const blobs = [
  ['bg-red-900 translate-y-36', 'left-[30%]'],
  ['bg-orange-900 translate-y-1', 'left-[45%]'],
  ['bg-blue-900 -translate-y-40', 'left-[55%]'],
  ['bg-cyan-900 translate-y-20', 'left-[68%]'],
]

export function AnimatedBlobs() {
  return (
    <div className="absolute h-full w-full overflow-hidden transform-gpu blur-[9rem] left-[-10rem] top-[-10rem]">
      {blobs.map((blob, i) => (
        <div key={i} className={clsx('absolute top-[50vh]', blob[1])}>
          <motion.div
            initial={{ opacity: 0, y: 50, filter: 'blur(10px)' }}
            animate={{ opacity: 1, y: 0, filter: 'blur(0px)' }}
            transition={{ duration: 0.8, ease: 'easeOut', delay: 0.1 }}
          >
            <div className={clsx('h-[20rem] w-[20rem] rounded-full', blob[0])}></div>
          </motion.div>
        </div>
      ))}
    </div>
  )
}
