module.exports = {
  root: true,
  extends: ['prettier', 'react'],
  settings: {
    next: {
      rootDir: ['apps/*/'],
    },
  },
  rules: {
    'react/jsx-key': 'off',
  },
  ignorePatterns: ['*/node_modules/*', '*/dist/*', '*/build/*'],
};
