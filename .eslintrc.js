const Path = require('path')

module.exports = {
  root: true,
  env: {
    browser: true,
  },
  extends: [
    'standard',
  ],
  plugins: [
    'snakecasejs',
    'import',
  ],
  globals: {
    __PRODUCTION__: false,
  },
  settings: {
    'import/resolver': {
      alias: {
        map: [
          ['@', Path.resolve(__dirname, 'src')],
        ],
      },
    },
  },
  rules: {
    'camelcase': 0,
    // 'camelcase': ['warn', {
    //   properties: 'never',
    //   ignoreDestructuring: true,
    // }],
    'comma-dangle': ['error', {
      arrays: 'always-multiline',
      objects: 'always-multiline',
      imports: 'always-multiline',
      exports: 'always-multiline',
      functions: 'never',
    }],
    'function-paren-newline': ['error', 'consistent'],
    'no-bitwise': 0,
    'no-console': 0,
    'no-param-reassign': 0,
    'no-shadow': 0,
    'no-underscore-dangle': 0,
    'no-constant-condition': 1,
    // 'snakecasejs/snakecasejs': 'warn',
    // 'snakecasejs/whitelist': [],
  },
}
