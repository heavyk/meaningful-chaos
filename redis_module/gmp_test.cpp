
#include <gmp.h>
#include <gmpxx.h>
#include <real.hpp>
#include <iostream>

using namespace std;
using namespace mpfr;

typedef real<4096, MPFR_RNDN> Real;

typedef mpz_class Integer;
typedef mpf_class Float;
typedef mpq_class Ratio;

// TODO: implement my own version of tan, sin, cos using degrees (instead of radians)
// https://gforge.inria.fr/scm/viewvc.php/mpfr/trunk/src/cos.c?view=markup
// https://gforge.inria.fr/scm/viewvc.php/mpfr/trunk/src/sin.c?view=markup
/*
Real real_sine (Real deg) {
  int quad = 0;
  Real n = deg;

  // first, reduce n to a value 0-360
  // n = n % 360;
  // doesn't work, so we're gonna do it the iterative way:
  while (n > 360) n -= 360;
  while (n < 0) n += 360;

  // cout << "n: " << n << '-' << angle << endl;

  // second, reduce n to a value between 0-90, storing its quadrant
  if (n >= 270) {
    n -= 270;
    quad = 4;
  } else if (n >= 180) {
    n -= 180;
    quad = 3;
  } else if (n >= 90) {
    n -= 90;
    quad = 2;
  } else {
    quad = 1;
  }

  assert(n >= 0 && n <= 90);
  assert(quad >= 1 && quad <= 4); // silly, I know :)



  return n;
}
*/


Float sine (Float angle) {
  int quad = 0;
  Float opp(0, 4096);
  Float adj(0, 4096);

  // first, reduce angle to a value 0-360
  // angle = angle % 360;
  // doesn't work, so we're gonna do it the iterative way:
  while (angle > 360) angle -= 360;
  while (angle < 0) angle += 360;

  // cout << "n: " << n << '-' << angle << endl;

  // second, reduce angle to a value between 0-90, storing its quadrant
  if (angle >= 270) {
    angle -= 270;
    quad = 4;
  } else if (angle >= 180) {
    angle -= 180;
    quad = 3;
  } else if (angle >= 90) {
    angle -= 90;
    quad = 2;
  } else {
    quad = 1;
  }

  assert(angle >= 0 && angle <= 90);
  assert(quad >= 1 && quad <= 4); // silly, I know :)

  if (angle == 0) {
    return angle;
  // } else if (angle == 90) {
  //   return Float(1);
  // } else if (angle == 30) {
  //   return Float(0.5);
  // } else if (angle == 60) {
  //   opp = 0.75;
  // } else if (angle == 45) {
  //   opp = 0.5;
  } else {
    Float ratio(angle / 90, 4096);
    Float hyp(0, 4096);

    opp = ratio * ratio;
    adj = opp * 1 / ratio;

    // iteratate over the opp and adj values until their hyp = 1.
    for (int i = 0; (hyp > 1 || hyp < 1) && i < 10; i++) {
      hyp = sqrt((opp*opp)+(adj*adj));
      cout << (opp*opp)+(adj*adj) << "==\n" << hyp*hyp << "\n---\n";
      cout << (opp*opp) / (adj*adj) << "==\n" << ratio << "\n---\n";
      // cout << "hyp: " << hyp << endl;
    }

    // for (int i = 0; (hyp > 1 || hyp < 1) && i < 10; i++) {
    //   hyp = sqrt((opp*opp)+(adj*adj));
    // }

    // adjust for the quadrant
    // ...

    cout << "ratio: " << ratio << endl;
    cout << "opp: " << opp << endl;
    cout << "adj: " << adj << endl;
  }

  return sqrt(opp);
}

// Real inscribed (Real n) {
//   // return n * sin(360 / n);
//   return n * sin(const_pi() / n);
// }
//
// Real circumscribed (Real n) {
//   // return n * tan(180 / n);
//   return n * tan(const_pi() / n);
// }

#define PRECISION 4096

void calc_triangle (Float opp, Float adj) {
  // first find the hypotenuse
  Float hyp(sqrt((opp*opp)+(adj*adj)));
  // rescale it so that hyp = 1
  opp = opp / hyp;
  adj = adj / hyp;
  hyp = 1;

  // cout << opp << " + " << adj << " = " << hyp << endl;

  Float opp2(opp*opp);
  Float adj2(adj*adj);
  Float hyp2(hyp*hyp);

  // assert(opp2+adj2 == hyp2);
  cout << opp2 << " + " << adj2 << " = " << hyp2 << endl;

  cout << "sin: " << sqrt(opp / hyp) << endl;
  cout << "cos: " << sqrt(adj / hyp) << endl;
  cout << "tan: " << sqrt(adj / opp) << endl;
  cout << "sin-1: " << (opp2 / adj2 * 90) << endl;
}

int main (void) {
  mpf_set_default_prec(PRECISION);
  cout.precision(20);
  // cout.precision(80);
  // cout.setf(ios_base::fixed);
  // cout.setf(ios_base::showpos);

  // Ratio ratio(22, 7);
  // Float pi_ratio(ratio, 2000);
  //
  // cout << pi_ratio << endl;
  //
  // Real a = "1.23456789";
  // Real b = 5.0;
  //
  // cout << sin(2 * pow(a, b)) << endl;

  // Real n = 4;
  //
  // cout << "inscribed: " << inscribed(n) << endl;
  // cout << "circumscribed: " << circumscribed(n) << endl;

  // Float out(sine(360), 4096);
  // Float out = ;

  cout << "3,4,5: " << endl;
  calc_triangle(3,4);
  cout << "45-45-90: " << endl;
  calc_triangle(2,2);
  cout << "30-60-90: " << endl;
  calc_triangle(1,3);

  // cout << "15° " << sine(15) << endl;
  // cout << "30° " << sine(30) << endl;
  // cout << "45° " << sine(45) << endl;
  // cout << "60° " << sine(60) << endl;

  return 0;
}

// first, construct a right-triange such that since sin = opp / hyp,

// notes:
// sin, cos, tan:
// https://www.mathsisfun.com/sine-cosine-tangent.html

// regular polygons:
// https://brilliant.org/wiki/regular-polygons/

/*

inscribed:
n * sin(360/n)

circumscribed:
n * tan(180/n)

sin(T) =
construct a right-angle triangle such that the T is a calculatable angle.. eg. 30°

start w/ square of unit length 3, then, when T = 30° it's 1/3 of the diagonal (hypotenuse) of of the square.

triangle is 3 (bottom), 1 (1/3 of hyp), x
*/
