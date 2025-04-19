import linksIconArkNights from '@/assets/links/ark-nights.com.png?url'
import linksIconPenguinStats from '@/assets/links/penguin-stats.png?url'
import linksIconPrtsPlus from '@/assets/links/prts.plus.png?url'
import linksIconYituliu from '@/assets/links/yituliu.site.png?url'
import linksIconArkntools from '@/assets/links/arkntools.app.png?url'
import chevronRight from '@iconify/icons-mdi/chevron-right'
import mdiGitHub from '@iconify/icons-mdi/github'
import { Icon } from '@iconify/react'

import clsx from 'clsx'
import { FC, ReactNode, forwardRef, useEffect } from 'react'
import { useTheme } from '@/contexts/ThemeContext'

import styles from './HomeLinks.module.css'

interface HomeLinksProps {
  showLinks?: boolean;
  onClose?: () => void; // 添加关闭回调函数
}

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
    key="status"
    href="https://status.annangela.cn/status/maa"
    title="MAA 状态监测"
    icon={
      <img
        src={linksIconPrtsPlus}
        alt="MAA 状态监测"
        className="h-6 w-6"
      />
    }
  />,
]

export const HomeLinks = forwardRef<HTMLDivElement, HomeLinksProps>(({ showLinks = false, onClose }, ref) => {
  const { theme } = useTheme()
  
  // 添加调试日志
  console.log('HomeLinks rendered, showLinks:', showLinks);
  
  // 使用 useEffect 直接响应 showLinks 状态变化
  useEffect(() => {
    console.log('showLinks changed to:', showLinks);
    const panel = ref as React.MutableRefObject<HTMLDivElement | null>;
    if (!panel.current) return;
    
    if (showLinks) {
      panel.current.style.opacity = '1';
      panel.current.style.pointerEvents = 'auto';
    } else {
      panel.current.style.opacity = '0';
      panel.current.style.pointerEvents = 'none';
    }
  }, [showLinks, ref]);
  
  // 添加点击外部关闭的事件监听
  useEffect(() => {
    // 如果未显示链接或没有关闭回调，不需要监听点击事件
    if (!showLinks || !onClose) return;
    
    const handleOutsideClick = (event: MouseEvent) => {
      const panel = ref as React.MutableRefObject<HTMLDivElement | null>;
      if (!panel.current) return;
      
      // 查找友情链接按钮
      const linkButton = document.querySelector('.friend-link-button');
      
      // 如果点击的是面板内部或友情链接按钮，不关闭
      if (panel.current.contains(event.target as Node) || 
          (linkButton && linkButton.contains(event.target as Node))) {
        return;
      }
      
      // 点击了面板和按钮以外的区域，关闭面板
      console.log('点击了面板外部，关闭友情链接');
      onClose();
    };
    
    // 添加点击事件监听，使用捕获阶段避免与其他点击事件冲突
    document.addEventListener('click', handleOutsideClick, true);
    
    return () => {
      document.removeEventListener('click', handleOutsideClick, true);
    };
  }, [ref, showLinks, onClose]);
  
  // 监听友情链接按钮位置变化，相应调整面板位置
  useEffect(() => {
    const positionLinksPanel = () => {
      const panel = ref as React.MutableRefObject<HTMLDivElement | null>;
      if (!panel.current) return;
      
      // 查找友情链接按钮
      const linkButton = document.querySelector('.friend-link-button');
      if (!linkButton) {
        console.log('Friend link button not found!');
        return;
      }
      
      // 获取按钮位置信息
      const buttonRect = linkButton.getBoundingClientRect();
      console.log('Button position:', buttonRect);
      
      if (showLinks) {
        // 计算面板位置 - 在按钮右侧显示
        const buttonRight = buttonRect.right;
        const buttonTop = buttonRect.top;
        
        // 确保面板不超出窗口范围
        const windowHeight = window.innerHeight;
        const windowWidth = window.innerWidth;
        
        // 设置初始高度，稍后再根据内容调整
        panel.current.style.maxHeight = 'none';
        
        // 调整面板最终位置 - 修改为显示在按钮右侧
        panel.current.style.position = 'fixed';
        panel.current.style.left = `${buttonRight + 10}px`; // 距离按钮右侧10px
        panel.current.style.top = `${buttonTop}px`;
        
        // 计算实际高度
        const panelHeight = Math.min(panel.current.scrollHeight, windowHeight * 0.6);
        
        // 确保面板不超出窗口底部
        if (buttonTop + panelHeight > windowHeight - 20) {
          // 如果会超出，调整顶部位置使面板底部不超出窗口
          const newTop = Math.max(20, windowHeight - panelHeight - 20);
          panel.current.style.top = `${newTop}px`;
        }
        
        // 确保面板不超出窗口右侧
        const maxWidth = windowWidth - buttonRight - 30; // 距离窗口右侧至少留20px
        panel.current.style.maxWidth = `${Math.min(400, maxWidth)}px`; // 限制最大宽度
        panel.current.style.maxHeight = `${panelHeight}px`;
        
        // 确保面板可见
        panel.current.style.opacity = '1';
        panel.current.style.pointerEvents = 'auto';
        panel.current.style.transform = 'translateX(0) rotateY(0)';
        
        console.log('Panel positioned and shown');
      } else {
        // 隐藏面板
        panel.current.style.opacity = '0';
        panel.current.style.pointerEvents = 'none';
        panel.current.style.transform = 'translateX(-10px) rotateY(10deg)';
        
        console.log('Panel hidden');
      }
    };
    
    // 初始执行一次定位
    setTimeout(positionLinksPanel, 50); // 添加小延迟确保DOM已更新
    
    // 监听窗口大小变化和滚动事件，重新计算位置
    window.addEventListener('resize', positionLinksPanel);
    window.addEventListener('scroll', positionLinksPanel);
    
    return () => {
      window.removeEventListener('resize', positionLinksPanel);
      window.removeEventListener('scroll', positionLinksPanel);
    };
  }, [ref, showLinks]);
  
  return (
    <div
      ref={ref}
      className={clsx(
        'fixed w-[15vw] min-w-[15rem] max-w-xs rounded-xl transition-all duration-300 z-50',
        theme === 'dark' ? 'text-[#eee] bg-black/80' : 'text-gray-800 bg-white/90',
        styles.root,
        showLinks ? 'visible' : 'invisible' // 使用 CSS 类来控制可见性，作为 JS 控制的备份
      )}
      style={{ opacity: 0 }} // 初始状态为隐藏
    >
      <div className="pt-4 pb-6 px-4">
        <h1 className="text-2xl font-bold mb-3 px-2">
          友情链接
          <span className={`text-sm ml-4 font-bold opacity-80 tracking-wider ${theme === 'dark' ? '' : 'text-gray-700'}`}>
            LINKS
          </span>
        </h1>
        <div className="max-h-[40vh] overflow-y-auto flex flex-col gap-1">{LINKS}</div>
      </div>
    </div>
  )
})
