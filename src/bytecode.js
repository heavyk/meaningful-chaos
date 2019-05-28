
function run (px, out, seq) {
  var x = 0, y = 0
  var d = 0
  const height = px.length
  const width = px[0].length
  const nd = output.length

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

      // move output pointer
      case '{':
        if (--d < 0) d = 0
      break
      case '}':
        if (++d > nd) d = nd
      break

      // loops (TODO)

      // unary operations
      case '++':
        output[d]++
      break
      case '--':
        output[d]--
      break

      //
      case '+=':
        output[d] += input[x][y]
      break
      case '-=':
        output[d] -= input[x][y]
      break
      case '*=':
        output[d] *= input[x][y]
      break
      case '/=':
        output[d] /= input[x][y]
      break
      case '%=':
        output[d] %= input[x][y]
      break

      // conditionals
      case '{':
        if (--d < 0) d = 0
      break
    }
  }
}
