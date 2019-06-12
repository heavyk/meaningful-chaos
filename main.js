// disable warnings
process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = true

// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain: ipc } = require('electron')
const Path = require('path')

var listen_port = 1166
var listen_ip = '127.0.0.1' // '0.0.0.0' // gives a firewall warning on OSX, so use localhost

const webpack = require('webpack')
const WebpackDevServer = require('webpack-dev-server')
const redis = require('./redis-db')

var main_window
var contents

ipc.on('key', event => {
  console.log('key', event)
})

function start_server () {
  const config = require('./webpack.config')
  return new Promise((resolve, reject) => {
    var i = 3 // after two callbacks
    const compiler = webpack(config)
    const server = new WebpackDevServer(compiler, config.devServer)

    redis.start().then(() => {
      if (!--i) resolve(server)
    })

    compiler.hooks.done.tap('AfterCompileAndListen', () => {
      if (!--i) resolve(server)
    })

    server.listen(listen_port, listen_ip, err => {
      if (err) reject(err)
      if (!--i) resolve(server)
    })
  })
}

function createWindow () {
  main_window = new BrowserWindow({
    width: 1920,
    height: 1100,
    webPreferences: {
      preload: __dirname + '/preload.js',
      // allowRunningInsecureContent: true,
      // contextIsolation: true,
    },
  })

  contents = main_window.webContents
  main_window.loadFile('splash.html')
  // contents.session.clearCache(() => { console.log('cleared')})
  contents.openDevTools()

  start_server().then(() => {
    main_window.loadURL('http://localhost:' + listen_port + '/')
    // contents.openDevTools()
  })

  main_window.on('closed', () => {
    main_window = null
    contents = null
  })

  // setTimeout(() => {
  //   contents.send('hello', {lala: 1234})
  // }, 2000)
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
  if (main_window === null) createWindow()
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
