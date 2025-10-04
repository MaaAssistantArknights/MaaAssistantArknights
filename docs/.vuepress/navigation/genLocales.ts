import { SiteLocaleConfig, LocaleConfig } from 'vuepress';
import { ThemeLocaleData } from 'vuepress-theme-plume';

import { locales } from './i18n';
import { genNavigationComponents } from './genNavigationComponents';

export function genSiteLocales(): SiteLocaleConfig {
  const siteLocales: SiteLocaleConfig = {};
  for (const locale of locales) {
    siteLocales[`/${locale.linkName}/`] = {
      lang: locale.htmlLang,
      title: locale.siteTitle,
      description: locale.siteDescription,
    };
  }
  return siteLocales;
}

export function genThemeLocales(): LocaleConfig<ThemeLocaleData> {
  const themeLocales: LocaleConfig<ThemeLocaleData> = {};
  for (const locale of locales) {
    const navigationComponents = genNavigationComponents(locale);
    themeLocales[`/${locale.linkName}/`] = {
      navbar: navigationComponents.navbar,
      notes: {
        dir: locale.dirName,
        link: `/${locale.linkName}/`,
        notes: navigationComponents.notes,
      },
    };
  }
  return themeLocales;
}
