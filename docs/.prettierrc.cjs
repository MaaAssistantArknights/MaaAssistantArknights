module.exports = {
  printWidth: 120,
  tabWidth: 2,
  useTabs: false,
  bracketSpacing: true,
  bracketSameLine: false,
  endOfLine: 'auto',
  semi: false,
  singleQuote: true,
  trailingComma: 'all',
  arrowParens: 'always',

  overrides: [
    {
      files: ['**/*.*css'],
      options: {
        singleQuote: false,
      },
    },
    {
      files: ['**/*.yml', '**/*.yaml'],
      options: {
        parser: 'yaml',
        singleQuote: false,
      },
    },
    {
      files: ['**/*.json'],
      options: {
        tabWidth: 4,
      },
    },
    {
      // Keep Markdown options aligned with the root config and markdownlint.
      // Markdown remains ignored because Prettier cannot emit compact tables.
      files: ['**/*.{md,markdown,mdown,mkdn,mkd}'],
      options: {
        printWidth: 120,
        tabWidth: 2,
        useTabs: false,
        proseWrap: 'preserve',
        endOfLine: 'auto',
        embeddedLanguageFormatting: 'off',
      },
    },
  ],
}
