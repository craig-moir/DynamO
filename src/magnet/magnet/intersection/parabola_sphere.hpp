/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.dynamomd.org
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <magnet/intersection/polynomial.hpp>
#include <magnet/math/vector.hpp>

namespace magnet {
  namespace intersection {
    /*! \brief A parabolic(ray)-sphere intersection test with backface culling.
      
      \param T The origin of the ray relative to the sphere center.
      \param D The direction/velocity of the ray.
      \param A The acceleration of the ray.
      \param r The radius of the sphere.
      \return The time until the intersection, or HUGE_VAL if no intersection.
    */
    template<bool inverse = false>
    inline double parabola_sphere(const math::Vector& R, const math::Vector& V, const math::Vector& A, const double& r)
    {
      detail::PolynomialFunction<4> f;
      f[0] = R.nrm2() - r * r;
      f[1] = 2 * (V | R);
      f[2] = 2 * (V.nrm2() + (A | R));
      f[3] = 6 * (A | V);
      f[4] = 6 * A.nrm2();
      if (inverse)
	f.flipSign();
      return detail::nextEvent(f, r * r);
    }
  }
}
