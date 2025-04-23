import { GlowButton } from '@/components/foundations/GlowButton/GlowButton'
import {
  DownloadState,
  HomeActionsRelease,
  HomeActionsReleaseErrorBoundary,
} from '@/components/home/HomeActionsRelease/HomeActionsRelease'
import mdiDocument from '@iconify/icons-mdi/document'
import mdiGitHub from '@iconify/icons-mdi/github'
import mdiQqchat from '@iconify/icons-mdi/qqchat'
import mdiLoading from '@iconify/icons-mdi/loading'
import mdiLink from '@iconify/icons-mdi/link-variant'
import { Icon } from '@iconify/react'

import { motion } from 'framer-motion'
import { FC, Suspense } from 'react'

interface HomeActionsProps {
  toggleLinks?: () => void;
  showLinks?: boolean;
}

export const HomeActions: FC<HomeActionsProps> = ({ toggleLinks, showLinks }) => {
  // console.log('HomeActions rendered, showLinks:', showLinks);
  
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
            <svg 
              width="20" 
              height="20" 
              viewBox="0 0 127.14 96.36" 
              xmlns="http://www.w3.org/2000/svg"
              className="fill-current"
            >
              <path d="M107.7,8.07A105.15,105.15,0,0,0,81.47,0a72.06,72.06,0,0,0-3.36,6.83A97.68,97.68,0,0,0,49,6.83,72.37,72.37,0,0,0,45.64,0,105.89,105.89,0,0,0,19.39,8.09C2.79,32.65-1.71,56.6.54,80.21h0A105.73,105.73,0,0,0,32.71,96.36,77.7,77.7,0,0,0,39.6,85.25a68.42,68.42,0,0,1-10.85-5.18c.91-.66,1.8-1.34,2.66-2a75.57,75.57,0,0,0,64.32,0c.87.71,1.76,1.39,2.66,2a68.68,68.68,0,0,1-10.87,5.19,77,77,0,0,0,6.89,11.1A105.25,105.25,0,0,0,126.6,80.22h0C129.24,52.84,122.09,29.11,107.7,8.07ZM42.45,65.69C36.18,65.69,31,60,31,53s5-12.74,11.43-12.74S54,46,53.89,53,48.84,65.69,42.45,65.69Zm42.24,0C78.41,65.69,73.25,60,73.25,53s5-12.74,11.44-12.74S96.23,46,96.12,53,91.08,65.69,84.69,65.69Z" />
            </svg>
            <span className="ml-2">Discord</span>
          </div>
        </GlowButton>

        <GlowButton translucent href="https://github.com/MaaAssistantArknights/MaaAssistantArknights">
          <div className="flex items-center -ml-1 text-sm">
            <Icon icon={mdiGitHub} fontSize="20px" />
            <span className="ml-2">GitHub</span>
          </div>
        </GlowButton>
        
        <GlowButton 
          translucent 
          onClick={() => {
            // console.log('Friend link button clicked, current state:', showLinks);
            if (toggleLinks) toggleLinks();
          }}
          className={`friend-link-button ${showLinks ? 'active-link-button' : ''}`}
        >
          <div className="flex items-center -ml-1 text-sm">
            <Icon icon={mdiLink} fontSize="20px" />
            <span className="ml-2">友情链接</span>
          </div>
        </GlowButton>
      </div>
      <div className="mt-6 text-xs leading-5 text-center md:mt-8 dark:text-white/70 text-black/70 transition-colors duration-300">
        <motion.span
          className="whitespace-nowrap"
          initial={{ opacity: 0, x: -10, }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ duration: 0.5, ease: 'easeOut', delay: 1.3 }}
          style={{ display: 'inline-block' }}
        >
          MAA 以&nbsp;
        <a
          href="https://spdx.org/licenses/AGPL-3.0-only.html"
          target="_blank"
          rel="noopener noreferrer"
          className="underline"
        >
          AGPL-3.0
        </a>&nbsp;协议开源
        </motion.span>
        <motion.span
          initial={{ opacity: 0, x: -10 }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ duration: 0.3, ease: 'easeOut', delay: 1.4 }}
          style={{ display: 'inline-block' }}
        >
          ；
        </motion.span>
        <motion.span
          className="whitespace-nowrap"
          initial={{ opacity: 0, x: -10 }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ duration: 0.4, ease: 'easeOut', delay: 1.4 }}
          style={{ display: 'inline-block' }}
        >
          使用即表示您同意并知悉「
        <a
          href="https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/terms-of-service.md"
          target="_blank"
          rel="noopener noreferrer"
          className="underline"
        >
          用户协议
        </a>
        」的相关内容。
        </motion.span>
      </div>
    </div>
  )
}
