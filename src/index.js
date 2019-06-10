// import plugger from 'lib/plugins/plugger'
import plugger from '@lib/plugins/plugger'
import { empty_array } from '@lib/utils'

import { put_cam_in_video_element, capture_video } from './camera'

const raf = requestAnimationFrame

plugger(function meaningful_chaos (hh) {
  const {G, h, c, t, v} = hh
  // const screen_size = G.resize()

  // main drawing canvas (the one the webcam is streamed into)
  var canvas = h('canvas')
  // offscreen canvas to hold video pixels
  var video_canvas = h('canvas')
  // video element which the webcam streams into
  var video = h('video', { autoplay: true })

  var summary_canvas = h('canvas', { width: 256 })

  canvas.resize = v()
  canvas.resize(({width, height}) => {
    video_canvas.width = canvas.width = video.width = width
    canvas.height = video.height = video_canvas.height = height
  })

  var frames = 0
  var start_time = 0
  var last_frame = 0
  var show_fps = false
  var fps = 2
  var max_diff = 1000 / fps

  put_cam_in_video_element(video, (stream) => {
    const settings = stream.getTracks()[0].getSettings()
    const {width, height, frameRate} = settings
    canvas.resize(settings)

    if (frameRate && !fps) fps = frameRate
    max_diff = 1000 / fps
    start_time = performance.now()
    raf(update)
  })

  function update (now) {
    var diff = now - last_frame
    if (diff >= max_diff) {
      frames++
      capture_video(video, video_canvas)
      // random_img_test(video_canvas, 0.1) // generate an image with Math.random()
      // exaggerate_pixels(video_canvas, canvas, 50)
      // strongest_pixels(video_canvas, canvas, 50)
      summarise_pixels(video_canvas, canvas, 2)
      summarise_densities(canvas, summary_canvas)

      // canvas.pt = run_sequence(canvas, )

      if (show_fps) {
        var elapsed = (performance.now() - start_time) / 1000
        if (elapsed > 5) {
          // @Incomplete: when making an interface, put this there instead of the console
          console.log(frames+' in '+ elapsed.toPrecision(2) + 's :: '+(frames / elapsed).toPrecision(3) + ' fps / '+fps)
          start_time = now
          frames = 0
        }
      }

      last_frame = now
    }

    raf(update)
  }

  return [
    // video,
    canvas,
    summary_canvas,
  ]
})


function random_img_test (dest_canvas, magnitude = 0.1) {
	const width = dest_canvas.width
	const height = dest_canvas.height
	const dd = new Uint8ClampedArray(width * height * 4)

  for (var y = 1; y <= height; y++) {
		for (var x = 1; x <= width; x++) {
			var i = (x * 4) + (y * width * 4)
			dd[i+0] = Math.floor(Math.random() * magnitude * 255)
			dd[i+1] = Math.floor(Math.random() * magnitude * 255)
			dd[i+2] = Math.floor(Math.random() * magnitude * 255)
			dd[i+3] = 255
		}
	}

	dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}


function exaggerate_pixels (src_canvas, dest_canvas, magnitude = 4) {
	const width = dest_canvas.width
	const height = dest_canvas.height
	const dd = new Uint8ClampedArray(width * height * 4)
  var vd = src_canvas.getContext('2d').getImageData(0, 0, width, height).data

  for (var i = 0; i < vd.length; i += 4) {
    dd[i+0] = Math.min(vd[i+0] * magnitude, 255)
    dd[i+1] = Math.min(vd[i+1] * magnitude, 255)
    dd[i+2] = Math.min(vd[i+2] * magnitude, 255)
    dd[i+3] = 255
  }

	dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}


function strongest_pixels (src_canvas, dest_canvas, magnitude = 4) {
	const width = dest_canvas.width
	const height = dest_canvas.height
	const dd = new Uint8ClampedArray(width * height * 4)
  var vd = src_canvas.getContext('2d').getImageData(0, 0, width, height).data

  for (var i = 0; i < vd.length; i += 4) {
    var v = Math.min(vd[i+0] * magnitude, vd[i+1] * magnitude, vd[i+2] * magnitude, 255)
    dd[i+0] = v
    dd[i+1] = v
    dd[i+2] = v
    dd[i+3] = 255
  }

	dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}


function summarise_pixels (src_canvas, dest_canvas, exaggeration = 4) {
	const width = dest_canvas.width
	const height = dest_canvas.height
	const dd = new Uint8ClampedArray(width * height * 4)
  var vd = src_canvas.getContext('2d').getImageData(0, 0, width, height).data
  var densities = empty_array(256)
  var grid = empty_array(width).map(() => empty_array(height))
  var max_density = 0

  for (var val, density, x = 0, y = 0, i = 0; i < vd.length; i += 4) {
    // calculate the value of the pixel
    val = Math.min(
          Math.min(vd[i+0] * exaggeration, 255) +
          Math.min(vd[i+1] * exaggeration, 255) +
          Math.min(vd[i+2] * exaggeration, 255)
        , 255)

    // visually paint all of them greyscale
    dd[i+0] = dd[i+1] = dd[i+2] = grid[x][y] = val
    dd[i+3] = 255

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
  			dd[i+0] = 255
  			dd[i+1] = 0
  			dd[i+2] = 0
  			dd[i+3] = 255
  		}
  	}

  	dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), margin, margin)
  }
}
