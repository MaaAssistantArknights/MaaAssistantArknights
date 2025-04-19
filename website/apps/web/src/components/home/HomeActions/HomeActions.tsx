import { GlowButton } from '@/components/foundations/GlowButton/GlowButton'
import {
  DownloadState,
  HomeActionsRelease,
  HomeActionsReleaseErrorBoundary,
} from '@/components/home/HomeActionsRelease/HomeActionsRelease'
import mdiDocument from '@iconify/icons-mdi/document'
import mdiGitHub from '@iconify/icons-mdi/github'
import mdiQqchat from '@iconify/icons-mdi/qqchat'
import mdiMinidisc from '@iconify/icons-mdi/minidisc'
import mdiLoading from '@iconify/icons-mdi/loading'
import { Icon } from '@iconify/react'

import { motion } from 'framer-motion'
import { FC, Suspense } from 'react'

export const HomeActions: FC = () => {
  return (
    <div className="absolute bottom-0 left-0 right-0 mb-24 md:mb-[7vh] flex flex-col mx-8">
      <motion.div
        className="flex-col items-center justify-center hidden gap-2 font-light md:flex md:flex-row md:gap-x-4 md:gap-y-2 flex-wrap relative"
        // layout
        layoutRoot
      >
        <HomeActionsReleaseErrorBoundary>
          <Suspense
            fallback={
              <DownloadState
                iconClassName="animate-spin"
                icon={mdiLoading}
                title = ""
              />
            }
          >
            <HomeActionsRelease />
          </Suspense>
        </HomeActionsReleaseErrorBoundary>
      </motion.div>

      <div className="flex-row gap-4 items-center justify-center mt-4 flex flex-col md:flex-row">
        <GlowButton translucent href="/docs">
          <div className="flex items-center -ml-1 font-light">
            <Icon icon={mdiDocument} fontSize="30px" />
            <span className="ml-2">使用文档</span>
          </div>
        </GlowButton>
        
        <GlowButton translucent href="https://ota.maa.plus/MaaAssistantArknights/api/qqgroup">
          <div className="flex items-center -ml-1 text-sm">
            <Icon icon={mdiQqchat} fontSize="20px" />
            <span className="ml-2">QQ 群</span>
          </div>
        </GlowButton>
        
        <GlowButton translucent href="https://discord.gg/23DfZ9uA4V">
          <div className="flex items-center -ml-1 text-sm">
            <Icon icon={mdiMinidisc} fontSize="20px" />
            <span className="ml-2">Discord</span>
          </div>
        </GlowButton>

        <GlowButton translucent href="https://github.com/MaaAssistantArknights/MaaAssistantArknights">
          <div className="flex items-center -ml-1 text-sm">
            <Icon icon={mdiGitHub} fontSize="20px" />
            <span className="ml-2">GitHub</span>
          </div>
        </GlowButton>
      </div>

      <div className="mt-6 text-xs leading-5 text-center md:mt-8 dark:text-white/70 text-black/70 transition-colors duration-300">
        MAA 以 AGPL-3.0 协议开源；使用即表示您同意并知悉「用户协议」的相关内容。
      </div>
    </div>
  )
}
