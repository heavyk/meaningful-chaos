const Path = require('path')
const Fs = require('fs')
const write_file = require('util').promisify(Fs.writeFile)
const read_file = require('util').promisify(Fs.readFile)
const exec = require('execa')
const mkdir = require('make-dir')
const tmp_dir = require('temp-dir')
const exists = require('path-exists')

async function ensure_installation () {
  async function extract_tar (filename) {
    console.log('extract tar')
    return exec('tar', ['xzpf', filename, '--strip-components=1', '-C', '.'], { cwd: redis_dir })
    .then(({ stdout }) => {
      console.log('tar finished!', stdout)
      return exec('make', ['distclean'], { cwd: redis_dir })
    }).then(({ stdout }) => {
      console.log('make distclean finished!', stdout)
      return exec('make',  ['-j'+ require('os').cpus().length], { cwd: redis_dir })
    }).then(({ stdout }) => {
      console.log('make completed')
    })
  }

  function download_tar (url, tar_path) {
    return new Promise((resolve, reject) => {
      const dest = Fs.createWriteStream(tar_path)
      got.stream(latest[4]).pipe(dest)
      dest.on('end', () => {
        // extract_tar()
        resolve()
      }).on('error', reject)
    })
  }

  try {
    if (!(await exists(redis.server_bin))) {
      // for now, redis will not upgrade itself. just download the latest
      await mkdir(redis_dir)
      const got = require('got')
      var res = await got('https://github.com/antirez/redis-hashes/raw/master/README')
      // 'hash' | filename | digest-name | digest-value | url
      var latest = res.body.trim().split('\n').pop().split(' ')
      console.log(latest)
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

const redis = {
  start,
  stop,
  dir: Path.join(__dirname, 'redis'),
  server_bin: Path.join(__dirname, 'redis', 'src', 'redis-server'),
  sock: tmp_dir + '/meaningful_chaos_redis.sock',
  pidfile: tmp_dir + '/meaningful_chaos_redis.pid',
  logfile: tmp_dir + '/meaningful_chaos_redis.log',
  conffile: Path.join(__dirname, 'redis', 'meaningful_chaos.conf'),
}

redis.conf = [
  'bind 127.0.0.1',
  // 'port 1167', // 'port 6379',
  // 'bind no',
  'port no',
  'unixsocket ' + redis.sock,
  'protected-mode yes',
  'timeout 0',
  'daemonize yes',
  'supervised no',
  'pidfile ' + redis.pidfile,
  'loglevel notice',
  'logfile ' + redis.logfile,
  'stop-writes-on-bgsave-error yes',
  'save 900 1', // after 900 sec (15 min) if at least 1 key changed
  'save 300 10', // after 300 sec (5 min) if at least 10 keys changed
  'save 60 10000', // after 60 sec if at least 10000 keys changed
  'rdbcompression yes',
  'rdbchecksum yes',
  'dbfilename db.rdb',
  'dir ./',
  'rdb-save-incremental-fsync yes',
]


async function start () {
  await ensure_installation()
  if (!(await exists(redis.pidfile))) {
    console.log('starting...')
    await write_file(redis.conf.path, redis.conf.join('\n'))
    await exec(redis.server_bin, [redis.conf.path])
    console.log('started on:', redis.sock)
  }
  console.log('redis-cli -s '+redis.sock)
}

async function stop () {
  if (await exists(redis.pidfile)) {
    const pid = await read_file(redis.pidfile) * 1
    await exec('kill', [pid])
    console.log('killed:', pid)
  }
}

if (!module.parent) {
  ensure_installation()
    .then(() => start())
    .then(() => stop())
    .catch(err => {
      console.error('error:', err)
    })
}

// const Redis = require('ioredis')
// var db = new Redis(redis.sock)

module.exports = redis
