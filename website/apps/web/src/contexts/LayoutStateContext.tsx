import React, {
  createContext,
  useContext,
  useEffect,
  useRef,
  useState,
  RefObject,
} from "react"

import { useTranslation } from "react-i18next"

type LayoutState = {
  isWidthOverflow: boolean
  registerWidthCheck: (ref: RefObject<HTMLElement | null>, measureClass: string) => void
}

const LayoutStateContext = createContext<LayoutState | undefined>(undefined)

export const LayoutStateProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {

  const { t, i18n } = useTranslation()
  const [isWidthOverflow, setIsWidthOverflow] = useState(false)
  const contentWidthRef = useRef(0)
  const targetRef = useRef<RefObject<HTMLElement | null>>(null)
  const targetMeasureClass = useRef<string>("")

  // 测量内容宽度，
  // 先临时将class设为传入值（通常是使得这个元素最宽的值），
  // 然后测量该元素的宽度加父元素的margin（目前写死，以后可能改得通用些）
  const measure = () => {
    const ref = targetRef.current
    const el = ref?.current
    if (!el) return

    const originalClass = el.className
    if (targetMeasureClass.current) {
      el.className = targetMeasureClass.current
    }

    const rect = el.getBoundingClientRect()
    const style = window.getComputedStyle(el.parentElement || el)
    const margin =
      (parseFloat(style.marginLeft) || 0) + (parseFloat(style.marginRight) || 0)
    const totalWidth = rect.width + margin

    el.className = originalClass

    contentWidthRef.current = totalWidth
    setIsWidthOverflow(window.innerWidth < totalWidth)
  }

  const registerWidthCheck = (ref: RefObject<HTMLElement | null>, measureClass: string) => {
    targetRef.current = ref
    targetMeasureClass.current = measureClass
    measure()
  }

  // 初始加载和切换语言时，更新宽度
  useEffect(() => {
    if (document.readyState === "complete") {
      measure()
    } else {
      const handleLoad = () => measure()
      window.addEventListener("load", handleLoad)
      return () => window.removeEventListener("load", handleLoad)
    }
  }, [t, i18n])

  // 窗口 resize
  useEffect(() => {
    const handleResize = () => {
      setIsWidthOverflow(window.innerWidth < contentWidthRef.current)
    }
    window.addEventListener("resize", handleResize)
    return () => window.removeEventListener("resize", handleResize)
  }, [])

  return (
    <LayoutStateContext.Provider value={{ isWidthOverflow, registerWidthCheck }}>
      {children}
    </LayoutStateContext.Provider>
  )
}

export const useLayoutState = () => {
  const ctx = useContext(LayoutStateContext)
  if (!ctx) {
    throw new Error("useLayoutState must be used within a LayoutStateProvider")
  }
  return ctx
}
