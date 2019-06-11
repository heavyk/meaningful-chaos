// Modules to control application life and create native browser window
const { app, BrowserWindow } = require('electron')
const Path = require('path')

var listen_port = 1166
var listen_ip = '127.0.0.1' // '0.0.0.0'
const config = {
  outputFilename: '/bundle.js',
}

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
var mainWindow
var contents

// function start_dev_server () {
//   const createConfig = require('webpack-dev-server/lib/utils/createConfig')
//   const createLogger = require('webpack-dev-server/lib/utils/createLogger')
//   const options = createConfig(config, process.argv, { port: listen_port })
//   startDevServer(config, options)
// }

function start_server () {
  const config = require('./webpack.config')
  const webpack = require('webpack')
  const WebpackDevServer = require('webpack-dev-server')

  return new Promise((resolve, reject) => {
    var i = 0
    const compiler = webpack(config)
    const server = new WebpackDevServer(compiler, config.devServer)

    compiler.hooks.done.tap('AfterCompileAndListen', () => {
      if (++i === 2) resolve(server)
    })

    server.listen(listen_port, listen_ip, err => {
      if (err) reject(err)
      if (++i === 2) resolve(server)
    })
  })
}

function createWindow () {
  mainWindow = new BrowserWindow({
    width: 1920,
    height: 1100,
    webPreferences: {
      preload: Path.join(__dirname, 'preload.js'),
    },
  })

  contents = mainWindow.webContents
  mainWindow.loadFile('splash.html')
  contents.openDevTools()

  start_server().then(() => {
    mainWindow.loadURL('http://localhost:' + listen_port + '/')
    // contents.openDevTools()
  })

  mainWindow.on('closed', () => {
    mainWindow = null
    contents = null
  })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On macOS it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q

  // for now, we're just going to quit, however, later I will want
  // the action-logger to continue to run in the background
  if (process.platform !== 'darwin' || true) app.quit()
})

app.on('activate', () => {
  // for macOS re-create the window when the dock icon is clicked
  if (mainWindow === null) createWindow()
})

// function key_code (key) {
//   // TODO
//   return {
//     enter, backspace, delete, tab, escape, control, alt, shift, end, home, insert, left, up, right, down, pageUp, pageDown, printScreen
//   }
// }

var kbd = {
  shift: false,
  control: false,
  alt: false,
  meta: false,
  rightButtonDown: false,
  leftButtonDown: false,
  middleButtonDown: false,
}
var mouse = {
  x: 0,
  y: 0,
  modifiers: [],
}
function mouse_event ({ x, y }) {
  var event = {
    x: (mouse.x += x),
    y: (mouse.y += y),
    modifers: mouse.modifers,

  }
  contents.sendInputEvent('mousemove', { x: mouse.x + x, y })
}

// here, we send key events to the page
// contents.sendInputEvent('keyup', {keyCode: 'k'})
