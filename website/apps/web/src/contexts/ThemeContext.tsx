import React, { createContext, useContext, useState, useEffect } from 'react';

type ThemeType = 'dark' | 'light';

interface ThemeContextType {
  theme: ThemeType;
  toggleTheme: () => void;
  resetThemeToSystem: () => void;
}

const ThemeContext = createContext<ThemeContextType | undefined>(undefined);

export const ThemeProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [theme, setTheme] = useState<ThemeType>(() => {
  const savedTheme = localStorage.getItem('theme') as ThemeType;
  return savedTheme || (window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
});

  const toggleTheme = () => {
    setTheme((prevTheme) => {
      const newTheme = prevTheme === 'dark' ? 'light' : 'dark';
      localStorage.setItem('theme', newTheme);
      return newTheme;
    });
  };

  const resetThemeToSystem = () => {
    localStorage.removeItem('theme');
    const systemTheme = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    setTheme(systemTheme);
  };

  useEffect(() => {
  const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
  
  const handler = (e: MediaQueryListEvent) => {
    if (!localStorage.getItem('theme')) {
      setTheme(e.matches ? 'dark' : 'light');
    }
  };

  handler({ matches: mediaQuery.matches } as MediaQueryListEvent);
  
  mediaQuery.addEventListener('change', handler);
  return () => mediaQuery.removeEventListener('change', handler);
}, []);

useEffect(() => {
    if (theme === 'dark') {
      document.documentElement.classList.add('dark');
      document.documentElement.style.background = '#080808';
    } else {
      document.documentElement.classList.remove('dark');
      document.documentElement.style.background = '#f5f5f5';
    }
  }, [theme]);

  return (
    <ThemeContext.Provider value={{ theme, toggleTheme, resetThemeToSystem }}>
      {children}
    </ThemeContext.Provider>
  );
};

export const useTheme = (): ThemeContextType => {
  const context = useContext(ThemeContext);
  if (context === undefined) {
    throw new Error('useTheme must be used within a ThemeProvider');
  }
  return context;
};
