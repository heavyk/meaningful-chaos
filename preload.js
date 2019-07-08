// All of the Node.js APIs are available in the preload process.
// It has the same sandbox as a Chrome extension.

window.electron = {
  ipc: require('electron').ipcRenderer,
  versions: process.versions,
  Redis: require('ioredis'),
}
