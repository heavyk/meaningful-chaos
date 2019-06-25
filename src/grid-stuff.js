
export function random_img_test (dest_canvas, magnitude = 0.1) {
  const width = dest_canvas.width
  const height = dest_canvas.height
  const dd = new Uint8ClampedArray(width * height * 4)

  for (var y = 1; y <= height; y++) {
    for (var x = 1; x <= width; x++) {
      var i = (x * 4) + (y * width * 4)
      dd[i + 0] = Math.floor(Math.random() * magnitude * 255)
      dd[i + 1] = Math.floor(Math.random() * magnitude * 255)
      dd[i + 2] = Math.floor(Math.random() * magnitude * 255)
      dd[i + 3] = 255
    }
  }

  dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}

export function exaggerate_pixels (src_canvas, dest_canvas, magnitude = 4) {
  const width = dest_canvas.width
  const height = dest_canvas.height
  const dd = new Uint8ClampedArray(width * height * 4)
  var vd = src_canvas.getContext('2d').getImageData(0, 0, width, height).data

  for (var i = 0; i < vd.length; i += 4) {
    dd[i + 0] = Math.min(vd[i + 0] * magnitude, 255)
    dd[i + 1] = Math.min(vd[i + 1] * magnitude, 255)
    dd[i + 2] = Math.min(vd[i + 2] * magnitude, 255)
    dd[i + 3] = 255
  }

  dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}

export function strongest_pixels (src_canvas, dest_canvas, magnitude = 4) {
  const width = dest_canvas.width
  const height = dest_canvas.height
  const dd = new Uint8ClampedArray(width * height * 4)
  var vd = src_canvas.getContext('2d').getImageData(0, 0, width, height).data

  for (var i = 0; i < vd.length; i += 4) {
    var v = Math.min(vd[i + 0] * magnitude, vd[i + 1] * magnitude, vd[i + 2] * magnitude, 255)
    dd[i + 0] = v
    dd[i + 1] = v
    dd[i + 2] = v
    dd[i + 3] = 255
  }

  dest_canvas.getContext('2d').putImageData(new ImageData(dd, width), 0, 0)
}
