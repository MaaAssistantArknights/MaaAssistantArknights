import React, { useState, useRef, useEffect } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { useTranslation } from "react-i18next";

const languages = [
    { code: "zh_cn", label: "中文" },
    { code: "en_us", label: "English" },
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

    // 点击页面其他地方关闭下拉
    useEffect(() => {
        const handleClickOutside = (event: MouseEvent) => {
            if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
                setOpen(false);
            }
        };
        document.addEventListener("mousedown", handleClickOutside);
        return () => document.removeEventListener("mousedown", handleClickOutside);
    }, []);

    return (
        <motion.div
            className="relative inline-block"
            ref={dropdownRef}
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            style={{ zIndex: 9999 }}
        >
            {/* 主按钮 */}
            <motion.button
                onClick={toggleDropdown}
                className="px-3 py-1 border rounded bg-white dark:bg-gray-800 shadow-md"
                whileTap={{ scale: 0.95 }}
            >
                {languages.find((l) => l.code === i18n.language)?.label || "Language"}
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
                        className="absolute mt-1 right-0 border rounded shadow-lg bg-white dark:bg-gray-800 z-50 min-w-[120px]"
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
