const Path = require('path')
const Fs = require('fs')
const write_file = require('util').promisify(Fs.writeFile)
const read_file = require('util').promisify(Fs.readFile)
const exec = require('execa')
const mkdir = require('make-dir')
// const tmp_dir = require('temp-dir')
const exists = require('path-exists')
const log = require('pino')()

async function ensure_installation () {
  // @Incomplete: windows installation from this repo:
  //   https://github.com/tporadowski/redis
  async function extract_tar (filename) {
    log.info('extract tar')
    // TODO: use fs-extra (for windows support?)
    return exec('tar', ['xzpf', filename, '--strip-components=1', '-C', '.'], { cwd: redis_dir })
    .then(({ stdout }) => {
      log.info('tar finished!', stdout)
      return exec('make', ['distclean'], { cwd: redis_dir })
    }).then(({ stdout }) => {
      log.info('make distclean finished!', stdout)
      return exec('make',  ['-j'+ require('os').cpus().length], { cwd: redis_dir })
    }).then(() => {
      log.info('make completed')
    })
  }

  function download_tar (url, tar_path) {
    return new Promise((resolve, reject) => {
      const dest = Fs.createWriteStream(tar_path)
      got.stream(latest[4]).pipe(dest)
      dest.on('end', () => { resolve() }).on('error', reject)
    })
  }

  try {
    await mkdir(data_dir)
    if (!(await exists(redis.server_bin))) {
      // for now, redis will not upgrade itself. just download the latest
      // so, if you want to upgrade, just delete the redis directory
      // actually, that's gonna cause problems because the db is stored there...
      await mkdir(redis_dir)
      const got = require('got')
      var res = await got('https://github.com/antirez/redis-hashes/raw/master/README')
      // 'hash' | filename | digest-name | digest-value | url
      var latest = res.body.trim().split('\n').pop().split(' ')
      const tar_path = Path.join(redis_dir, latest[1])
      if (!(await exists(tar_path))) {
        await download_tar()
        // not already downloaded
      }

      await extract_tar(latest[1])
    }
  } catch (e) {
    throw e
  }
}

const data_dir = Path.join(__dirname, '.data')
var _db
const redis = {
  start, stop, kill, pid,
  restarter,
  dir: Path.join(__dirname, 'redis'),
  data: data_dir,
  server_bin: Path.join(__dirname, 'redis', 'src', 'redis-server'),
  sock: data_dir + '/meaningful_chaos_redis.sock',
  pidfile: data_dir + '/meaningful_chaos_redis.pid',
  logfile: data_dir + '/meaningful_chaos_redis.log',
  stdout: data_dir + '/meaningful_chaos_stdout.log',
  stderr: data_dir + '/meaningful_chaos_stderr.log',
  conffile: Path.join(__dirname, 'redis', 'meaningful-chaos.conf'),
  get db () {
    if (!_db) {
      const Redis = require('ioredis')
      _db = new Redis(redis.sock)
    }

    return _db
  }
}

redis.conf = [
  'bind 127.0.0.1',
  // 'port 1167',
  'port 6379',
  // 'port no',
  'unixsocket ' + redis.sock,
  'protected-mode yes',
  'timeout 0',
  // 'daemonize yes', // disabled to allow easier debugging
  'supervised no',
  'pidfile ' + redis.pidfile,
  // 'loglevel notice',
  'loglevel debug',
  'logfile ' + redis.logfile,
  'stop-writes-on-bgsave-error yes',
  'save 900 1', // after 900 sec (15 min) if at least 1 key changed
  'save 300 10', // after 300 sec (5 min) if at least 10 keys changed
  'save 60 10000', // after 60 sec if at least 10000 keys changed
  'rdbcompression yes',
  'rdbchecksum yes',
  'dbfilename db.rdb',
  'dir ' + data_dir,
  // 'loadmodule ' + Path.join(__dirname, 'redis_module', 'redis_date_module.so'),
  'loadmodule ' + Path.join(__dirname, 'redis_module', 'meaningful-chaos.so'),
  // 'loadmodule ' + Path.join(__dirname, 'neural-redis', 'neuralredis.so'),
  'rdb-save-incremental-fsync yes',
]

// TODO: generalise these. so that, given a conf similar to `redis` the start/stop commands generalise on the pidfile, conffile, etc.
async function start () {
  await ensure_installation()
  await stop()

  // Fs.unlink(redis.logfile, ()=>{}) // truncate log?

  await write_file(redis.conffile, redis.conf.join('\n'))
  // await exec(redis.server_bin, [redis.conffile], { detached: true })
  const { spawn } = require('child_process')
  const out = Fs.openSync(redis.stdout, 'a')
  const err = Fs.openSync(redis.stderr, 'a')
  const sp = spawn(redis.server_bin, [redis.conffile], {
    detached: true,
    stdio: ['ignore', out, err]
  })

  // to fully let it be independent of node's process:
  // sp.unref()

  // wait up to ~2 seconds for it to start
  let tries = 20, _pid = 0
  do {
    await sleep(100) // give it a little time to start
    if (--tries == 0) {
      log.info('aborting attempt to find the pid after 20 tries')
      break
    }
  } while (!(_pid = await pid()))

  // log.info('redis-cli -s '+redis.sock)
  return _pid
}

async function pid () {
  var p
  try {
    p = await read_file(redis.pidfile)
    log.info('pid: '+p)
  } catch (e) {
    // log.warn('pidfile read error: ' + e)
    p = 0
  }

  return p * 1
}

async function restarter (path) {
  return require('chokidar')
  .watch(path, { ignoreInitial: true })
  .on('all', async (event, path) => {
    if (event == 'unlink') return
    await sleep(100) // sometimes gives image not found.
    await start()
  })
}

async function kill (pid_ = -1) {
  if (!~pid_) pid_ = await pid()
  log.info('killing:', pid_)
  return await exec('kill', [pid_]).then(() => Fs.unlink(redis.pidfile, ()=>{})).catch(()=>{})
}

async function stop () {
  if (await exists(redis.pidfile)) {
    const pid_ = await pid()
    let running
    while (running = await is_running(pid_)) {
      log.info('killing: '+pid_ + ' running:'+running)
      await kill(pid_)
      await sleep(500)
    }

    Fs.unlink(redis.pidfile, ()=>{})
  }
}

// TODO: move out to a lib
async function is_running (query) {
  query = (query + '').toLowerCase().trim()
  let cmd = ''
  switch (process.platform) {
    case 'win32' : cmd = `tasklist`; break
    case 'darwin' : cmd = `ps -ax`; break
    case 'linux' : cmd = `ps -A`; break
  }

  const { stdout } = await exec.shell(cmd)
  // searching for spaces around it. works for darwin. not sure for others. %windows% %linux%
  return !!~stdout.toLowerCase().indexOf(' '+query+' ')
}

// TODO: move out to a lib
function sleep (ms) {
  return new Promise((resolve) => setTimeout(resolve, ms) )
}

if (!module.parent) {
  start().then(async () => {
    await sleep(1000) // give it 1s to fail startup
    if (!(await is_running(await pid()))) {
      log.error('started process not running. something happened.')
      log.info(await read_file(redis.logfile, 'utf8'))
    }
    // await stop()
    const Tail = require('tail-forever')
    let tail = new Tail(redis.logfile)
    tail.on('line', (line) => {
      console.log(line)
    })
    tail.on('error', (err) => {
      console.log('ERROR:', err)
    })
    redis.tail = tail
  }).catch(err => {
    log.error(err.stack)
    log.info(Fs.readFileSync(redis.logfile, 'utf8'))
  })
}

// this should go in electron!!
process.on('exit', async () => {
  await stop()
  if (redis.tail) redis.tail.unwatch()
})

module.exports = redis
