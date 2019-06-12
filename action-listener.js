const net = require('net')
const { ipcMain: ipc } = require('electron')

ipc.on('grid', (event) => {
  // new grid stored in the database.
  // full event info and grid id for the event

  // ...probably want to make a queue which tries
  // to find the best bytecode to resolve that event to the
  // action's position in space

})

var server = net.createServer(client => {
  console.log('someone connected!')
  client.on('data', data => {
    console.log('data: ' + data)
    var event = JSON.parse(data)
    ipc.send('action', event)
  }).on('end', () => {
    console.log('client disconnected')
  }).on('error', err => {
    console.log('client error:', err)
  })

  client.write('action_listener v0.1.0\n')
  client.pipe(client)
}).on('error', err => {
  console.log('server error:', err)
})


server.listen(1166, '127.0.0.1')
