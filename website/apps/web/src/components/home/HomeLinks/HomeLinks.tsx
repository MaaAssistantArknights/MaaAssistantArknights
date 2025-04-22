import linksIconArkNights from '@/assets/links/ark-nights.com.png?url'
import linksIconPenguinStats from '@/assets/links/penguin-stats.png?url'
import linksIconPrtsPlus from '@/assets/links/prts.plus.png?url'
import linksIconYituliu from '@/assets/links/yituliu.site.png?url'
import linksIconArkntools from '@/assets/links/arkntools.app.png?url'
import linksIconMirrorChyan from '@/assets/links/mirrorc.png?url'
import chevronRight from '@iconify/icons-mdi/chevron-right'
import mdiGitHub from '@iconify/icons-mdi/github'
import { Icon } from '@iconify/react'

import clsx from 'clsx'
import { FC, ReactNode, forwardRef, useEffect } from 'react'
import { useTheme } from '@/contexts/ThemeContext'

import styles from './HomeLinks.module.css'
import React from 'react'

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
      className={`flex items-center p-2 rounded-md transition-all duration-300 ${theme === 'dark' 
        ? 'text-white/80 bg-black hover:text-black/100 hover:bg-white active:bg-white/60' 
        : 'text-black/80 bg-white hover:text-white/100 hover:bg-black active:bg-black/60'}`}
    >
      <div className="text-lg h-6 w-6 rounded-sm overflow-hidden flex-shrink-0 mr-2">{icon}</div>
      <span className="text-base whitespace-nowrap">{title}</span>
      <div className="flex-1 min-w-[10px]" />
      <Icon icon={chevronRight} className="text-lg flex-shrink-0" />
    </a>
  )
}

const LINKS = [
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
  <HomeLink
    key="mirrorc"
    href="https://mirrorchyan.com/zh/projects"
    title="Mirror酱"
    icon={
      <img
        src={linksIconMirrorChyan}
        alt="Mirror酱"
        className="h-6 w-6"
      />
    }
  />,
  <HomeLink
    key="penguin-stats"
    href="https://penguin-stats.cn/"
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
    key="alas"
    href="https://alas.azurlane.cloud/"
    title="ALAS"
    icon={<Icon icon={mdiGitHub} className="h-6 w-6" />}
  />,
]

export const HomeLinks = forwardRef<HTMLDivElement, HomeLinksProps>(({ showLinks = false, onClose }, ref) => {
  const { theme } = useTheme()
  // 添加一个引用变量来跟踪面板的定位状态
  const isPanelCenteredRef = React.useRef(false);
  
  // console.log('HomeLinks rendered, showLinks:', showLinks);
  
  // 使用 useEffect 直接响应 showLinks 状态变化
  useEffect(() => {
    // console.log('showLinks changed to:', showLinks);
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
      
      // 查找友链按钮
      const linkButton = document.querySelector('.friend-link-button');
      
      // 如果点击的是面板内部或友链按钮，不关闭
      if (panel.current.contains(event.target as Node) || 
          (linkButton && linkButton.contains(event.target as Node))) {
        return;
      }
      
      // 点击了面板和按钮以外的区域，关闭面板
      // console.log('Clicked outside the panel, close the friend link');
      onClose();
    };
    
    // 添加点击事件监听，使用捕获阶段避免与其他点击事件冲突
    document.addEventListener('click', handleOutsideClick, true);
    
    return () => {
      document.removeEventListener('click', handleOutsideClick, true);
    };
  }, [ref, showLinks, onClose]);
  
  // 监听友链按钮位置变化，相应调整面板位置
  useEffect(() => {
    const positionLinksPanel = () => {
      const panel = ref as React.MutableRefObject<HTMLDivElement | null>;
      if (!panel.current) return;
      
      // 查找友链按钮
      const linkButton = document.querySelector('.friend-link-button');
      if (!linkButton) {
        console.error('Friend link button not found!');
        return;
      }
      
      // 获取按钮位置信息
      const buttonRect = linkButton.getBoundingClientRect();
      
      if (showLinks) {
        // 设置初始高度，稍后再根据内容调整
        panel.current.style.maxHeight = 'none';
        panel.current.style.width = 'auto'; // 先设为自动宽度来测量内容宽度
        
        // 确保面板不超出窗口范围
        const windowHeight = window.innerHeight;
        const windowWidth = window.innerWidth;
        
        // 计算面板实际需要的宽度和高度
        // 先设置一个临时的宽度，让内容能够正常布局
        panel.current.style.maxWidth = '480px';
        
        // 强制浏览器重新计算布局
        void panel.current.offsetWidth;
        
        // 获取内容实际宽度（加上一些内边距）
        const contentWidth = Math.min(
          Math.max(panel.current.scrollWidth, 240), // 至少240px宽，最多不超过面板宽度
          480 // 最大宽度限制为480px
        );
        
        const panelHeight = Math.min(panel.current.scrollHeight, windowHeight * 0.6);
        
        // 计算面板在按钮右侧显示时是否会超出窗口右边缘
        const buttonRight = buttonRect.right;
        const buttonTop = buttonRect.top;
        const spaceOnRight = windowWidth - buttonRight - 20; // 右侧可用空间（减去20px边距）
        
        // 判断是否有足够空间在按钮右侧显示
        const isSufficientSpaceOnRight = spaceOnRight >= contentWidth;
        
        // 如果右侧空间不足则居中显示
        const shouldCenterPanel = !isSufficientSpaceOnRight;
        
        // 更新面板定位状态
        isPanelCenteredRef.current = shouldCenterPanel;
        
        if (shouldCenterPanel) {
          // 居中显示
          panel.current.style.position = 'fixed';
          panel.current.style.left = '50%';
          panel.current.style.top = '50%';
          panel.current.style.transform = 'translate(-50%, -50%)';
          
            // 设置宽度为内容实际需要的宽度
            const panelWidth = Math.min(windowWidth * 0.9, contentWidth);
          
          panel.current.style.width = `${panelWidth}px`;
          panel.current.style.maxHeight = `${Math.min(panelHeight, windowHeight * 0.8)}px`;
        } else {
          // 在按钮右侧显示
          panel.current.style.position = 'fixed';
          panel.current.style.left = `${buttonRight + 10}px`;
          panel.current.style.top = `${buttonTop}px`;
          panel.current.style.transform = 'translateX(0) rotateY(0)';
          
          // 设置宽度为内容实际需要的宽度，但不超过右侧可用空间
          panel.current.style.width = `${Math.min(contentWidth, spaceOnRight)}px`;
          
          // 确保面板不超出窗口底部
          if (buttonTop + panelHeight > windowHeight - 20) {
            // 调整顶部位置使面板底部不超出窗口
            const newTop = Math.max(20, windowHeight - panelHeight - 20);
            panel.current.style.top = `${newTop}px`;
          }
          
          panel.current.style.maxHeight = `${panelHeight}px`;
        }
        
        // 确保面板可见
        panel.current.style.opacity = '1';
        panel.current.style.pointerEvents = 'auto';
        
        // console.log(`Panel positioned ${shouldCenterPanel ? 'centered' : 'at right'}, content width: ${contentWidth}px`);
      } else {
        // 隐藏面板
        panel.current.style.opacity = '0';
        panel.current.style.pointerEvents = 'none';
        
        // 根据面板位置应用不同的隐藏动画
        if (isPanelCenteredRef.current) {
          // 居中时向下纵向淡出
          panel.current.style.transform = 'translate(-50%, -50%) translateY(10px) rotateX(5deg)';
        } else {
          // 在右侧时向左横向淡出
          panel.current.style.transform = 'translateX(-10px) rotateY(10deg)';
        }
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
        'fixed min-w-[15rem] rounded-xl transition-all duration-300 z-50',
        theme === 'dark' ? 'text-[#eee] bg-black/80' : 'text-gray-800 bg-white/90',
        styles.root,
        showLinks ? 'visible' : 'invisible' // 使用 CSS 类来控制可见性，作为 JS 控制的备份
      )}
      style={{ opacity: 0 }} // 初始状态为隐藏
    >
      <div className="pt-4 pb-4 px-4">
        <h1 className="text-2xl font-bold mb-3 px-2">
          友情链接
          <span className={`text-sm ml-4 font-bold opacity-80 tracking-wider ${theme === 'dark' ? '' : 'text-gray-700'}`}>
            LINKS
          </span>
        </h1>
        <div className="max-h-[50vh] overflow-y-auto flex flex-col gap-2 pb-2">{LINKS}</div>
      </div>
    </div>
  )
})
