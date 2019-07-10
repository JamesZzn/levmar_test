#include <vector>
#include <array>
#include <cmath>
#include "levmar/levmar.h"
#include "mio/altro/rand.h"
#include "mio/altro/error.h"

#ifndef LM_DBL_PREC
#error Demo program assumes that levmar has been compiled with double precision, see LM_DBL_PREC!
#endif

#define NUM_EXP_PARAM 1

/*
y = f(x) = a*exp(-bx) + c = p[0]*exp(-p[1]*x) + p[2]
traditionally:
  m is the number of training examples
  n is the number of features (x0 x1 x2... xn)
  y is the column vector of all training labels with dimensionality m (ie. f(x))
  x is the column vector of all the feature inputs with dimensionality n+1 (ie. the known variables of f(x))
    so x and y are vectors known as the training data
  θ is the parameter column vector with dimensionality n + 1 (ie. THE THINGS WE ARE TRYING TO ESTIMATE)

levmar uses slightly different notation:
  m is the parameter vector dimension. For the problem here, m = 3.
  n is the measurement vector dimension. (english translation: the number of training examples)
  x is the "measurement vector". (english translation: this is y, aka f(x))
  p is the parameter vector with dimension m.
*/

// Definite integral of sin^2(x)
double DefIntSineSquared(const double a, const double b) {
  return 0.5 * (b - a - 0.5 * (std::sin(2.0*b) - std::sin(2.0*a)));
}

// Partial derivative with respect to 'b' of the definite integral of sin^2(x)
double DefIntSineSquared_d_db(const double b) {
  return std::sin(b) * std::sin(b);
}


void Func(double *p, double *x, int m, int n, void *user_data) {
  std::vector<double> &x_data_vec = *static_cast< std::vector<double>* >(user_data);
  STD_INVALID_ARG_E(x_data_vec.size() == n)
  STD_INVALID_ARG_E(m == NUM_EXP_PARAM)
  x[0] = DefIntSineSquared(x_data_vec[0], p[0]);
}


void JacFunc(double *p, double *jac, int m, int n, void *user_data) {
  std::vector<double> &x_data_vec = *static_cast< std::vector<double>* >(user_data);
  STD_INVALID_ARG_E(x_data_vec.size() == n)
  STD_INVALID_ARG_E(m == NUM_EXP_PARAM)
  jac[0] = DefIntSineSquared_d_db(p[0]);
}


void HalfSineTest() {
  const size_t n = 1, // number of training samples
               m = NUM_EXP_PARAM; // number of parameters to estimate
  double opts[LM_OPTS_SZ], // opts[4] = minim. options [\mu, \epsilon1, \epsilon2, \epsilon3]
         info[LM_INFO_SZ];
  std::array<double, NUM_EXP_PARAM> p = {1.0}; // "random" initial value, zero does not work
  std::vector<double> y_data_vec = {M_PI/2.0}, // the definite integral from 0 to PI of sin^2(x) is PI/2
                      x_data_vec = {0}; // starting boundary (this is 'a' in the definite integral)
  const bool default_opt = false;
  opts[0] = LM_INIT_MU; // scale factor for initial \mu
  opts[1] = 1E-40; // stopping threshold for ||J^T e||_inf
  opts[2] = 1E-40; // stopping threshold for ||Dp||_2
  opts[3] = 1E-40; // stopping threshold for ||e||_2
  void *adata = static_cast<void*>(&x_data_vec);
  int ret = dlevmar_der(Func, JacFunc, p.data(), y_data_vec.data(), m, n, 10000,
                        default_opt ? NULL : opts, info, NULL, NULL, adata);
  printf("Levenberg-Marquardt returned in %g iter, reason %g, sumsq %g [%g]\n",
    info[5], info[6], info[1], info[0]);
  printf("Best fit parameters (should equal PI): %.7g\n", p[0]);
}


// Divide a half sin^2(x) wave into N equal area parts
void EqualAreaSineSquared() {
  const size_t N = 100;
  double opts[LM_OPTS_SZ], // opts[4] = minim. options [\mu, \epsilon1, \epsilon2, \epsilon3]
         info[LM_INFO_SZ];
  const double step = M_PI/static_cast<double>(N);
  // Approximate the starting position
  std::array<double, NUM_EXP_PARAM> p = {step};
  std::vector<double> y_data_vec = {(M_PI/2.0)/static_cast<double>(N)}, // the definite integral from 0 to PI of sin^2(x) is PI/2
                      x_data_vec = {0}; // starting boundary (this is 'a' in the definite integral)
  std::vector<double> bounds = {0.0};
  opts[0] = LM_INIT_MU; // scale factor for initial \mu
  opts[1] = 1E-40; // stopping threshold for ||J^T e||_inf
  opts[2] = 1E-40; // stopping threshold for ||Dp||_2
  opts[3] = 1E-40; // stopping threshold for ||e||_2
  for (size_t i = 0; i < N; ++i) {
    void *adata = static_cast<void*>(&x_data_vec);
    int ret = dlevmar_der(Func, JacFunc, p.data(), y_data_vec.data(), 1, 1, 10000,
                          opts, info, NULL, NULL, adata);
    bounds.push_back(p[0]);
    x_data_vec[0] = p[0]; // Update starting boundary 'a'
    p[0] += step;
  }
  for (size_t i = 0; i < N; ++i) {
    printf("%.8f, %.8f, %.8f\n", bounds[i], bounds[i+1], DefIntSineSquared(bounds[i], bounds[i+1]));
  }
}


int main(int argc, char *argv[]) {
  HalfSineTest();
  EqualAreaSineSquared();
  return 0;
}
