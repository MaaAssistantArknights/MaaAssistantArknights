import { FC } from 'react'

export const HomeHeroHeader: FC = () => {
  return (
    <header className="h-full w-full text-white opacity-80 pt-24 md:pt-[10vh]">
      <div className="flex flex-col items-center justify-center px-4">
        <h1 className="text-3xl md:text-5xl font-logo font-light mb-4 select-none">
          MaaAssistantArknights
        </h1>
        <p className="text-sm md:text-base px-4 text-center">
          <span className="whitespace-nowrap">《明日方舟》小助手，</span>
          <span className="whitespace-nowrap">自动刷图、智能基建换班，</span>
          <span className="whitespace-nowrap">全日常一键长草</span>
        </p>
      </div>
    </header>
  )
}
