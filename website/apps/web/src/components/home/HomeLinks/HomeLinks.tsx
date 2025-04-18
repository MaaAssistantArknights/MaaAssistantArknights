import linksIconArkNights from '@/assets/links/ark-nights.com.png?url'
import linksIconPenguinStats from '@/assets/links/penguin-stats.png?url'
import linksIconPrtsPlus from '@/assets/links/prts.plus.png?url'
import linksIconYituliu from '@/assets/links/yituliu.site.png?url'
import linksIconArkntools from '@/assets/links/arkntools.app.png?url'
import chevronRight from '@iconify/icons-mdi/chevron-right'
import mdiGitHub from '@iconify/icons-mdi/github'
import { Icon } from '@iconify/react'

import clsx from 'clsx'
import { FC, ReactNode, forwardRef } from 'react'
import { useTheme } from '@/contexts/ThemeContext'

import styles from './HomeLinks.module.css'

const HomeLink: FC<{
  href: string
  title: ReactNode
  icon?: ReactNode
}> = ({ href, title, icon }) => {
  const { theme } = useTheme()
  return (
    <a
      href={href}
      target="_blank"
      rel="noopener noreferrer"
      className={`flex items-center space-x-2 p-2 rounded-md transition-all duration-300 ${theme === 'dark' 
        ? 'text-white/80 bg-black hover:text-black/100 hover:bg-white active:bg-white/60' 
        : 'text-black/80 bg-white hover:text-white/100 hover:bg-black active:bg-black/60'}`}
    >
      <div className="text-lg h-6 w-6 rounded-sm overflow-hidden">{icon}</div>
      <span className="text-base">{title}</span>
      <div className="flex-1" />
      <Icon icon={chevronRight} className="text-lg" />
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
        className="h-6 w-6"
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
        className="h-6 w-6"
      />
    }
  />,
  <HomeLink
    key="yituliu"
    href="https://ark.yituliu.cn/"
    title="明日方舟一图流"
    icon={
      <img src={linksIconYituliu} alt="明日方舟一图流" className="h-6 w-6" />
    }
  />,
  <HomeLink
    key="arkntools"
    href="https://arkntools.app/"
    title="明日方舟工具箱"
    icon={
      <img src={linksIconArkntools} alt="明日方舟工具箱" className="h-6 w-6" />
    }
  />,
  <HomeLink
    key="alas"
    href="https://github.com/LmeSzinc/AzurLaneAutoScript"
    title="ALAS"
    icon={<Icon icon={mdiGitHub} className="h-6 w-6" />}
  />,
  <HomeLink
    key="prts-plus"
    href="https://prts.plus"
    title="PRTS 作业站"
    icon={
      <img
        src={linksIconPrtsPlus}
        alt="PRTS 作业站"
        className="h-6 w-6"
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
        className="h-6 w-6"
      />
    }
  />,
]

export const HomeLinks = forwardRef<HTMLDivElement>((_props, ref) => {
  const { theme } = useTheme()
  
  return (
  <div
    ref={ref}
    className={clsx(
      'fixed right-0 top-[20vh] hidden md:flex flex-col pr-1 pl-6 pt-4 pb-6 h-[45vh] w-[15vw] min-w-[15rem] rounded-xl opacity-0 transition-all duration-200',
      theme === 'dark' ? 'text-[#eee] bg-black/80' : 'text-gray-800 bg-white/90',
      styles.root,
    )}
  >
    <h1 className="text-2xl font-bold mb-3 px-2">
      友情链接
      <span className={`text-sm ml-4 font-bold opacity-80 tracking-wider ${theme === 'dark' ? '' : 'text-gray-700'}`}>
        LINKS
      </span>
    </h1>
    <div className="flex-1 overflow-y-auto flex flex-col gap-1">{LINKS}</div>
  </div>
)
})
