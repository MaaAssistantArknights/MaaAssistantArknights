import linksIconArkNights from '@/assets/links/ark-nights.com.png?url'
import linksIconPenguinStats from '@/assets/links/penguin-stats.png?url'
import linksIconPrtsPlus from '@/assets/links/prts.plus.png?url'
import linksIconYituliu from '@/assets/links/yituliu.site.png?url'
import chevronRight from '@iconify/icons-mdi/chevron-right'
import mdiGitHub from '@iconify/icons-mdi/github'
import { Icon } from '@iconify/react'

import clsx from 'clsx'
import { FC, ReactNode, forwardRef } from 'react'

import styles from './HomeLinks.module.css'

const HomeLink: FC<{
  href: string
  title: ReactNode
  icon?: ReactNode
}> = ({ href, title, icon }) => {
  return (
    <a
      href={href}
      target="_blank"
      rel="noopener noreferrer"
      className="flex items-center space-x-2 text-white/80 hover:text-black/100 bg-black hover:bg-white active:bg-white/60 p-2 rounded-md"
    >
      <div className="text-2xl h-8 w-8 rounded-sm overflow-hidden">{icon}</div>
      <span className="text-xl">{title}</span>
      <div className="flex-1" />
      <Icon icon={chevronRight} className="text-2xl" />
    </a>
  )
}

const LINKS = [
  <HomeLink
    key="penguin-stats"
    href="https://penguin-stats.io"
    title="企鹅物流数据统计"
    icon={
      <img
        src={linksIconPenguinStats}
        alt="企鹅物流数据统计"
        className="h-8 w-8"
      />
    }
  />,
  <HomeLink
    key="ark-nights"
    href="https://ark-nights.com"
    title="Arknights | Planner"
    icon={
      <img
        src={linksIconArkNights}
        alt="Arknights | Planner"
        className="h-8 w-8"
      />
    }
  />,
  <HomeLink
    key="yituliu"
    href="https://ark.yituliu.cn/"
    title="明日方舟一图流"
    icon={
      <img src={linksIconYituliu} alt="明日方舟一图流" className="h-8 w-8" />
    }
  />,
  <HomeLink
    key="alas"
    href="https://github.com/LmeSzinc/AzurLaneAutoScript"
    title="AzurLaneAutoScript (ALAS)"
    icon={<Icon icon={mdiGitHub} className="h-8 w-8" />}
  />,
  <HomeLink
    key="prts-plus"
    href="https://prts.plus"
    title="MAA Copilot 作业分享站"
    icon={
      <img
        src={linksIconPrtsPlus}
        alt="MAA Copilot 作业分享站"
        className="h-8 w-8"
      />
    }
  />,
  <HomeLink
    key="QQ"
    href="https://ota.maa.plus/MaaAssistantArknights/api/qqgroup"
    title="MAA 用户群"
    icon={
      <img
        src={linksIconPrtsPlus}
        alt="MAA 用户群"
        className="h-8 w-8"
      />
    }
  />,
]

export const HomeLinks = forwardRef<HTMLDivElement>((_props, ref) => (
  <div
    ref={ref}
    className={clsx(
      'fixed right-[5vw] top-[15vh] hidden md:flex flex-col p-8 h-[50vh] w-[20vw] min-w-[25rem] text-[#eee] bg-black/80 rounded-xl opacity-0',
      styles.root,
    )}
  >
    <div className="text-md font-bold mb-1 opacity-80 tracking-wider">
      LINKS
    </div>
    <h1 className="text-4xl font-bold mb-4">友情链接</h1>
    <div className="flex-1 overflow-y-auto flex flex-col gap-2">{LINKS}</div>
  </div>
))
