
export function put_cam_in_video_element (video, cb) {
  navigator.mediaDevices.getUserMedia({
    audio: false,
    video: {
      facingMode: 'user',
      width: { min: 128, ideal: 512 * 3 },
      height: { min: 128, ideal: 512 * 3 },
    },
  }).then((stream) => {
    video.srcObject = stream
    cb && cb(stream)
  })
}

export function capture_video (video, canvas) {
  var width = video.width
  var height = video.height
  canvas.getContext('2d').drawImage(video, 0, 0, width, height)
  return canvas
}
