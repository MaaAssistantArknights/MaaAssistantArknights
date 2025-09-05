import { useEffect, useState, useRef } from "react"

/**
 * useOverflowDetection
 * @param ref 容器 ref
 * @param deps 初始测量依赖，比如语言切换
 * @param measureClass 可选：测量时使用的临时 className（整个 className 字符串）
 */
export function useOverflowDetection<T extends HTMLElement>(
    ref: React.RefObject<T | null>,
    measureClass: string,
    deps: any[] = []
) {
    const [isOverflow, setIsOverflow] = useState(false)
    const contentWidthRef = useRef(0) // 记录初始内容宽度（含 margin）

    useEffect(() => {
        const measure = () => {
            const el = ref.current
            if (!el) return

            const originalClassName = el.className
            if (measureClass) el.className = measureClass

            const parentElement = el.parentElement || el
            const rect = el.getBoundingClientRect()
            const style = window.getComputedStyle(parentElement)
            const marginLeft = parseFloat(style.marginLeft) || 0
            const marginRight = parseFloat(style.marginRight) || 0
            const totalWidth = rect.width + marginLeft + marginRight

            el.className = originalClassName

            contentWidthRef.current = totalWidth
            setIsOverflow(window.innerWidth < totalWidth)
        }

        if (document.readyState === "complete") {
            // 页面已经加载完成，直接测量
            measure()
        } else {
            // 等待 load 事件
            const handleLoad = () => measure()
            window.addEventListener("load", handleLoad)
            return () => window.removeEventListener("load", handleLoad)
        }
    }, deps)

    // 窗口 resize 监听
    useEffect(() => {
        const handleResize = () => {
            setIsOverflow(window.innerWidth < contentWidthRef.current)
        }
        window.addEventListener("resize", handleResize)
        return () => window.removeEventListener("resize", handleResize)
    }, [])

    return isOverflow
}