// Modules to control application life and create native browser window
const {app, BrowserWindow} = require('electron')
const path = require('path')

const DEFAULT_PORT = 1166
const config = {
  outputFilename: '/bundle.js',
}

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let mainWindow

// function start_dev_server () {
//   const createConfig = require('webpack-dev-server/lib/utils/createConfig')
//   const createLogger = require('webpack-dev-server/lib/utils/createLogger')
//   const options = createConfig(config, process.argv, { port: DEFAULT_PORT })
//   startDevServer(config, options)
// }

function makeServer(_config) {
  const config = require('./webpack.config')
  const webpack = require('webpack')
  const WebpackDevServer = require('webpack-dev-server')

  return new Promise((resolve, reject) => {
    var i = 0
    const compiler = webpack(config)
    const server = new WebpackDevServer(compiler, config.devServer)

    compiler.hooks.done.tap('IDoNotUnderstandWhatThisStringIsForButItCannotBeEmpty', () => {
      // console.log('Done compiling')
      if (++i === 2) resolve(server)
    })

    server.listen(DEFAULT_PORT, '0.0.0.0', err => {
      if (err) reject(err)

      // console.log('listening')
      if (++i === 2) resolve(server)
    })
  })
}

function createWindow () {
  // Create the browser window.
  mainWindow = new BrowserWindow({
    width: 1920,
    height: 1100,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  mainWindow.loadFile('splash.html')

  // iniitialise webkit-dev-server
  makeServer().then(() => {
    // and load the index.html of the app.
    mainWindow.loadURL('http://localhost:'+DEFAULT_PORT+'/')
    // Open the DevTools.
    mainWindow.webContents.openDevTools()
  })

  // Emitted when the window is closed.
  mainWindow.on('closed', function () {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null
  })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', function () {
  // On macOS it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', function () {
  // On macOS it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (mainWindow === null) createWindow()
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
