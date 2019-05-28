const Path = require('path')
const webpack = require('webpack')
const HtmlWebpackPlugin = require('html-webpack-plugin')

const mode = process.env.NODE_ENV === 'production' ? 'production' : 'development'
const src_path = Path.resolve(__dirname, 'src')
const lib_path = Path.join(require('os').homedir(), 'phoenix', 'affinaties-staging', 'src', 'lib') // YUCK!!

module.exports = {
  mode,
  entry: [
    Path.join(src_path, 'index.js')
  ],
  resolve: {
    alias: {
      '@': src_path,
      '@src': src_path,
      '@lib': lib_path,
    }
  },
  module: {
    rules: [
      // rollup slows things down a little bit.
      // also, the webpack es6 code loader makes really big bundles
      // I don't really want to think about it right now, so it's commented out
      // {
      //   test: new RegExp(Path.relative(__dirname, src_path) + '/index\.js$'),
      //   use: [{
      //     loader: 'webpack-rollup-loader',
      //     options: {
      //       // OPTIONAL: any rollup options (except `entry`)
      //       // e.g.
      //       external: [/* modules that shouldn't be rollup'd */]
      //     },
      //   }]
      // },
      {
        test: /\.(vert|frag)$/,
        loader: 'raw-loader',
        include: src_path,
      }
    ]
  },
  plugins: [
    new webpack.DefinePlugin({
      'process.env': {
        NODE_ENV: JSON.stringify(mode)
      },
      __PRODUCTION__: JSON.stringify(mode === 'production'),
      DEBUG: JSON.stringify(mode !== 'production'),
      RUN_COMPACTOR_NULLS: '11', // after finding 11 nulls, compact the array
    }),
    new HtmlWebpackPlugin({
      template: Path.join(src_path, 'index.ejs'),
      title: 'meaningful chaos',
    })
  ]
}
