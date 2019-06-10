
import { empty_array } from '@lib/utils'

class Step {
  constructor () {

  }
}

// run a sequence on a canvas to output a point
function run_sequence (canvas, seq, nd = 10) {
  var x = 0, y = 0
  var d = 0
  const { grid, width, height } = convas
  const pt = empty_array(nd)

  for (var i = 0; i < seq.length; i += 2) {
    var o = seq[i]
    var r = seq[i+1]
    switch (o) {
      // move input
      case 'r':
        if (++x > width) x = width
      break
      case 'l':
        if (--x < 0) x = 0
      break
      case 'u':
        if (--y > height) y = height
      break
      case 'd':
        if (--y < 0) y = 0
      break

      // move output point dimension pointer
      case '{':
        if (--d < 0) d = 0
      break
      case '}':
        if (++d > nd) d = nd
      break

      // loops (TODO)

      // unary operations
      case '++':
        pt[d]++
      break
      case '--':
        pt[d]--
      break

      //
      case '+=':
        pt[d] += input[x][y]
      break
      case '-=':
        pt[d] -= input[x][y]
      break
      case '*=':
        pt[d] *= input[x][y]
      break
      case '/=':
        pt[d] /= input[x][y]
      break
      case '%=':
        pt[d] %= input[x][y]
      break

      // conditionals
      case '{':
        if (--d < 0) d = 0
      break
    }
  }

  return pt
}
