import React, { useState, useRef, useEffect } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { useTranslation } from "react-i18next";

const languages = [
    { code: "zh-CN", label: "简体中文" },
    { code: "zh-TW", label: "繁體中文" },
    { code: "en-US", label: "English" },
    { code: "ja-JP", label: "日本語" },
    { code: "ko-KR", label: "한국어" },
];

export const LanguageToggle: React.FC = () => {
    const { i18n } = useTranslation();
    const [open, setOpen] = useState(false);
    const dropdownRef = useRef<HTMLDivElement>(null);

    const toggleDropdown = () => setOpen((prev) => !prev);
    const changeLanguage = (lng: string) => {
        i18n.changeLanguage(lng);
        setOpen(false);
    };

    // 点击外部关闭下拉
    useEffect(() => {
        const handleClickOutside = (event: MouseEvent) => {
            if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
                setOpen(false);
            }
        };
        document.addEventListener("mousedown", handleClickOutside);
        return () => document.removeEventListener("mousedown", handleClickOutside);
    }, []);

    const currentLabel = languages.find((l) => l.code === i18n.language)?.label || "Language";

    return (
        <motion.div ref={dropdownRef} className="relative">
            {/* 按钮 */}
            <motion.button
                onClick={toggleDropdown}
                className="px-3 py-1 rounded-lg border shadow-md
                           bg-white dark:bg-gray-800
                           text-gray-800 dark:text-white
                           border-gray-200 dark:border-gray-700
                           hover:bg-gray-100 dark:hover:bg-gray-700
                           transition-all"
                whileTap={{ scale: 0.95 }}
                whileHover={{ scale: 1.05 }}
            >
                {currentLabel}
            </motion.button>

            {/* 下拉菜单 */}
            <AnimatePresence>
                {open && (
                    <motion.div
                        key="dropdown"
                        initial={{ opacity: 0, y: -10 }}
                        animate={{ opacity: 1, y: 0 }}
                        exit={{ opacity: 0, y: -10 }}
                        transition={{ duration: 0.2 }}
                        className="absolute right-0 mt-1 min-w-[140px] border rounded shadow-lg
              bg-white dark:bg-gray-800 text-gray-800 dark:text-white z-50"
                    >
                        {languages.map((lang) => (
                            <motion.button
                                key={lang.code}
                                onClick={() => changeLanguage(lang.code)}
                                disabled={i18n.language === lang.code}
                                className="block w-full px-4 py-2 text-left hover:bg-gray-200 dark:hover:bg-gray-700 disabled:opacity-50"
                                whileTap={{ scale: 0.95 }}
                            >
                                {lang.label}
                            </motion.button>
                        ))}
                    </motion.div>
                )}
            </AnimatePresence>
        </motion.div>
    );
};
