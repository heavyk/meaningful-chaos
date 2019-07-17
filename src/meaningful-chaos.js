// import plugger from 'lib/plugins/plugger'
import plugger from '@lib/plugins/plugger'
import { empty_array } from '@lib/utils'

import { put_cam_in_video_element, capture_video } from './camera'
import { random_img_test, exaggerate_pixels, strongest_pixels } from './grid-stuff'

const raf = requestAnimationFrame

var db

electron.ipc.on('redis', (event, sock) => {
  // connect to redis
  db = window.db = new electron.Redis(sock)
})


plugger(function meaningful_chaos (hh) {
  // const { G, h, c, t, v } = hh
  const { h, v } = hh
  // const screen_size = G.resize()

  // main drawing canvas (the one the webcam is streamed into)
  var grid_canvas = h('canvas')
  // offscreen canvas to hold video pixels
  var video_canvas = h('canvas')
  // video element which the webcam streams into
  var video = h('video', { autoplay: true })

  var summary_canvas = h('canvas', { width: 256 })

  grid_canvas.resize = v()
  grid_canvas.resize(({ width, height }) => {
    video_canvas.width = grid_canvas.width = video.width = width
    grid_canvas.height = video.height = video_canvas.height = height
  })

  var frames = 0
  var start_time = 0
  var last_frame = 0
  var show_fps = false
  var fps = 2
  var max_diff = 1000 / fps

  function update (now) {
    var diff = now - last_frame
    if (diff >= max_diff) {
      frames++
      capture_video(video, video_canvas)
      // random_img_test(video_canvas, 0.1) // generate an image with Math.random()
      // exaggerate_pixels(video_canvas, grid_canvas, 50)
      // strongest_pixels(video_canvas, grid_canvas, 50)
      summarise_pixels(video_canvas, grid_canvas, 2)
      summarise_densities(grid_canvas, summary_canvas)

      // canvas.pt = run_sequence(grid_canvas, )

      if (show_fps) {
        var elapsed = (performance.now() - start_time) / 1000
        if (elapsed > 5) {
          // @Incomplete: when making an interface, put this there instead of the console
          console.log(frames + ' in ' + elapsed.toPrecision(2) + 's :: ' + (frames / elapsed).toPrecision(3) + ' fps / ' + fps)
          start_time = now
          frames = 0
        }
      }

      last_frame = now
    }

    raf(update)
  }

  put_cam_in_video_element(video, (stream) => {
    const settings = stream.getTracks()[0].getSettings()
    const { width, height, frame_rate } = settings
    grid_canvas.resize(settings)

    if (frame_rate && !fps) fps = frame_rate
    max_diff = 1000 / fps
    start_time = performance.now()
    raf(update)
  })

  return [
    // video,
    // grid_canvas,
    summary_canvas,
  ]
})

function summarise_pixels (src_canvas, dest_canvas, exaggeration = 4) {
  const width = dest_canvas.width
  const height = dest_canvas.height
  const dd = new Uint8ClampedArray(width * height * 4)

  var vd = src_canvas.getContext('2d').getImageData(0, 0, width, height).data
  // @Performance: convert this to a a Float32Array
  var densities = empty_array(256)
  var grid = empty_array(width).map(() => empty_array(height))
  var max_density = 0

  for (var val, density, x = 0, y = 0, i = 0; i < vd.length; i += 4) {
    // calculate the value of the pixel
    val = Math.min(
      Math.min(vd[i + 0] * exaggeration, 255) +
          Math.min(vd[i + 1] * exaggeration, 255) +
          Math.min(vd[i + 2] * exaggeration, 255)
      , 255
    )

    // visually paint all of them greyscale
    dd[i + 0] = dd[i + 1] = dd[i + 2] = grid[x][y] = val
    dd[i + 3] = 255

    density = ++densities[val]
    if (val > 0 && density > max_density) max_density = density

    // calculate the *next* x/y coord
    if (++x >= width) {
      x = 0
      y++
    }
  }

  dest_canvas.grid = grid
  dest_canvas.densities = densities
  dest_canvas.max_density = max_density
  dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}

async function store_grid (ts, event, canvas) {
  // - grid format: `struct { u16 width; u16 height; u32 reserved; u8[width * height] data }`
	//   - key event: `redis.hset('keyup-k', 'grid-xxxxxxxx', Buffer.from(grid))`
  const { width, height } = canvas
  const _dd = new Uint8ClampedArray(4 + (width * height))
  const wh = new Uint16ClampedArray(_dd.buffer, 0, 2)
  const dd = new Uint8ClampedArray(_dd.buffer, 4, width * height)
  wh[0] = width, wh[1] = height
  // redis.hset(event, ts, Buffer.from(dd))
  await redis.hset(event, 'grid-' + ts, dd.buffer)
}

function summarise_densities (src_canvas, dest_canvas) {
  const margin = 30
  const width = 256
  const height = src_canvas.max_density
  if (height > 0) {
    if (height > 1024) {
      // @Incomplete: account for cases where the height is really big. add a divider
    }

    const densities = src_canvas.densities
    const dd = new Uint8ClampedArray(width * height * 4)

    // @Incomplete: don't resize every frame.
    //              only resize if it's actually bigger
    dest_canvas.height = height + margin + margin
    dest_canvas.width = width + margin + margin

    var x, y, d, i
    for (x = 1; x <= width; x++) {
      d = densities[x]
      for (y = 1; y <= d; y++) {
        i = (x * 4) + (y * width * 4)
        dd[i + 0] = 255
        dd[i + 1] = 0
        dd[i + 2] = 0
        dd[i + 3] = 255
      }
    }

    dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), margin, margin)
  }
}
