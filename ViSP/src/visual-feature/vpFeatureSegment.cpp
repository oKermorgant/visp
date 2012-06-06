/****************************************************************************
 *
 * $Id: vpFeatureThetaU.cpp 3530 2012-01-03 10:52:12Z fspindle $
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2012 by INRIA. All rights reserved.
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional 
 * Edition License.
 *
 * See http://www.irisa.fr/lagadic/visp/visp.html for more information.
 * 
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://www.irisa.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Description:
 * Segment visual feature.
 *
 * Authors:
 * Filip Novotny
 * Fabien Spindler
 *
 *****************************************************************************/


#include <visp/vpBasicFeature.h>
#include <visp/vpFeatureSegment.h>
#include <visp/vpImagePoint.h>
#include <visp/vpMeterPixelConversion.h>
#include <visp/vpMath.h>
#include <visp/vpDisplay.h>
#include <cmath>

// Exception
#include <visp/vpException.h>
#include <visp/vpMatrixException.h>

// Debug trace
#include <visp/vpDebug.h>


/*!
  \file vpFeatureSegment.cpp
  \brief class that defines the vpFeatureSegment visual feature
*/


/*! 

  Initialise the memory space requested for segment visual
  feature.

*/
void
vpFeatureSegment::init()
{
  //feature dimension
  dim_s = 4 ;
  nbParameters = 6;

  // memory allocation
  s.resize(dim_s) ;
  if (flags == NULL)
    flags = new bool[nbParameters];
  for (unsigned int i = 0; i < nbParameters; i++) flags[i] = false;
}

/*! 
  Default constructor that builds an empty segment visual feature.

  \param normalized : If true, use normalized features \f${\bf s} = (x_n, y_n, l_n, \alpha)\f$.
  If false, use non normalized features \f${\bf s} = (x_c, y_c, l_c, \alpha)\f$.
*/
vpFeatureSegment::vpFeatureSegment(bool normalized):
  vpBasicFeature()
{
  this->normalized_ = normalized;

  init();
}

/*!
  Compute and return the interaction matrix \f$ L \f$ associated to a
  subset of the possible features \f${\bf s} = (x_c, y_c, l, \alpha)\f$
  or \f${\bf s} = (x_n, y_n, l_n, \alpha)\f$.

  The interaction matrix of the non normalized feature set is of the following form:
  \f[
  {\bf L} = \left[
      \begin{array}{c}
        L_{x_c} \\
        L_{y_c} \\
        L_{l} \\
        L_{\alpha}
      \end{array}
    \right] =
    \left[
      \begin{array}{cccccc}
        -\lambda_2 & 0 & \lambda_2 x_c - \lambda_1 l \frac{\cos \alpha}{4} &
        x_c y_c + l^2 \frac{\cos \alpha \sin \alpha}{4} &
        -(1 + {x_{c}}^{2} + l^2 \frac{\cos^2\alpha}{4}) &
        y_c \\
        0 & -\lambda_2 & \lambda_2 y_c - \lambda_1 l \frac{\sin \alpha}{4} &
        1 + {y_{c}}^{2} + l^2 \frac{\sin^2 \alpha}{4} &
        -x_c y_c-l^2 \frac{\cos \alpha \sin \alpha}{4} &
        -x_c \\
        \lambda_1 \cos \alpha & \lambda_1 \sin \alpha &
        \lambda_2 l - \lambda_1 (x_c \cos \alpha + y_c \sin \alpha) &
        l (x_c \cos \alpha \sin \alpha + y_c (1 + \sin^2 \alpha)) &
        -l (x_c (1 + \cos^2 \alpha)+y_c \cos \alpha \sin \alpha) &
        0 \\
        -\lambda_1  \frac{\sin \alpha}{l} & \lambda_1 \frac{\cos \alpha}{l} &
        \lambda_1 \frac{x_c \sin \alpha - y_c \cos \alpha}{l} &
        -x_c \sin^2 \alpha + y_c \cos \alpha \sin \alpha &
        x_c \cos \alpha \sin \alpha - y_c \cos^2 \alpha &
        -1
      \end{array}
     \right]
  \f]

  with \f$ \lambda_1 = \frac{Z_1 - Z_2}{Z_1 Z_2}\f$ and \f$ \lambda_2 = \frac{Z_1 + Z_2}{2 Z_1 Z_2}\f$
  where \f$Z_i\f$ are the depths of the points.


  \param select : Selection of a subset of the possible segment features.
  - To compute the interaction matrix for all the four 
    subset features \f$(x_c \f$, \f$ y_c \f$, \f$ l \f$, \f$ \alpha)\f$ or
    \f$(x_n \f$, \f$ y_n \f$, \f$ l_n \f$, \f$ \alpha)\f$ use vpBasicFeature::FEATURE_ALL. In
    that case the dimension of the interaction matrix is \f$ [4 \times 6] \f$.
  - To compute the interaction matrix for only one of the subset
    use one of the following functions:
    selectXc(), selectYc(), selectL(), selectAlpha(). In that case, the returned
    interaction matrix is of dimension \f$ [1 \times 6] \f$ .

  \return The interaction matrix computed from the segment features.

  The code below shows how to compute the interaction matrix associated to 
  the visual feature \f${\bf s} = (x_c, y_c, l, \alpha)\f$.
  \code
#include <visp/vpFeatureSegment.h>
#include <visp/vpPoint.h>

int main()
{
  // Define two 3D points in the object frame
  vpPoint p1, p2;
  p1.setWorldCoordinates(.1, .1, 0.);
  p2.setWorldCoordinates(.3, .2, 0.);

  // Define the camera pose wrt the object
  vpHomogeneousMatrix cMo (0, 0, 1, 0, 0, 0); // Z=1 meter
  // Compute the coordinates of the points in the camera frame
  p1.changeFrame(cMo);
  p2.changeFrame(cMo);
  // Compute the coordinates of the points in the image plane by perspective projection
  p1.project();
  p2.project();

  // Build the segment visual feature
  vpFeatureSegment s;
  s.buildFrom(p1.get_x(), p1.get_y(), p1.get_Z(), p2.get_x(), p2.get_y(), p2.get_Z());

  // Compute the interaction matrix
  vpMatrix L = s.interaction( vpBasicFeature::FEATURE_ALL );
}
  \endcode

  In this case, L is a 4 by 6 matrix.

  It is also possible to build the interaction matrix associated to
  one of the possible features. The code below shows how to modify the previous code to consider as
  visual feature \f$s = (l, \alpha)\f$.

  \code
  vpMatrix L = s.interaction( vpFeatureSegment::selectL() | vpFeatureSegment::selectAlpha() );
  \endcode

  In that case, L is a 2 by 6 matrix.
*/
vpMatrix
vpFeatureSegment::interaction( const unsigned int select )
{

  vpMatrix L ;
  L.resize(0,6) ;

  if (deallocate == vpBasicFeature::user)  
  {
    for (unsigned int i = 0; i < nbParameters; i++)
    {
      if (flags[i] == false)
      {
        switch(i){
        case 0:
          vpTRACE("Warning !!!  The interaction matrix is computed but xc was not set yet");
        break;
        case 1:
          vpTRACE("Warning !!!  The interaction matrix is computed but Yc was not set yet");
        break;
        case 2:
          vpTRACE("Warning !!!  The interaction matrix is computed but l was not set yet");
        break;
        case 3:
          vpTRACE("Warning !!!  The interaction matrix is computed but alpha was not set yet");
        break;
        case 4:
          vpTRACE("Warning !!!  The interaction matrix is computed but Z1 was not set yet");
        break;
        case 5:
          vpTRACE("Warning !!!  The interaction matrix is computed but Z2 was not set yet");
        break;
        default:
          vpTRACE("Problem during the reading of the variable flags");
        }
      }
    }
  }

  //This version is a simplification
  double lambda1 = (Z1_-Z2_)/(Z1_*Z2_); // -l * lambda
  double lambda2 = (Z1_+Z2_)/(2*Z1_*Z2_); // 1/Zm
  
  if (normalized_)
  {
    // here var xc_ contains xc/l, yc contains yc/l and l contains 1/l
    double lambda = -lambda1 * l_;
    double Zn_inv = lambda2 * l_;
    double lc = cos_a_ / l_;
    double ls = sin_a_ / l_;
    double xnalpha = xc_*cos_a_+yc_*sin_a_;
    double lnc = cos_a_ *  l_;
    double lns = sin_a_ *  l_;

    if (vpFeatureSegment::selectXc() & select ){
      vpMatrix Lxn(1,6);
      Lxn[0][0] = -Zn_inv +  lambda * xc_ * cos_a_;
      Lxn[0][1] = lambda * xc_ * sin_a_ ;
      //Lxn[0][2] = l_ * lambda * (lc /4. - xc_*xnalpha); // orig FC
      Lxn[0][2] = lambda / l_ * (cos_a_ /4. - xc_*xnalpha);
      //Lxn[0][3] = l_*(lc*ls/4. - xc_*xnalpha*sin_a_); // orig FC
      Lxn[0][3] = l_* lc*ls/4. - xc_*xnalpha*sin_a_/l_;
      //Lxn[0][4] = -l_*(1.+lc*lc/4. - xc_*xnalpha*cos_a_) ; // orig FC
      Lxn[0][4] = -l_*(1.+lc*lc/4. - xc_*xnalpha*cos_a_/l_/l_) ;
      Lxn[0][5] = yc_ ;
      L = vpMatrix::stackMatrices(L, Lxn) ;
    }

    if (vpFeatureSegment::selectYc() & select ){
      vpMatrix Lyn(1,6);
      Lyn[0][0] = lambda*yc_*cos_a_ ;
      Lyn[0][1] = -Zn_inv + lambda*yc_*sin_a_ ;
      //Lyn[0][2] = lambda * l_*(ls/4.-yc_*xnalpha); // orig FC
      Lyn[0][2] = lambda / l_*(sin_a_/4.-yc_*xnalpha);
      //Lyn[0][3] = l_*(1+ls*ls/4.-yc_*xnalpha*sin_a_) ; // orig FC
      Lyn[0][3] = l_*(1+ls*ls/4.-yc_*xnalpha*sin_a_/l_/l_) ;
      //Lyn[0][4] = l_*(yc_*xnalpha*cos_a_-lc*ls/4.) ;  // orig FC
      Lyn[0][4] = yc_*xnalpha*cos_a_/l_ - lc*ls/4.*l_;
      Lyn[0][5] = -xc_ ;
      L = vpMatrix::stackMatrices(L, Lyn) ;
    }

    if (vpFeatureSegment::selectL() & select ){
      vpMatrix Lln(1,6);
      Lln[0][0] = lambda * lnc ;
      Lln[0][1] = lambda * lns ;
      Lln[0][2] = -(Zn_inv + lambda*xnalpha);
      Lln[0][3] = -yc_-xnalpha*sin_a_ ;
      Lln[0][4] = xc_ + xnalpha*cos_a_ ;
      Lln[0][5] = 0 ;
      L = vpMatrix::stackMatrices(L, Lln) ;
    }

  }
  else
  {
    if (vpFeatureSegment::selectXc() & select ){
      vpMatrix Lxc(1,6);
      Lxc[0][0] = -lambda2 ;
      Lxc[0][1] = 0. ;
      Lxc[0][2] = lambda2*xc_ - lambda1*l_*cos_a_/4.;
      //Lxc[0][3] = xc_*xc_ + l_*l_*cos_a_*sin_a_/4. ; // orig bug
      Lxc[0][3] = xc_*yc_ + l_*l_*cos_a_*sin_a_/4. ;
      Lxc[0][4] = -(1+xc_*xc_+l_*l_*cos_a_*cos_a_/4.) ;
      Lxc[0][5] = yc_ ;
      L = vpMatrix::stackMatrices(L,Lxc) ;
    }

    if (vpFeatureSegment::selectYc() & select ){
      vpMatrix Lyc(1,6);
      Lyc[0][0] = 0. ;
      Lyc[0][1] = -lambda2 ;
      Lyc[0][2] = lambda2*yc_ - lambda1*l_*sin_a_/4.;
      Lyc[0][3] = 1+yc_*yc_+l_*l_*sin_a_*sin_a_/4. ;
      Lyc[0][4] = -xc_*yc_-l_*l_*cos_a_*sin_a_/4. ;
      Lyc[0][5] = -xc_ ;
      L = vpMatrix::stackMatrices(L,Lyc) ;
    }

    if (vpFeatureSegment::selectL() & select ){
      vpMatrix Ll(1,6);
      Ll[0][0] = lambda1*cos_a_ ;
      Ll[0][1] = lambda1*sin_a_ ;
      Ll[0][2] = lambda2*l_-lambda1*(xc_*cos_a_+yc_*sin_a_);
      Ll[0][3] = l_*(xc_*cos_a_*sin_a_ + yc_*(1+sin_a_*sin_a_)) ;
      Ll[0][4] = -l_*(xc_*(1+cos_a_*cos_a_)+yc_*cos_a_*sin_a_) ;
      Ll[0][5] = 0 ;
      L = vpMatrix::stackMatrices(L,Ll) ;
    }
  }
  if (vpFeatureSegment::selectAlpha() & select ){
    vpMatrix Lalpha(1,6);
      Lalpha[0][0] = -lambda1*sin_a_/l_ ;
      Lalpha[0][1] = lambda1*cos_a_/l_ ;
      Lalpha[0][2] = lambda1*(xc_*sin_a_-yc_*cos_a_)/l_;
      Lalpha[0][3] = -xc_*sin_a_*sin_a_+yc_*cos_a_*sin_a_;
      Lalpha[0][4] = xc_*cos_a_*sin_a_ - yc_*cos_a_*cos_a_ ;
      Lalpha[0][5] = -1 ;
    L = vpMatrix::stackMatrices(L,Lalpha) ;
  }

  return L ;
}

/*!
  Computes the error between the current and the desired visual features from a subset
  of the possible features \f${\bf s} = (x_c, y_c, l, \alpha)\f$
  or \f${\bf s} = (x_n, y_n, l_n, \alpha)\f$.

  For the angular component \f$\alpha\f$, we define the error as
  \f$\alpha \ominus \alpha^*\f$, where \f$\ominus\f$ is modulo \f$2\pi\f$
  substraction.

  \param s_star : Desired 2D segment feature.

  \param select : The error can be computed for a selection of a
  subset of the possible segment features.
  - To compute the error for all the four parameters use
    vpBasicFeature::FEATURE_ALL. In that case the error vector is a 4
    dimension column vector.
  - To compute the error for only one subfeature of
    \f${\bf s} = (x_c, y_c, l, \alpha)\f$ or \f${\bf s} = (x_n, y_n, l_n, \alpha)\f$ feature set
    use one of the following functions: selectXc(), selectYc(), selectL(), selectAlpha().

  \return The error between the current and the desired
  visual feature.

*/
vpColVector
vpFeatureSegment::error( const vpBasicFeature &s_star, const unsigned int select )
{ 
  vpColVector e(0) ;

  if (vpFeatureSegment::selectXc() & select ){
    vpColVector exc(1) ;
    exc[0] = xc_-s_star[0];
    e = vpMatrix::stackMatrices(e,exc) ;
  }

  if (vpFeatureSegment::selectYc() & select ){
    vpColVector eyc(1) ;
    eyc[0] = yc_ - s_star[1];
    e = vpMatrix::stackMatrices(e,eyc) ;
  }

  if (vpFeatureSegment::selectL() & select ){
    vpColVector eL(1) ;
    eL[0] = l_ - s_star[2];
    e = vpMatrix::stackMatrices(e,eL) ;
  }

  if (vpFeatureSegment::selectAlpha() & select ){
    vpColVector eAlpha(1) ;
    eAlpha[0] = alpha_ - s_star[3];
    while (eAlpha[0] < -M_PI) eAlpha[0] += 2*M_PI ;
    while (eAlpha[0] > M_PI) eAlpha[0] -= 2*M_PI ;
    e = vpMatrix::stackMatrices(e,eAlpha) ;
  }
  return e ;
}

/*!
  Print to stdout the values of the current visual feature \f$ s \f$.

  \param select : Selection of a subset of the possible segement features (\f$ x_c \f$,\f$ y_c \f$,\f$ l \f$,\f$ \alpha \f$).

  \code  
  s.print();
  \endcode

  produces the following output:

  \code
  vpFeatureSegment: (xc = -0.255634; yc = -0.13311; l = 0.105005; alpha = 92.1305 deg)
  \endcode

  while
  \code
  s.print( vpFeatureSegment::selectL() | vpFeatureSegment::selectAlpha() );
  \endcode

  produces the following output:

  \code
  vpFeatureSegment: (l = 0.105005; alpha = 92.1305 deg)
  \endcode
*/
void
vpFeatureSegment::print( const unsigned int select ) const
{
  std::cout <<"vpFeatureSegment: (";
  if (vpFeatureSegment::selectXc() & select ) {
    if (normalized_)
      std::cout << "xn = ";
    else
      std::cout << "xc = ";
    std::cout << s[0] << "; ";
  }
  if (vpFeatureSegment::selectYc() & select ) {
    if (normalized_)
      std::cout << "yn = ";
    else
      std::cout << "yc = ";
    std::cout << s[1] << "; ";
  }
  if (vpFeatureSegment::selectL() & select ) {
    if (normalized_)
      std::cout << "ln = ";
    else
      std::cout << "l = ";
    std::cout << s[2] << "; ";
  }
  if (vpFeatureSegment::selectAlpha() & select ) {
    std::cout << "alpha = " << vpMath::deg(s[3]) << " deg";
  }
  std::cout << ")" << std::endl;
}

/*!  
  Create an object with the same type.

  \code
  vpBasicFeature *s_star;
  vpFeatureSegment s;
  s_star = s.duplicate(); // s_star is now a vpFeatureSegment
  \endcode

*/
vpFeatureSegment *vpFeatureSegment::duplicate() const
{
  vpFeatureSegment *feature;
  
  feature = new vpFeatureSegment(*this) ;    
  return feature ;
}

/*!

  Displays a segment representing the feature on a greyscale image.
  The two limiting points are displayed in cyan and yellow.

  \param cam : Camera parameters.
  \param I : Image.
  \param color : Color to use for the segment.
  \param thickness : Thickness of the feature representation.

*/
void
vpFeatureSegment::display(const vpCameraParameters &cam ,
                          const vpImage<unsigned char> & I,
                          const vpColor &color,
                          unsigned int  thickness ) const
{
  double l, x, y;
  if (normalized_) {
    l = 1./l_;
    x = xc_ * l;
    y = yc_ * l;
  }
  else {
    l = l_;
    x = xc_;
    y = yc_;
  }

  double x1 = x - (l/2.)*cos_a_;
  double x2 = x + (l/2.)*cos_a_;

  double y1 = y - (l/2.)*sin_a_;
  double y2 = y + (l/2.)*sin_a_;
  vpImagePoint ip1,ip2;

  vpMeterPixelConversion::convertPoint(cam, x1, y1, ip1);
  vpMeterPixelConversion::convertPoint(cam, x2, y2, ip2);
  vpDisplay::displayLine(I, ip1, ip2, color, thickness);
  vpDisplay::displayCircle(I, ip1, 5, color, true);
  vpDisplay::displayCircle(I, ip2, 5, vpColor::yellow, true);
}

/*!

  Displays a segment representing the feature on a RGBa image.
  The two limiting points are displayed in cyan and yellow.

  \param cam : Camera parameters.
  \param I : Image.
  \param color : Color to use for the segment.
  \param thickness : Thickness of the feature representation.
*/
void
vpFeatureSegment::display(const vpCameraParameters & cam ,
                          const vpImage<vpRGBa> & I,
                          const vpColor &color,
                          unsigned int thickness ) const
{
  double l, x, y;
  if (normalized_) {
    l = 1./l_;
    x = xc_ * l;
    y = yc_ * l;
  }
  else {
    l = l_;
    x = xc_;
    y = yc_;
  }

  double x1 = x - (l/2.)*cos_a_;
  double x2 = x + (l/2.)*cos_a_;

  double y1 = y - (l/2.)*sin_a_;
  double y2 = y + (l/2.)*sin_a_;
  vpImagePoint ip1,ip2;

  vpMeterPixelConversion::convertPoint(cam, x1, y1, ip1);
  vpMeterPixelConversion::convertPoint(cam, x2, y2, ip2);
  vpDisplay::displayLine(I, ip1, ip2, color, thickness);
  vpDisplay::displayCircle(I, ip1, 5,vpColor::cyan, true);
  vpDisplay::displayCircle(I, ip2, 5,vpColor::yellow, true);
} 

/*!

  Build a segment visual feature from two points and their Z coordinates.

  \param x1, y1 : coordinates of the first point in the image plane.
  \param Z1 : depth of the first point in the camera frame.

  \param x2, y2 : coordinates of the second point in the image plane.
  \param Z2 : depth of the second point in the camera frame.

  Depending on the feature set that is considered, the features \f${\bf s} = (x_c, y_c, l, \alpha)\f$
  or \f${\bf s} = (x_n, y_n, l_n, \alpha)\f$ are computed from the two points using the
  following formulae:
  \f[ x_c = \frac{x_1 + x_2}{2} \f]
  \f[ y_c = \frac{y_1 + y_2}{2} \f]
  \f[ l = \sqrt{{x_1 - x_2}^2 + {y_1 - y_2}^2} \f]
  \f[ \alpha = arctan(\frac{y_1 - y_2}{x_1 - x_2}) \f]
*/
void vpFeatureSegment::buildFrom(const double x1, const double y1, const double Z1,
                                 const double x2, const double y2, const double Z2)
{
  double l = sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
  double x_c = (x1+x2)/2.;
  double y_c = (y1+y2)/2.;
  double alpha = atan2(y1-y2,x1-x2);

  if (normalized_) {
    setXc( x_c / l );
    setYc( y_c / l );
    setL( 1/l );
    setAlpha( alpha );

    setZ1(Z1);
    setZ2(Z2);
  }
  else {
    setXc( x_c );
    setYc( y_c );
    setL( l );
    setAlpha( alpha );

    setZ1(Z1);
    setZ2(Z2);
  }
}

