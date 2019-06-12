const Path = require('path')
const webpack = require('webpack')
const HtmlWebpackPlugin = require('html-webpack-plugin')

const mode = process.env.NODE_ENV === 'production' ? 'production' : 'development'
const src_path = Path.resolve(__dirname, 'src')
const lib_path = Path.join(require('os').homedir(), 'phoenix', 'affinaties-staging', 'src', 'lib') // YUCK!!

module.exports = {
  mode,
  target: 'electron-renderer',
  entry: [
    Path.join(src_path, 'meaningful-chaos.js'),
  ],
  resolve: {
    alias: {
      '@': src_path,
      '@src': src_path,
      '@lib': lib_path,
    },
  },
  devServer: {
    // contentBase: './dist',
    // hot: true,
    historyApiFallback: true,
    // It suppress error shown in console, so it has to be set to false.
    quiet: false,
    // It suppress everything except error, so it has to be set to false as well
    // to see success build.
    noInfo: false,
    stats: {
      // Config for minimal console.log mess.
      assets: false,
      colors: true,
      version: false,
      hash: false,
      timings: false,
      chunks: false,
      chunkModules: false,
    },
  },
  // externals: {
  //   electron: 'electron'
  // },
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
      },
    ],
  },
  plugins: [
    new webpack.DefinePlugin({
      'process.env': {
        NODE_ENV: JSON.stringify(mode),
      },
      __PRODUCTION__: JSON.stringify(mode === 'production'),
      DEBUG: JSON.stringify(mode !== 'production'),
      RUN_COMPACTOR_NULLS: '11', // after finding 11 nulls, compact the array
    }),
    new HtmlWebpackPlugin({
      template: Path.join(src_path, 'index.ejs'),
      title: 'meaningful chaos',
    }),
    // new webpack.HotModuleReplacementPlugin(),
  ],
}
