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
      files: ['**/*.ts'],
      options: {
        semi: true,
      },
    },
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
  ],
}
