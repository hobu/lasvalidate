/*
===============================================================================

  FILE:  crscheck.cpp
  
  CONTENTS:
  
    see corresponding header file

  PROGRAMMERS:
  
    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2013, martin isenburg, rapidlasso - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/

#include <time.h>
#include <string.h>
#include <math.h>

#include "crscheck.hpp"

static const F64 PI = 3.141592653589793238462643383279502884197169;
static const F64 TWO_PI = PI * 2;
static const F64 PI_OVER_2 = PI / 2;
static const F64 PI_OVER_4 = PI / 4;
static const F64 deg2rad = PI / 180.0;
static const F64 rad2deg = 180.0 / PI;

static const F64 feet2meter = 0.3048;
static const F64 surveyfeet2meter = 0.3048006096012;

static const int CRS_PROJECTION_UTM      = 0;
static const int CRS_PROJECTION_LCC      = 1;
static const int CRS_PROJECTION_TM       = 2;
static const int CRS_PROJECTION_LONG_LAT = 3;
static const int CRS_PROJECTION_LAT_LONG = 4;

#define CRS_ELLIPSOID_NAD27 5
#define CRS_ELLIPSOID_NAD83 11
#define CRS_ELLIPSOID_Inter 14
#define CRS_ELLIPSOID_SAD69 19
#define CRS_ELLIPSOID_WGS72 22
#define CRS_ELLIPSOID_WGS84 23
#define CRS_ELLIPSOID_ID74  24
#define CRS_ELLIPSOID_GDA94 CRS_ELLIPSOID_WGS84

#define CRS_VERTICAL_WGS84   5030
#define CRS_VERTICAL_NAVD29  5102
#define CRS_VERTICAL_NAVD88  5103

class ReferenceEllipsoid
{
public:
  ReferenceEllipsoid(const I32 id, const CHAR* name, const F64 equatorialRadius, const F64 eccentricitySquared, const F64 inverseFlattening)
  {
    this->id = id;
    this->name = name; 
    this->equatorialRadius = equatorialRadius;
    this->eccentricitySquared = eccentricitySquared;
    this->inverseFlattening = inverseFlattening;
  }
  I32 id;
  const CHAR* name;
  F64 equatorialRadius; 
  F64 eccentricitySquared;  
  F64 inverseFlattening;  
};

static const ReferenceEllipsoid ellipsoid_list[] = 
{
  //  d, Ellipsoid name, Equatorial Radius, square of eccentricity, inverse flattening  
  ReferenceEllipsoid( -1, "Placeholder", 0, 0, 0),  //placeholder to allow array indices to match id numbers
  ReferenceEllipsoid( 1, "Airy", 6377563.396, 0.00667054, 299.3249646),
  ReferenceEllipsoid( 2, "Australian National", 6378160.0, 0.006694542, 298.25),
  ReferenceEllipsoid( 3, "Bessel 1841", 6377397.155, 0.006674372, 299.1528128),
  ReferenceEllipsoid( 4, "Bessel 1841 (Nambia) ", 6377483.865, 0.006674372, 299.1528128),
  ReferenceEllipsoid( 5, "Clarke 1866 (NAD-27)", 6378206.4, 0.006768658, 294.9786982),
  ReferenceEllipsoid( 6, "Clarke 1880", 6378249.145, 0.006803511, 293.465),
  ReferenceEllipsoid( 7, "Everest 1830", 6377276.345, 0.006637847, 300.8017),
  ReferenceEllipsoid( 8, "Fischer 1960 (Mercury) ", 6378166, 0.006693422, 298.3),
  ReferenceEllipsoid( 9, "Fischer 1968", 6378150, 0.006693422, 298.3),
  ReferenceEllipsoid( 10, "GRS 1967", 6378160, 0.006694605, 298.247167427),
  ReferenceEllipsoid( 11, "GRS 1980 (NAD-83)", 6378137, 0.00669438002290, 298.257222101),
  ReferenceEllipsoid( 12, "Helmert 1906", 6378200, 0.006693422, 298.3),
  ReferenceEllipsoid( 13, "Hough", 6378270, 0.00672267, 297.0),
  ReferenceEllipsoid( 14, "International", 6378388, 0.00672267, 297.0),
  ReferenceEllipsoid( 15, "Krassovsky", 6378245, 0.006693422, 298.3),
  ReferenceEllipsoid( 16, "Modified Airy", 6377340.189, 0.00667054, 299.3249646),
  ReferenceEllipsoid( 17, "Modified Everest", 6377304.063, 0.006637847, 300.8017),
  ReferenceEllipsoid( 18, "Modified Fischer 1960", 6378155, 0.006693422, 298.3),
  ReferenceEllipsoid( 19, "South American 1969", 6378160, 0.006694542, 298.25),
  ReferenceEllipsoid( 20, "WGS 60", 6378165, 0.006693422, 298.3),
  ReferenceEllipsoid( 21, "WGS 66", 6378145, 0.006694542, 298.25),
  ReferenceEllipsoid( 22, "WGS-72", 6378135, 0.006694318, 298.26),
  ReferenceEllipsoid( 23, "WGS-84", 6378137, 0.00669437999013, 298.257223563),
  ReferenceEllipsoid( 24, "Indonesian National 1974", 6378160, 0.0066946091071419115, 298.2469988070381)
};

static const int PCS_NAD27_Alabama_East = 26729;
static const int PCS_NAD27_Alabama_West = 26730;
static const int PCS_NAD27_Alaska_zone_1 = 26731; /* Hotine Oblique Mercator Projection not supported*/
static const int PCS_NAD27_Alaska_zone_2 = 26732;
static const int PCS_NAD27_Alaska_zone_3 = 26733;
static const int PCS_NAD27_Alaska_zone_4 = 26734;
static const int PCS_NAD27_Alaska_zone_5 = 26735;
static const int PCS_NAD27_Alaska_zone_6 = 26736;
static const int PCS_NAD27_Alaska_zone_7 = 26737;
static const int PCS_NAD27_Alaska_zone_8 = 26738;
static const int PCS_NAD27_Alaska_zone_9 = 26739;
static const int PCS_NAD27_Alaska_zone_10 = 26740;
static const int PCS_NAD27_California_I = 26741;
static const int PCS_NAD27_California_II = 26742;
static const int PCS_NAD27_California_III = 26743;
static const int PCS_NAD27_California_IV = 26744;
static const int PCS_NAD27_California_V = 26745;
static const int PCS_NAD27_California_VI = 26746;
static const int PCS_NAD27_California_VII = 26747;
static const int PCS_NAD27_Arizona_East = 26748;
static const int PCS_NAD27_Arizona_Central = 26749;
static const int PCS_NAD27_Arizona_West = 26750;
static const int PCS_NAD27_Arkansas_North = 26751;
static const int PCS_NAD27_Arkansas_South = 26752;
static const int PCS_NAD27_Colorado_North = 26753;
static const int PCS_NAD27_Colorado_Central = 26754;
static const int PCS_NAD27_Colorado_South = 26755;
static const int PCS_NAD27_Connecticut = 26756;
static const int PCS_NAD27_Delaware = 26757;
static const int PCS_NAD27_Florida_East = 26758;
static const int PCS_NAD27_Florida_West = 26759;
static const int PCS_NAD27_Florida_North = 26760;
static const int PCS_NAD27_Hawaii_zone_1 = 26761;
static const int PCS_NAD27_Hawaii_zone_2 = 26762;
static const int PCS_NAD27_Hawaii_zone_3 = 26763;
static const int PCS_NAD27_Hawaii_zone_4 = 26764;
static const int PCS_NAD27_Hawaii_zone_5 = 26765;
static const int PCS_NAD27_Georgia_East = 26766;
static const int PCS_NAD27_Georgia_West = 26767;
static const int PCS_NAD27_Idaho_East = 26768;
static const int PCS_NAD27_Idaho_Central = 26769;
static const int PCS_NAD27_Idaho_West = 26770;
static const int PCS_NAD27_Illinois_East = 26771;
static const int PCS_NAD27_Illinois_West = 26772;
static const int PCS_NAD27_Indiana_East = 26773;
static const int PCS_NAD27_Indiana_West = 26774;
static const int PCS_NAD27_Iowa_North = 26775;
static const int PCS_NAD27_Iowa_South = 26776;
static const int PCS_NAD27_Kansas_North = 26777;
static const int PCS_NAD27_Kansas_South = 26778;
static const int PCS_NAD27_Kentucky_North = 26779;
static const int PCS_NAD27_Kentucky_South = 26780;
static const int PCS_NAD27_Louisiana_North = 26781;
static const int PCS_NAD27_Louisiana_South = 26782;
static const int PCS_NAD27_Maine_East = 26783;
static const int PCS_NAD27_Maine_West = 26784;
static const int PCS_NAD27_Maryland = 26785;
static const int PCS_NAD27_Massachusetts = 26786;
static const int PCS_NAD27_Massachusetts_Is = 26787;
static const int PCS_NAD27_Michigan_North = 26788;
static const int PCS_NAD27_Michigan_Central = 26789;
static const int PCS_NAD27_Michigan_South = 26790;
static const int PCS_NAD27_Minnesota_North = 26791;
static const int PCS_NAD27_Minnesota_Central = 26792;
static const int PCS_NAD27_Minnesota_South = 26793;
static const int PCS_NAD27_Mississippi_East = 26794;
static const int PCS_NAD27_Mississippi_West = 26795;
static const int PCS_NAD27_Missouri_East = 26796;
static const int PCS_NAD27_Missouri_Central = 26797;
static const int PCS_NAD27_Missouri_West = 26798;
static const int PCS_NAD27_Montana_North = 32001;
static const int PCS_NAD27_Montana_Central = 32002;
static const int PCS_NAD27_Montana_South = 32003;
static const int PCS_NAD27_Nebraska_North = 32005;
static const int PCS_NAD27_Nebraska_South = 32006;
static const int PCS_NAD27_Nevada_East = 32007;
static const int PCS_NAD27_Nevada_Central = 32008;
static const int PCS_NAD27_Nevada_West = 32009;
static const int PCS_NAD27_New_Hampshire = 32010;
static const int PCS_NAD27_New_Jersey = 32011;
static const int PCS_NAD27_New_Mexico_East = 32012;
static const int PCS_NAD27_New_Mexico_Central = 32013;
static const int PCS_NAD27_New_Mexico_West = 32014;
static const int PCS_NAD27_New_York_East = 32015;
static const int PCS_NAD27_New_York_Central = 32016;
static const int PCS_NAD27_New_York_West = 32017;
static const int PCS_NAD27_New_York_Long_Is = 32018;
static const int PCS_NAD27_North_Carolina = 32019;
static const int PCS_NAD27_North_Dakota_N = 32020;
static const int PCS_NAD27_North_Dakota_S = 32021;
static const int PCS_NAD27_Ohio_North = 32022;
static const int PCS_NAD27_Ohio_South = 32023;
static const int PCS_NAD27_Oklahoma_North = 32024;
static const int PCS_NAD27_Oklahoma_South = 32025;
static const int PCS_NAD27_Oregon_North = 32026;
static const int PCS_NAD27_Oregon_South = 32027;
static const int PCS_NAD27_Pennsylvania_N = 32028;
static const int PCS_NAD27_Pennsylvania_S = 32029;
static const int PCS_NAD27_Rhode_Island = 32030;
static const int PCS_NAD27_South_Carolina_N = 32031;
static const int PCS_NAD27_South_Carolina_S = 32033;
static const int PCS_NAD27_South_Dakota_N = 32034;
static const int PCS_NAD27_South_Dakota_S = 32035;
static const int PCS_NAD27_Tennessee = 2204;
static const int PCS_NAD27_Texas_North = 32037;
static const int PCS_NAD27_Texas_North_Central = 32038;
static const int PCS_NAD27_Texas_Central = 32039;
static const int PCS_NAD27_Texas_South_Central = 32040;
static const int PCS_NAD27_Texas_South = 32041;
static const int PCS_NAD27_Utah_North = 32042;
static const int PCS_NAD27_Utah_Central = 32043;
static const int PCS_NAD27_Utah_South = 32044;
static const int PCS_NAD27_Vermont = 32045;
static const int PCS_NAD27_Virginia_North = 32046;
static const int PCS_NAD27_Virginia_South = 32047;
static const int PCS_NAD27_Washington_North = 32048;
static const int PCS_NAD27_Washington_South = 32049;
static const int PCS_NAD27_West_Virginia_N = 32050;
static const int PCS_NAD27_West_Virginia_S = 32051;
static const int PCS_NAD27_Wisconsin_North = 32052;
static const int PCS_NAD27_Wisconsin_Central = 32053;
static const int PCS_NAD27_Wisconsin_South = 32054;
static const int PCS_NAD27_Wyoming_East = 32055;
static const int PCS_NAD27_Wyoming_East_Central = 32056;
static const int PCS_NAD27_Wyoming_West_Central = 32057;
static const int PCS_NAD27_Wyoming_West = 32058;
static const int PCS_NAD27_Puerto_Rico = 32059;
static const int PCS_NAD27_St_Croix = 32060;

static const int PCS_NAD83_Alabama_East = 26929;
static const int PCS_NAD83_Alabama_West = 26930;
static const int PCS_NAD83_Alaska_zone_1 = 26931; /* Hotine Oblique Mercator Projection not supported*/
static const int PCS_NAD83_Alaska_zone_2 = 26932;
static const int PCS_NAD83_Alaska_zone_3 = 26933;
static const int PCS_NAD83_Alaska_zone_4 = 26934;
static const int PCS_NAD83_Alaska_zone_5 = 26935;
static const int PCS_NAD83_Alaska_zone_6 = 26936;
static const int PCS_NAD83_Alaska_zone_7 = 26937;
static const int PCS_NAD83_Alaska_zone_8 = 26938;
static const int PCS_NAD83_Alaska_zone_9 = 26939;
static const int PCS_NAD83_Alaska_zone_10 = 26940;
static const int PCS_NAD83_California_1 = 26941;
static const int PCS_NAD83_California_2 = 26942;
static const int PCS_NAD83_California_3 = 26943;
static const int PCS_NAD83_California_4 = 26944;
static const int PCS_NAD83_California_5 = 26945;
static const int PCS_NAD83_California_6 = 26946;
static const int PCS_NAD83_Arizona_East = 26948;
static const int PCS_NAD83_Arizona_Central = 26949;
static const int PCS_NAD83_Arizona_West = 26950;
static const int PCS_NAD83_Arkansas_North = 26951;
static const int PCS_NAD83_Arkansas_South = 26952;
static const int PCS_NAD83_Colorado_North = 26953;
static const int PCS_NAD83_Colorado_Central = 26954;
static const int PCS_NAD83_Colorado_South = 26955;
static const int PCS_NAD83_Connecticut = 26956;
static const int PCS_NAD83_Delaware = 26957;
static const int PCS_NAD83_Florida_East = 26958;
static const int PCS_NAD83_Florida_West = 26959;
static const int PCS_NAD83_Florida_North = 26960;
static const int PCS_NAD83_Hawaii_zone_1 = 26961;
static const int PCS_NAD83_Hawaii_zone_2 = 26962;
static const int PCS_NAD83_Hawaii_zone_3 = 26963;
static const int PCS_NAD83_Hawaii_zone_4 = 26964;
static const int PCS_NAD83_Hawaii_zone_5 = 26965;
static const int PCS_NAD83_Georgia_East = 26966;
static const int PCS_NAD83_Georgia_West = 26967;
static const int PCS_NAD83_Idaho_East = 26968;
static const int PCS_NAD83_Idaho_Central = 26969;
static const int PCS_NAD83_Idaho_West = 26970;
static const int PCS_NAD83_Illinois_East = 26971;
static const int PCS_NAD83_Illinois_West = 26972;
static const int PCS_NAD83_Indiana_East = 26973;
static const int PCS_NAD83_Indiana_West = 26974;
static const int PCS_NAD83_Iowa_North = 26975;
static const int PCS_NAD83_Iowa_South = 26976;
static const int PCS_NAD83_Kansas_North = 26977;
static const int PCS_NAD83_Kansas_South = 26978;
static const int PCS_NAD83_Kentucky_North = 2205;
static const int PCS_NAD83_Kentucky_South = 26980;
static const int PCS_NAD83_Louisiana_North = 26981;
static const int PCS_NAD83_Louisiana_South = 26982;
static const int PCS_NAD83_Maine_East = 26983;
static const int PCS_NAD83_Maine_West = 26984;
static const int PCS_NAD83_Maryland = 26985;
static const int PCS_NAD83_Massachusetts = 26986;
static const int PCS_NAD83_Massachusetts_Is = 26987;
static const int PCS_NAD83_Michigan_North = 26988;
static const int PCS_NAD83_Michigan_Central = 26989;
static const int PCS_NAD83_Michigan_South = 26990;
static const int PCS_NAD83_Minnesota_North = 26991;
static const int PCS_NAD83_Minnesota_Central = 26992;
static const int PCS_NAD83_Minnesota_South = 26993;
static const int PCS_NAD83_Mississippi_East = 26994;
static const int PCS_NAD83_Mississippi_West = 26995;
static const int PCS_NAD83_Missouri_East = 26996;
static const int PCS_NAD83_Missouri_Central = 26997;
static const int PCS_NAD83_Missouri_West = 26998;
static const int PCS_NAD83_Montana = 32100;
static const int PCS_NAD83_Nebraska = 32104;
static const int PCS_NAD83_Nevada_East = 32107;
static const int PCS_NAD83_Nevada_Central = 32108;
static const int PCS_NAD83_Nevada_West = 32109;
static const int PCS_NAD83_New_Hampshire = 32110;
static const int PCS_NAD83_New_Jersey = 32111;
static const int PCS_NAD83_New_Mexico_East = 32112;
static const int PCS_NAD83_New_Mexico_Central = 32113;
static const int PCS_NAD83_New_Mexico_West = 32114;
static const int PCS_NAD83_New_York_East = 32115;
static const int PCS_NAD83_New_York_Central = 32116;
static const int PCS_NAD83_New_York_West = 32117;
static const int PCS_NAD83_New_York_Long_Is = 32118;
static const int PCS_NAD83_North_Carolina = 32119;
static const int PCS_NAD83_North_Dakota_N = 32120;
static const int PCS_NAD83_North_Dakota_S = 32121;
static const int PCS_NAD83_Ohio_North = 32122;
static const int PCS_NAD83_Ohio_South = 32123;
static const int PCS_NAD83_Oklahoma_North = 32124;
static const int PCS_NAD83_Oklahoma_South = 32125;
static const int PCS_NAD83_Oregon_North = 32126;
static const int PCS_NAD83_Oregon_South = 32127;
static const int PCS_NAD83_Pennsylvania_N = 32128;
static const int PCS_NAD83_Pennsylvania_S = 32129;
static const int PCS_NAD83_Rhode_Island = 32130;
static const int PCS_NAD83_South_Carolina = 32133;
static const int PCS_NAD83_South_Dakota_N = 32134;
static const int PCS_NAD83_South_Dakota_S = 32135;
static const int PCS_NAD83_Tennessee = 32136;
static const int PCS_NAD83_Texas_North = 32137;
static const int PCS_NAD83_Texas_North_Central = 32138;
static const int PCS_NAD83_Texas_Central = 32139;
static const int PCS_NAD83_Texas_South_Central = 32140;
static const int PCS_NAD83_Texas_South = 32141;
static const int PCS_NAD83_Utah_North = 32142;
static const int PCS_NAD83_Utah_Central = 32143;
static const int PCS_NAD83_Utah_South = 32144;
static const int PCS_NAD83_Vermont = 32145;
static const int PCS_NAD83_Virginia_North = 32146;
static const int PCS_NAD83_Virginia_South = 32147;
static const int PCS_NAD83_Washington_North = 32148;
static const int PCS_NAD83_Washington_South = 32149;
static const int PCS_NAD83_West_Virginia_N = 32150;
static const int PCS_NAD83_West_Virginia_S = 32151;
static const int PCS_NAD83_Wisconsin_North = 32152;
static const int PCS_NAD83_Wisconsin_Central = 32153;
static const int PCS_NAD83_Wisconsin_South = 32154;
static const int PCS_NAD83_Wyoming_East = 32155;
static const int PCS_NAD83_Wyoming_East_Central = 32156;
static const int PCS_NAD83_Wyoming_West_Central = 32157;
static const int PCS_NAD83_Wyoming_West = 32158;
static const int PCS_NAD83_Puerto_Rico = 32161;

class StatePlaneLCC
{
public:
  StatePlaneLCC(short geokey, char* zone, F64 falseEasting, F64 falseNorthing, F64 latOriginDegree, F64 longMeridianDegree, F64 firstStdParallelDegree, F64 secondStdParallelDegree)
  {
    this->geokey = geokey;
    this->zone = zone;
    this->falseEasting = falseEasting;
    this->falseNorthing = falseNorthing;
    this->latOriginDegree = latOriginDegree;
    this->longMeridianDegree = longMeridianDegree;
    this->firstStdParallelDegree = firstStdParallelDegree;
    this->secondStdParallelDegree = secondStdParallelDegree;
  }
  short geokey;
  char* zone;
  F64 falseEasting;
  F64 falseNorthing;
  F64 latOriginDegree;
  F64 longMeridianDegree;
  F64 firstStdParallelDegree;
  F64 secondStdParallelDegree;
};

static const StatePlaneLCC state_plane_lcc_nad27_list[] =
{
  // zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), 1st std para, 2nd std para 
  StatePlaneLCC(PCS_NAD27_Alaska_zone_10, "AK_10",914401.8288,0,51,-176,51.83333333,53.83333333),
  StatePlaneLCC(PCS_NAD27_Arkansas_North, "AR_N",609601.2192,0,34.33333333,-92,34.93333333,36.23333333),
  StatePlaneLCC(PCS_NAD27_Arkansas_South, "AR_S",609601.2192,0,32.66666667,-92,33.3,34.76666667),
  StatePlaneLCC(PCS_NAD27_California_I, "CA_I",609601.2192,0,39.33333333,-122,40,41.66666667),
  StatePlaneLCC(PCS_NAD27_California_II, "CA_II",609601.2192,0,37.66666667,-122,38.33333333,39.83333333),
  StatePlaneLCC(PCS_NAD27_California_III, "CA_III",609601.2192,0,36.5,-120.5,37.06666667,38.43333333),
  StatePlaneLCC(PCS_NAD27_California_IV, "CA_IV",609601.2192,0,35.33333333,-119,36,37.25),
  StatePlaneLCC(PCS_NAD27_California_V, "CA_V",609601.2192,0,33.5,-118,34.03333333,35.46666667),
  StatePlaneLCC(PCS_NAD27_California_VI, "CA_VI",609601.2192,0,32.16666667,-116.25,32.78333333,33.88333333),
  StatePlaneLCC(PCS_NAD27_California_VII, "CA_VII",1276106.451,1268253.007,34.13333333,-118.3333333,33.86666667,34.41666667),
  StatePlaneLCC(PCS_NAD27_Colorado_North, "CO_N",609601.2192,0,39.33333333,-105.5,39.71666667,40.78333333),
  StatePlaneLCC(PCS_NAD27_Colorado_Central, "CO_C",609601.2192,0,37.83333333,-105.5,38.45,39.75),
  StatePlaneLCC(PCS_NAD27_Colorado_South, "CO_S",609601.2192,0,36.66666667,-105.5,37.23333333,38.43333333),
  StatePlaneLCC(PCS_NAD27_Connecticut, "CT",182880.3658,0,40.83333333,-72.75,41.2,41.86666667),
  StatePlaneLCC(PCS_NAD27_Florida_North, "FL_N",609601.2192,0,29,-84.5,29.58333333,30.75),
  StatePlaneLCC(PCS_NAD27_Iowa_North, "IA_N",609601.2192,0,41.5,-93.5,42.06666667,43.26666667),
  StatePlaneLCC(PCS_NAD27_Iowa_South, "IA_S",609601.2192,0,40,-93.5,40.61666667,41.78333333),
  StatePlaneLCC(PCS_NAD27_Kansas_North, "KS_N",609601.2192,0,38.33333333,-98,38.71666667,39.78333333),
  StatePlaneLCC(PCS_NAD27_Kansas_South, "KS_S",609601.2192,0,36.66666667,-98.5,37.26666667,38.56666667),
  StatePlaneLCC(PCS_NAD27_Kentucky_North, "KY_N",609601.2192,0,37.5,-84.25,37.96666667,38.96666667),
  StatePlaneLCC(PCS_NAD27_Kentucky_South, "KY_S",609601.2192,0,36.33333333,-85.75,36.73333333,37.93333333),
  StatePlaneLCC(PCS_NAD27_Louisiana_North, "LA_N",609601.2192,0,30.66666667,-92.5,31.16666667,32.66666667),
  StatePlaneLCC(PCS_NAD27_Louisiana_South, "LA_S",609601.2192,0,28.66666667,-91.33333333,29.3,30.7),
  StatePlaneLCC(PCS_NAD27_Maryland, "MD",243840.4877,0,37.83333333,-77,38.3,39.45),
  StatePlaneLCC(PCS_NAD27_Massachusetts, "MA_M",182880.3658,0,41,-71.5,41.71666667,42.68333333),
  StatePlaneLCC(PCS_NAD27_Massachusetts_Is, "MA_I",60960.12192,0,41,-70.5,41.28333333,41.48333333),
  StatePlaneLCC(PCS_NAD27_Michigan_North, "MI_N",609601.2192,0,44.78333333,-87,45.48333333,47.08333333),
  StatePlaneLCC(PCS_NAD27_Michigan_Central, "MI_C",609601.2192,0,43.31666667,-84.33333333,44.18333333,45.7),
  StatePlaneLCC(PCS_NAD27_Michigan_South, "MI_S",609601.2192,0,41.5,-84.33333333,42.1,43.66666667),
  StatePlaneLCC(PCS_NAD27_Minnesota_North, "MN_N",609601.2192,0,46.5,-93.1,47.03333333,48.63333333),
  StatePlaneLCC(PCS_NAD27_Minnesota_Central, "MN_C",609601.2192,0,45,-94.25,45.61666667,47.05),
  StatePlaneLCC(PCS_NAD27_Minnesota_South, "MN_S",609601.2192,0,43,-94,43.78333333,45.21666667),
  StatePlaneLCC(PCS_NAD27_Montana_North, "MT_N",609601.2192,0,47,-109.5,47.85,48.71666667),
  StatePlaneLCC(PCS_NAD27_Montana_Central, "MT_C",609601.2192,0,45.83333333,-109.5,46.45,47.88333333),
  StatePlaneLCC(PCS_NAD27_Montana_South, "MT_S",609601.2192,0,44,-109.5,44.86666667,46.4),
  StatePlaneLCC(PCS_NAD27_Nebraska_North, "NE_N",609601.2192,0,41.33333333,-100,41.85,42.81666667),
  StatePlaneLCC(PCS_NAD27_Nebraska_South, "NE_S",609601.2192,0,39.66666667,-99.5,40.28333333,41.71666667),
  StatePlaneLCC(PCS_NAD27_New_York_Long_Is, "NY_LI",609601.2192,30480.06096,40.5,-74,40.66666667,41.03333333),
  StatePlaneLCC(PCS_NAD27_North_Carolina, "NC",609601.2192,0,33.75,-79,34.33333333,36.16666667),
  StatePlaneLCC(PCS_NAD27_North_Dakota_N, "ND_N",609601.2192,0,47,-100.5,47.43333333,48.73333333),
  StatePlaneLCC(PCS_NAD27_North_Dakota_S, "ND_S",609601.2192,0,45.66666667,-100.5,46.18333333,47.48333333),
  StatePlaneLCC(PCS_NAD27_Ohio_North, "OH_N",609601.2192,0,39.66666667,-82.5,40.43333333,41.7),
  StatePlaneLCC(PCS_NAD27_Ohio_South, "OH_S",609601.2192,0,38,-82.5,38.73333333,40.03333333),
  StatePlaneLCC(PCS_NAD27_Oklahoma_North, "OK_N",609601.2192,0,35,-98,35.56666667,36.76666667),
  StatePlaneLCC(PCS_NAD27_Oklahoma_South, "OK_S",609601.2192,0,33.33333333,-98,33.93333333,35.23333333),
  StatePlaneLCC(PCS_NAD27_Oregon_North, "OR_N",609601.2192,0,43.66666667,-120.5,44.33333333,46),
  StatePlaneLCC(PCS_NAD27_Oregon_South, "OR_S",609601.2192,0,41.66666667,-120.5,42.33333333,44),
  StatePlaneLCC(PCS_NAD27_Pennsylvania_N, "PA_N",609601.2192,0,40.16666667,-77.75,40.88333333,41.95),
  StatePlaneLCC(PCS_NAD27_Pennsylvania_S, "PA_S",609601.2192,0,39.33333333,-77.75,39.93333333,40.96666667),
  StatePlaneLCC(PCS_NAD27_Puerto_Rico, "PR",152400.3048,0,17.83333333,-66.43333333,18.03333333,18.43333333),
  StatePlaneLCC(PCS_NAD27_St_Croix, "St.Croix",152400.3048,30480.06096,17.83333333,-66.43333333,18.03333333,18.43333333),
  StatePlaneLCC(PCS_NAD27_South_Carolina_N, "SC_N",609601.2192,0,33,-81,33.76666667,34.96666667),
  StatePlaneLCC(PCS_NAD27_South_Carolina_S, "SC_S",609601.2192,0,31.83333333,-81,32.33333333,33.66666667),
  StatePlaneLCC(PCS_NAD27_South_Dakota_N, "SD_N",609601.2192,0,43.83333333,-100,44.41666667,45.68333333),
  StatePlaneLCC(PCS_NAD27_South_Dakota_S, "SD_S",609601.2192,0,42.33333333,-100.3333333,42.83333333,44.4),
  StatePlaneLCC(PCS_NAD27_Tennessee, "TN",609601.2192,30480.06096,34.66666667,-86,35.25,36.41666667),
  StatePlaneLCC(PCS_NAD27_Texas_North, "TX_N",609601.2192,0,34,-101.5,34.65,36.18333333),
  StatePlaneLCC(PCS_NAD27_Texas_North_Central, "TX_NC",609601.2192,0,31.66666667,-97.5,32.13333333,33.96666667),
  StatePlaneLCC(PCS_NAD27_Texas_Central, "TX_C",609601.2192,0,29.66666667,-100.3333333,30.11666667,31.88333333),
  StatePlaneLCC(PCS_NAD27_Texas_South_Central, "TX_SC",609601.2192,0,27.83333333,-99,28.38333333,30.28333333),
  StatePlaneLCC(PCS_NAD27_Texas_South, "TX_S",609601.2192,0,25.66666667,-98.5,26.16666667,27.83333333),
  StatePlaneLCC(PCS_NAD27_Utah_North, "UT_N",609601.2192,0,40.33333333,-111.5,40.71666667,41.78333333),
  StatePlaneLCC(PCS_NAD27_Utah_Central, "UT_C",609601.2192,0,38.33333333,-111.5,39.01666667,40.65),
  StatePlaneLCC(PCS_NAD27_Utah_South, "UT_S",609601.2192,0,36.66666667,-111.5,37.21666667,38.35),
  StatePlaneLCC(PCS_NAD27_Virginia_North, "VA_N",609601.2192,0,37.66666667,-78.5,38.03333333,39.2),
  StatePlaneLCC(PCS_NAD27_Virginia_South, "VA_S",609601.2192,0,36.33333333,-78.5,36.76666667,37.96666667),
  StatePlaneLCC(PCS_NAD27_Washington_North, "WA_N",609601.2192,0,47,-120.8333333,47.5,48.73333333),
  StatePlaneLCC(PCS_NAD27_Washington_South, "WA_S",609601.2192,0,45.33333333,-120.5,45.83333333,47.33333333),
  StatePlaneLCC(PCS_NAD27_West_Virginia_N, "WV_N",609601.2192,0,38.5,-79.5,39,40.25),
  StatePlaneLCC(PCS_NAD27_West_Virginia_S, "WV_S",609601.2192,0,37,-81,37.48333333,38.88333333),
  StatePlaneLCC(PCS_NAD27_Wisconsin_North, "WI_N",609601.2192,0,45.16666667,-90,45.56666667,46.76666667),
  StatePlaneLCC(PCS_NAD27_Wisconsin_Central, "WI_C",609601.2192,0,43.83333333,-90,44.25,45.5),
  StatePlaneLCC(PCS_NAD27_Wisconsin_South, "WI_S",609601.2192,0,42,-90,42.73333333,44.06666667),
  StatePlaneLCC(0,0,-1,-1,-1,-1,-1,-1)
};

static const StatePlaneLCC state_plane_lcc_nad83_list[] =
{
  // geotiff key, zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), 1st std para, 2nd std para 
  StatePlaneLCC(PCS_NAD83_Alaska_zone_10, "AK_10",1000000,0,51.000000,-176.000000,51.833333,53.833333),
  StatePlaneLCC(PCS_NAD83_Arkansas_North, "AR_N",400000,0,34.333333,-92.000000,34.933333,36.233333),
  StatePlaneLCC(PCS_NAD83_Arkansas_South, "AR_S",400000,400000,32.666667,-92.000000,33.300000,34.766667),
  StatePlaneLCC(PCS_NAD83_California_1, "CA_I",2000000,500000,39.333333,-122.000000,40.000000,41.666667),
  StatePlaneLCC(PCS_NAD83_California_2, "CA_II",2000000,500000,37.666667,-122.000000,38.333333,39.833333),
  StatePlaneLCC(PCS_NAD83_California_3, "CA_III",2000000,500000,36.500000,-120.500000,37.066667,38.433333),
  StatePlaneLCC(PCS_NAD83_California_4, "CA_IV",2000000,500000,35.333333,-119.000000,36.000000,37.250000),
  StatePlaneLCC(PCS_NAD83_California_5, "CA_V",2000000,500000,33.500000,-118.000000,34.033333,35.466667),
  StatePlaneLCC(PCS_NAD83_California_6, "CA_VI",2000000,500000,32.166667,-116.250000,32.783333,33.883333),
  StatePlaneLCC(PCS_NAD83_Colorado_North, "CO_N",914401.8289,304800.6096,39.333333,-105.500000,39.716667,40.783333),
  StatePlaneLCC(PCS_NAD83_Colorado_Central, "CO_C",914401.8289,304800.6096,37.833333,-105.500000,38.450000,39.750000),
  StatePlaneLCC(PCS_NAD83_Colorado_South, "CO_S",914401.8289,304800.6096,36.666667,-105.500000,37.233333,38.433333),
  StatePlaneLCC(PCS_NAD83_Connecticut, "CT",304800.6096,152400.3048,40.833333,-72.750000,41.200000,41.866667),
  StatePlaneLCC(PCS_NAD83_Florida_North, "FL_N",600000,0,29.000000,-84.500000,29.583333,30.750000),
  StatePlaneLCC(PCS_NAD83_Iowa_North, "IA_N",1500000,1000000,41.500000,-93.500000,42.066667,43.266667),
  StatePlaneLCC(PCS_NAD83_Iowa_South, "IA_S",500000,0,40.000000,-93.500000,40.616667,41.783333),
  StatePlaneLCC(PCS_NAD83_Kansas_North, "KS_N",400000,0,38.333333,-98.000000,38.716667,39.783333),
  StatePlaneLCC(PCS_NAD83_Kansas_South, "KS_S",400000,400000,36.666667,-98.500000,37.266667,38.566667),
  StatePlaneLCC(PCS_NAD83_Kentucky_North, "KY_N",500000,0,37.500000,-84.250000,37.966667,38.966667),
  StatePlaneLCC(PCS_NAD83_Kentucky_South, "KY_S",500000,500000,36.333333,-85.750000,36.733333,37.933333),
  StatePlaneLCC(PCS_NAD83_Louisiana_North, "LA_N",1000000,0,30.500000,-92.500000,31.166667,32.666667),
  StatePlaneLCC(PCS_NAD83_Louisiana_South, "LA_S",1000000,0,28.500000,-91.333333,29.300000,30.700000),
  StatePlaneLCC(PCS_NAD83_Maryland, "MD",400000,0,37.666667,-77.000000,38.300000,39.450000),
  StatePlaneLCC(PCS_NAD83_Massachusetts, "MA_M",200000,750000,41.000000,-71.500000,41.716667,42.683333),
  StatePlaneLCC(PCS_NAD83_Massachusetts_Is, "MA_I",500000,0,41.000000,-70.500000,41.283333,41.483333),
  StatePlaneLCC(PCS_NAD83_Michigan_North, "MI_N",8000000,0,44.783333,-87.000000,45.483333,47.083333),
  StatePlaneLCC(PCS_NAD83_Michigan_Central, "MI_C",6000000,0,43.316667,-84.366667,44.183333,45.700000),
  StatePlaneLCC(PCS_NAD83_Michigan_South, "MI_S",4000000,0,41.500000,-84.366667,42.100000,43.666667),
  StatePlaneLCC(PCS_NAD83_Minnesota_North, "MN_N",800000,100000,46.500000,-93.100000,47.033333,48.633333),
  StatePlaneLCC(PCS_NAD83_Minnesota_Central, "MN_C",800000,100000,45.000000,-94.250000,45.616667,47.050000),
  StatePlaneLCC(PCS_NAD83_Minnesota_South, "MN_S",800000,100000,43.000000,-94.000000,43.783333,45.216667),
  StatePlaneLCC(PCS_NAD83_Montana, "MT",600000,0,44.250000,-109.500000,45.000000,49.000000),
  StatePlaneLCC(PCS_NAD83_Nebraska, "NE",500000,0,39.833333,-100.000000,40.000000,43.000000),
  StatePlaneLCC(PCS_NAD83_New_York_Long_Is, "NY_LI",300000,0,40.166667,-74.000000,40.666667,41.033333),
  StatePlaneLCC(PCS_NAD83_North_Carolina, "NC",609601.22,0,33.750000,-79.000000,34.333333,36.166667),
  StatePlaneLCC(PCS_NAD83_North_Dakota_N, "ND_N",600000,0,47.000000,-100.500000,47.433333,48.733333),
  StatePlaneLCC(PCS_NAD83_North_Dakota_S, "ND_S",600000,0,45.666667,-100.500000,46.183333,47.483333),
  StatePlaneLCC(PCS_NAD83_Ohio_North, "OH_N",600000,0,39.666667,-82.500000,40.433333,41.700000),
  StatePlaneLCC(PCS_NAD83_Ohio_South, "OH_S",600000,0,38.000000,-82.500000,38.733333,40.033333),
  StatePlaneLCC(PCS_NAD83_Oklahoma_North, "OK_N",600000,0,35.000000,-98.000000,35.566667,36.766667),
  StatePlaneLCC(PCS_NAD83_Oklahoma_South, "OK_S",600000,0,33.333333,-98.000000,33.933333,35.233333),
  StatePlaneLCC(PCS_NAD83_Oregon_North, "OR_N",2500000,0,43.666667,-120.500000,44.333333,46.000000),
  StatePlaneLCC(PCS_NAD83_Oregon_South, "OR_S",1500000,0,41.666667,-120.500000,42.333333,44.000000),
  StatePlaneLCC(PCS_NAD83_Pennsylvania_N, "PA_N",600000,0,40.166667,-77.750000,40.883333,41.950000),
  StatePlaneLCC(PCS_NAD83_Pennsylvania_S, "PA_S",600000,0,39.333333,-77.750000,39.933333,40.966667),
  StatePlaneLCC(PCS_NAD83_Puerto_Rico, "PR",200000,200000,17.833333,-66.433333,18.033333,18.433333),
  StatePlaneLCC(PCS_NAD83_South_Carolina, "SC",609600,0,31.833333,-81.000000,32.500000,34.833333),
  StatePlaneLCC(PCS_NAD83_South_Dakota_N, "SD_N",600000,0,43.833333,-100.000000,44.416667,45.683333),
  StatePlaneLCC(PCS_NAD83_South_Dakota_S, "SD_S",600000,0,42.333333,-100.333333,42.833333,44.400000),
  StatePlaneLCC(PCS_NAD83_Tennessee, "TN",600000,0,34.333333,-86.000000,35.250000,36.416667),
  StatePlaneLCC(PCS_NAD83_Texas_North, "TX_N",200000,1000000,34.000000,-101.500000,34.650000,36.183333),
  StatePlaneLCC(PCS_NAD83_Texas_North_Central, "TX_NC",600000,2000000,31.666667,-98.500000,32.133333,33.966667),
  StatePlaneLCC(PCS_NAD83_Texas_Central, "TX_C",700000,3000000,29.666667,-100.333333,30.116667,31.883333),
  StatePlaneLCC(PCS_NAD83_Texas_South_Central, "TX_SC",600000,4000000,27.833333,-99.000000,28.383333,30.283333),
  StatePlaneLCC(PCS_NAD83_Texas_South, "TX_S",300000,5000000,25.666667,-98.500000,26.166667,27.833333),
  StatePlaneLCC(PCS_NAD83_Utah_North, "UT_N",500000,1000000,40.333333,-111.500000,40.716667,41.783333),
  StatePlaneLCC(PCS_NAD83_Utah_Central, "UT_C",500000,2000000,38.333333,-111.500000,39.016667,40.650000),
  StatePlaneLCC(PCS_NAD83_Utah_South, "UT_S",500000,3000000,36.666667,-111.500000,37.216667,38.350000),
  StatePlaneLCC(PCS_NAD83_Virginia_North, "VA_N",3500000,2000000,37.666667,-78.500000,38.033333,39.200000),
  StatePlaneLCC(PCS_NAD83_Virginia_South, "VA_S",3500000,1000000,36.333333,-78.500000,36.766667,37.966667),
  StatePlaneLCC(PCS_NAD83_Washington_North, "WA_N",500000,0,47.000000,-120.833333,47.500000,48.733333),
  StatePlaneLCC(PCS_NAD83_Washington_South, "WA_S",500000,0,45.333333,-120.500000,45.833333,47.333333),
  StatePlaneLCC(PCS_NAD83_West_Virginia_N, "WV_N",600000,0,38.500000,-79.500000,39.000000,40.250000),
  StatePlaneLCC(PCS_NAD83_West_Virginia_S, "WV_S",600000,0,37.000000,-81.000000,37.483333,38.883333),
  StatePlaneLCC(PCS_NAD83_Wisconsin_North, "WI_N",600000,0,45.166667,-90.000000,45.566667,46.766667),
  StatePlaneLCC(PCS_NAD83_Wisconsin_Central, "WI_C",600000,0,43.833333,-90.000000,44.250000,45.500000),
  StatePlaneLCC(PCS_NAD83_Wisconsin_South, "WI_S",600000,0,42.000000,-90.000000,42.733333,44.066667),
  StatePlaneLCC(0,0,-1,-1,-1,-1,-1,-1)
};

class StatePlaneTM
{
public:
  StatePlaneTM(short geokey, char* zone, F64 falseEasting, F64 falseNorthing, F64 latOriginDegree, F64 longMeridianDegree, F64 scaleFactor)
  {
    this->geokey = geokey;
    this->zone = zone;
    this->falseEasting = falseEasting;
    this->falseNorthing = falseNorthing;
    this->latOriginDegree = latOriginDegree;
    this->longMeridianDegree = longMeridianDegree;
    this->scaleFactor = scaleFactor;
  }
  short geokey;
  char* zone;
  F64 falseEasting;
  F64 falseNorthing;
  F64 latOriginDegree;
  F64 longMeridianDegree;
  F64 scaleFactor;
};

static const StatePlaneTM state_plane_tm_nad27_list[] =
{
  // geotiff key, zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), scale factor
  StatePlaneTM(PCS_NAD27_Alabama_East, "AL_E",152400.3048,0,30.5,-85.83333333,0.99996),
  StatePlaneTM(PCS_NAD27_Alabama_West, "AL_W",152400.3048,0,30,-87.5,0.999933333),
  StatePlaneTM(PCS_NAD27_Alaska_zone_2, "AK_2",152400.3048,0,54,-142,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_3, "AK_3",152400.3048,0,54,-146,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_4, "AK_4",152400.3048,0,54,-150,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_5, "AK_5",152400.3048,0,54,-154,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_6, "AK_6",152400.3048,0,54,-158,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_7, "AK_7",213360.4267,0,54,-162,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_8, "AK_8",152400.3048,0,54,-166,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_9, "AK_9",182880.3658,0,54,-170,0.9999),
  StatePlaneTM(PCS_NAD27_Arizona_East, "AZ_E",152400.3048,0,31,-110.1666667,0.9999),
  StatePlaneTM(PCS_NAD27_Arizona_Central, "AZ_C",152400.3048,0,31,-111.9166667,0.9999),
  StatePlaneTM(PCS_NAD27_Arizona_West, "AZ_W",152400.3048,0,31,-113.75,0.999933333),
  StatePlaneTM(PCS_NAD27_Delaware, "DE",152400.3048,0,38,-75.41666667,0.999995),
  StatePlaneTM(PCS_NAD27_Florida_East, "FL_E",152400.3048,0,24.33333333,-81,0.999941177),
  StatePlaneTM(PCS_NAD27_Florida_West, "FL_W",152400.3048,0,24.33333333,-82,0.999941177),
  StatePlaneTM(PCS_NAD27_Georgia_East, "GA_E",152400.3048,0,30,-82.16666667,0.9999),
  StatePlaneTM(PCS_NAD27_Georgia_West, "GA_W",152400.3048,0,30,-84.16666667,0.9999),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_1, "HI_1",152400.3048,0,18.83333333,-155.5,0.999966667),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_2, "HI_2",152400.3048,0,20.33333333,-156.6666667,0.999966667),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_3, "HI_3",152400.3048,0,21.16666667,-158,0.99999),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_4, "HI_4",152400.3048,0,21.83333333,-159.5,0.99999),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_5, "HI_5",152400.3048,0,21.66666667,-160.1666667,1),
  StatePlaneTM(PCS_NAD27_Idaho_East, "ID_E",152400.3048,0,41.66666667,-112.1666667,0.999947368),
  StatePlaneTM(PCS_NAD27_Idaho_Central, "ID_C",152400.3048,0,41.66666667,-114,0.999947368),
  StatePlaneTM(PCS_NAD27_Idaho_West, "ID_W",152400.3048,0,41.66666667,-115.75,0.999933333),
  StatePlaneTM(PCS_NAD27_Illinois_East, "IL_E",152400.3048,0,36.66666667,-88.33333333,0.999975),
  StatePlaneTM(PCS_NAD27_Illinois_West, "IL_W",152400.3048,0,36.66666667,-90.16666667,0.999941177),
  StatePlaneTM(PCS_NAD27_Indiana_East, "IN_E",152400.3048,0,37.5,-85.66666667,0.999966667),
  StatePlaneTM(PCS_NAD27_Indiana_West, "IN_W",152400.3048,0,37.5,-87.08333333,0.999966667),
  StatePlaneTM(PCS_NAD27_Maine_East, "ME_E",152400.3048,0,43.83333333,-68.5,0.9999),
  StatePlaneTM(PCS_NAD27_Maine_West, "ME_W",152400.3048,0,42.83333333,-70.16666667,0.999966667),
  StatePlaneTM(PCS_NAD27_Mississippi_East, "MS_E",152400.3048,0,29.66666667,-88.83333333,0.99996),
  StatePlaneTM(PCS_NAD27_Mississippi_West, "MS_W",152400.3048,0,30.5,-90.33333333,0.999941177),
  StatePlaneTM(PCS_NAD27_Missouri_East, "MO_E",152400.3048,0,35.83333333,-90.5,0.999933333),
  StatePlaneTM(PCS_NAD27_Missouri_Central, "MO_C",152400.3048,0,35.83333333,-92.5,0.999933333),
  StatePlaneTM(PCS_NAD27_Missouri_West, "MO_W",152400.3048,0,36.16666667,-94.5,0.999941177),
  StatePlaneTM(PCS_NAD27_Nevada_East, "NV_E",152400.3048,0,34.75,-115.5833333,0.9999),
  StatePlaneTM(PCS_NAD27_Nevada_Central, "NV_C",152400.3048,0,34.75,-116.6666667,0.9999),
  StatePlaneTM(PCS_NAD27_Nevada_West, "NV_W",152400.3048,0,34.75,-118.5833333,0.9999),
  StatePlaneTM(PCS_NAD27_New_Hampshire, "NH",152400.3048,0,42.5,-71.66666667,0.999966667),
  StatePlaneTM(PCS_NAD27_New_Jersey, "NJ",609601.2192,0,38.83333333,-74.66666667,0.999975),
  StatePlaneTM(PCS_NAD27_New_Mexico_East, "NM_E",152400.3048,0,31,-104.3333333,0.999909091),
  StatePlaneTM(PCS_NAD27_New_Mexico_Central, "NM_C",152400.3048,0,31,-106.25,0.9999),
  StatePlaneTM(PCS_NAD27_New_Mexico_West, "NM_W",152400.3048,0,31,-107.8333333,0.999916667),
  StatePlaneTM(PCS_NAD27_New_York_East, "NY_E",152400.3048,0,40,-74.33333333,0.999966667),
  StatePlaneTM(PCS_NAD27_New_York_Central, "NY_C",152400.3048,0,40,-76.58333333,0.9999375),
  StatePlaneTM(PCS_NAD27_New_York_West, "NY_W",152400.3048,0,40,-78.58333333,0.9999375),
  StatePlaneTM(PCS_NAD27_Rhode_Island, "RI",152400.3048,0,41.08333333,-71.5,0.99999375),
  StatePlaneTM(PCS_NAD27_Vermont, "VT",152400.3048,0,42.5,-72.5,0.999964286),
  StatePlaneTM(PCS_NAD27_Wyoming_East, "WY_E",152400.3048,0,40.66666667,-105.1666667,0.999941177),
  StatePlaneTM(PCS_NAD27_Wyoming_East_Central, "WY_EC",152400.3048,0,40.66666667,-107.3333333,0.999941177),
  StatePlaneTM(PCS_NAD27_Wyoming_West_Central, "WY_WC",152400.3048,0,40.66666667,-108.75,0.999941177),
  StatePlaneTM(PCS_NAD27_Wyoming_West, "WY_W",152400.3048,0,40.66666667,-110.0833333,0.999941177),
  StatePlaneTM(0,0,-1,-1,-1,-1,-1)
};

static const StatePlaneTM state_plane_tm_nad83_list[] =
{
  // geotiff key, zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), scale factor
  StatePlaneTM(PCS_NAD83_Alabama_East, "AL_E",200000,0,30.5,-85.83333333,0.99996),
  StatePlaneTM(PCS_NAD83_Alabama_West, "AL_W",600000,0,30,-87.5,0.999933333),
  StatePlaneTM(PCS_NAD83_Alaska_zone_2, "AK_2",500000,0,54,-142,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_3, "AK_3",500000,0,54,-146,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_4, "AK_4",500000,0,54,-150,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_5, "AK_5",500000,0,54,-154,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_6, "AK_6",500000,0,54,-158,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_7, "AK_7",500000,0,54,-162,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_8, "AK_8",500000,0,54,-166,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_9, "AK_9",500000,0,54,-170,0.9999),
  StatePlaneTM(PCS_NAD83_Arizona_East, "AZ_E",213360,0,31,-110.1666667,0.9999),
  StatePlaneTM(PCS_NAD83_Arizona_Central, "AZ_C",213360,0,31,-111.9166667,0.9999),
  StatePlaneTM(PCS_NAD83_Arizona_West, "AZ_W",213360,0,31,-113.75,0.999933333),
  StatePlaneTM(PCS_NAD83_Delaware, "DE",200000,0,38,-75.41666667,0.999995),
  StatePlaneTM(PCS_NAD83_Florida_East, "FL_E",200000,0,24.33333333,-81,0.999941177),
  StatePlaneTM(PCS_NAD83_Florida_West, "FL_W",200000,0,24.33333333,-82,0.999941177),
  StatePlaneTM(PCS_NAD83_Georgia_East, "GA_E",200000,0,30,-82.16666667,0.9999),
  StatePlaneTM(PCS_NAD83_Georgia_West, "GA_W",700000,0,30,-84.16666667,0.9999),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_1, "HI_1",500000,0,18.83333333,-155.5,0.999966667),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_2, "HI_2",500000,0,20.33333333,-156.6666667,0.999966667),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_3, "HI_3",500000,0,21.16666667,-158,0.99999),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_4, "HI_4",500000,0,21.83333333,-159.5,0.99999),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_5, "HI_5",500000,0,21.66666667,-160.1666667,1),
  StatePlaneTM(PCS_NAD83_Idaho_East, "ID_E",200000,0,41.66666667,-112.1666667,0.999947368),
  StatePlaneTM(PCS_NAD83_Idaho_Central, "ID_C",500000,0,41.66666667,-114,0.999947368),
  StatePlaneTM(PCS_NAD83_Idaho_West, "ID_W",800000,0,41.66666667,-115.75,0.999933333),
  StatePlaneTM(PCS_NAD83_Illinois_East, "IL_E",300000,0,36.66666667,-88.33333333,0.999975),
  StatePlaneTM(PCS_NAD83_Illinois_West, "IL_W",700000,0,36.66666667,-90.16666667,0.999941177),
  StatePlaneTM(PCS_NAD83_Indiana_East, "IN_E",100000,250000,37.5,-85.66666667,0.999966667),
  StatePlaneTM(PCS_NAD83_Indiana_West, "IN_W",900000,250000,37.5,-87.08333333,0.999966667),
  StatePlaneTM(PCS_NAD83_Maine_East, "ME_E",300000,0,43.66666667,-68.5,0.9999),
  StatePlaneTM(PCS_NAD83_Maine_West, "ME_W",900000,0,42.83333333,-70.16666667,0.999966667),
  StatePlaneTM(PCS_NAD83_Mississippi_East, "MS_E",300000,0,29.5,-88.83333333,0.99995),
  StatePlaneTM(PCS_NAD83_Mississippi_West, "MS_W",700000,0,29.5,-90.33333333,0.99995),
  StatePlaneTM(PCS_NAD83_Missouri_East, "MO_E",250000,0,35.83333333,-90.5,0.999933333),
  StatePlaneTM(PCS_NAD83_Missouri_Central, "MO_C",500000,0,35.83333333,-92.5,0.999933333),
  StatePlaneTM(PCS_NAD83_Missouri_West, "MO_W",850000,0,36.16666667,-94.5,0.999941177),
  StatePlaneTM(PCS_NAD83_Nevada_East, "NV_E",200000,8000000,34.75,-115.5833333,0.9999),
  StatePlaneTM(PCS_NAD83_Nevada_Central, "NV_C",500000,6000000,34.75,-116.6666667,0.9999),
  StatePlaneTM(PCS_NAD83_Nevada_West, "NV_W",800000,4000000,34.75,-118.5833333,0.9999),
  StatePlaneTM(PCS_NAD83_New_Hampshire, "NH",300000,0,42.5,-71.66666667,0.999966667),
  StatePlaneTM(PCS_NAD83_New_Jersey, "NJ",150000,0,38.83333333,-74.5,0.9999),
  StatePlaneTM(PCS_NAD83_New_Mexico_East, "NM_E",165000,0,31,-104.3333333,0.999909091),
  StatePlaneTM(PCS_NAD83_New_Mexico_Central, "NM_C",500000,0,31,-106.25,0.9999),
  StatePlaneTM(PCS_NAD83_New_Mexico_West, "NM_W",830000,0,31,-107.8333333,0.999916667),
  StatePlaneTM(PCS_NAD83_New_York_East, "NY_E",150000,0,38.83333333,-74.5,0.9999),
  StatePlaneTM(PCS_NAD83_New_York_Central, "NY_C",250000,0,40,-76.58333333,0.9999375),
  StatePlaneTM(PCS_NAD83_New_York_West, "NY_W",350000,0,40,-78.58333333,0.9999375),
  StatePlaneTM(PCS_NAD83_Rhode_Island, "RI",100000,0,41.08333333,-71.5,0.99999375),
  StatePlaneTM(PCS_NAD83_Vermont, "VT",500000,0,42.5,-72.5,0.999964286),
  StatePlaneTM(PCS_NAD83_Wyoming_East, "WY_E",200000,0,40.5,-105.1666667,0.9999375),
  StatePlaneTM(PCS_NAD83_Wyoming_East_Central, "WY_EC",400000,100000,40.5,-107.3333333,0.9999375),
  StatePlaneTM(PCS_NAD83_Wyoming_West_Central, "WY_WC",600000,0,40.5,-108.75,0.9999375),
  StatePlaneTM(PCS_NAD83_Wyoming_West, "WY_W",800000,100000,40.5,-110.0833333,0.9999375),
  StatePlaneTM(0,0,-1,-1,-1,-1,-1)
};

static I32 lidardouble2string(CHAR* string, F64 value)
{
  I32 len;
  len = sprintf(string, "%.15f", value) - 1;
  while (string[len] == '0') len--;
  if (string[len] != '.') len++;
  string[len] = '\0';
  return len;
};

static I32 lidardouble2string(CHAR* string, F64 value, F64 scale)
{
  I32 decimal_digits = 0;
  while (scale < 1.0)
  {
    scale *= 10;
    decimal_digits++;
  }
  if (decimal_digits == 0)
    sprintf(string, "%d", (I32)value);
  else if (decimal_digits == 1)
    sprintf(string, "%.1f", value);
  else if (decimal_digits == 2)
    sprintf(string, "%.2f", value);
  else if (decimal_digits == 3)
    sprintf(string, "%.3f", value);
  else if (decimal_digits == 4)
    sprintf(string, "%.4f", value);
  else if (decimal_digits == 5)
    sprintf(string, "%.5f", value);
  else if (decimal_digits == 6)
    sprintf(string, "%.6f", value);
  else if (decimal_digits == 7)
    sprintf(string, "%.7f", value);
  else if (decimal_digits == 8)
    sprintf(string, "%.8f", value);
  else
    return lidardouble2string(string, value);
  return strlen(string)-1;
};

void CRScheck::set_coordinates_in_survey_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 3;
  else
    coordinate_units[1] = 3;
}

void CRScheck::set_coordinates_in_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 2;
  else
    coordinate_units[1] = 2;
}

void CRScheck::set_coordinates_in_meter(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 1;
  else
    coordinate_units[1] = 1;
}

void CRScheck::set_elevation_in_survey_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 3;
  else
    coordinate_units[1] = 3;
}

void CRScheck::set_elevation_in_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 2;
  else
    coordinate_units[1] = 2;
}

void CRScheck::set_elevation_in_meter(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 1;
  else
    coordinate_units[1] = 1;
}

BOOL CRScheck::set_ellipsoid(const I32 ellipsoid_id, const BOOL from_geokeys, char* description)
{
  if ((ellipsoid_id <= 0) || (ellipsoid_id >= 25))
  {
    return FALSE;
  }

  CRSprojectionEllipsoid* ellipsoid;

  if (from_geokeys)
  {
    if (ellipsoids[0] == 0)
    {
      ellipsoids[0] = new CRSprojectionEllipsoid();
    }
    ellipsoid = ellipsoids[0];
  }
  else
  {
    if (ellipsoids[1] == 0)
    {
      ellipsoids[1] = new CRSprojectionEllipsoid();
    }
    ellipsoid = ellipsoids[1];
  }

  ellipsoid->id = ellipsoid_id;
  ellipsoid->name = ellipsoid_list[ellipsoid_id].name;
  ellipsoid->equatorial_radius = ellipsoid_list[ellipsoid_id].equatorialRadius;
  ellipsoid->eccentricity_squared = ellipsoid_list[ellipsoid_id].eccentricitySquared;
  ellipsoid->eccentricity_prime_squared = (ellipsoid->eccentricity_squared)/(1-ellipsoid->eccentricity_squared);
  ellipsoid->polar_radius = ellipsoid->equatorial_radius*sqrt(1-ellipsoid->eccentricity_squared);    
  ellipsoid->eccentricity = sqrt(ellipsoid->eccentricity_squared);
  ellipsoid->eccentricity_e1 = (1-sqrt(1-ellipsoid->eccentricity_squared))/(1+sqrt(1-ellipsoid->eccentricity_squared));

  if (description)
  {
    sprintf(description, "%2d - %s (%g %g)", ellipsoid->id, ellipsoid->name, ellipsoid->equatorial_radius, ellipsoid->eccentricity_squared);
  }

  return TRUE;
}

void CRScheck::set_projection(CRSprojectionParameters* projection, const BOOL from_geokeys)
{
  if (from_geokeys)
  {
    if (projections[0]) delete projections[0];
    projections[0] = projection;
  }
  else
  {
    if (projections[1]) delete projections[1];
    projections[1] = projection;
  }
}

BOOL CRScheck::set_latlong_projection(const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParameters* latlong = new CRSprojectionParameters();
  latlong->type = CRS_PROJECTION_LAT_LONG;
  sprintf(latlong->name, "latitude/longitude");
  set_projection(latlong, from_geokeys);
  if (description)
  {
    sprintf(description, "%s", latlong->name);
  }
  return TRUE;
}

BOOL CRScheck::set_longlat_projection(const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParameters* longlat = new CRSprojectionParameters();
  longlat->type = CRS_PROJECTION_LONG_LAT;
  sprintf(longlat->name, "longitude/latitude");
  set_projection(longlat, from_geokeys);
  if (description)
  {
    sprintf(description, "%s", longlat->name);
  }
  return TRUE;
}

BOOL CRScheck::set_utm_projection(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 zone_number;
  CHAR* zone_letter;
  zone_number = strtoul(zone, &zone_letter, 10);
  if ((*zone_letter < 'C') || (*zone_letter > 'X'))
  {
    return FALSE;
  }
  if ((zone_number < 0) || (zone_number > 60))
  {
    return FALSE;
  }
  CRSprojectionParametersUTM* utm = new CRSprojectionParametersUTM();
  utm->type = CRS_PROJECTION_UTM;
  utm->utm_zone_number = zone_number;
  utm->utm_zone_letter = *zone_letter;
  if((*zone_letter - 'N') >= 0)
  {
    utm->utm_northern_hemisphere = TRUE; // point is in northern hemisphere
  }
  else
  {
    utm->utm_northern_hemisphere = FALSE; //point is in southern hemisphere
  }
  sprintf(utm->name, "UTM zone %s (%s)", zone, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  utm->utm_long_origin = (zone_number - 1) * 6 - 180 + 3; // + 3 puts origin in middle of zone
  set_projection(utm, from_geokeys);
  if (description)
  {
    sprintf(description, "UTM %d %s", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  }
  return TRUE;
}

BOOL CRScheck::set_utm_projection(const I32 zone_number, const BOOL northern, const BOOL from_geokeys, CHAR* description)
{
  if ((zone_number < 0) || (zone_number > 60))
  {
    return FALSE;
  }
  CRSprojectionParametersUTM* utm = new CRSprojectionParametersUTM();
  utm->type = CRS_PROJECTION_UTM;
  utm->utm_zone_number = zone_number;
  utm->utm_zone_letter = ' ';
  utm->utm_northern_hemisphere = northern;
  sprintf(utm->name, "UTM zone %d (%s)", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  utm->utm_long_origin = (zone_number - 1) * 6 - 180 + 3;  // + 3 puts origin in middle of zone
  set_projection(utm, from_geokeys);
  if (description)
  {
    sprintf(description, "UTM %d %s", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  }
  return TRUE;
}

// Configure a Lambert Conic Conformal Projection
//
// The function Set_Lambert_Parameters receives the ellipsoid parameters and
// Lambert Conformal Conic projection parameters as inputs, and sets the
// corresponding state variables.
//
// falseEasting & falseNorthing are just an offset in meters added 
// to the final coordinate calculated.
//
// latOriginDegree & longMeridianDegree are the "center" latitiude and
// longitude in decimal degrees of the area being projected. All coordinates
// will be calculated in meters relative to this point on the earth.
//
// firstStdParallelDegree & secondStdParallelDegree are the two lines of
// longitude in decimal degrees (that is they run east-west) that define
// where the "cone" intersects the earth. They bracket the area being projected.
void CRScheck::set_lambert_conformal_conic_projection(const F64 falseEasting, const F64 falseNorthing, const F64 latOriginDegree, const F64 longMeridianDegree, const F64 firstStdParallelDegree, const F64 secondStdParallelDegree, const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParametersLCC* lcc = new CRSprojectionParametersLCC();
  lcc->type = CRS_PROJECTION_LCC;
  sprintf(lcc->name, "Lambert Conformal Conic");
  lcc->lcc_false_easting_meter = falseEasting;
  lcc->lcc_false_northing_meter = falseNorthing;
  lcc->lcc_lat_origin_degree = latOriginDegree;
  lcc->lcc_long_meridian_degree = longMeridianDegree;
  lcc->lcc_first_std_parallel_degree = firstStdParallelDegree;
  lcc->lcc_second_std_parallel_degree = secondStdParallelDegree;
  lcc->lcc_lat_origin_radian = deg2rad*lcc->lcc_lat_origin_degree;
  lcc->lcc_long_meridian_radian = deg2rad*lcc->lcc_long_meridian_degree;
  lcc->lcc_first_std_parallel_radian = deg2rad*lcc->lcc_first_std_parallel_degree;
  lcc->lcc_second_std_parallel_radian = deg2rad*lcc->lcc_second_std_parallel_degree;
  set_projection(lcc, from_geokeys);
  if (description)
  {
    sprintf(description, "false east/north: %g/%g [m], origin lat/ meridian long: %g/%g, parallel 1st/2nd: %g/%g", lcc->lcc_false_easting_meter, lcc->lcc_false_northing_meter, lcc->lcc_lat_origin_degree, lcc->lcc_long_meridian_degree, lcc->lcc_first_std_parallel_degree, lcc->lcc_second_std_parallel_degree);
  }
}

/*
  * The function set_transverse_mercator_projection receives the Tranverse
  * Mercator projection parameters as input and sets the corresponding state
  * variables. 
  * falseEasting   : Easting/X in meters at the center of the projection
  * falseNorthing  : Northing/Y in meters at the center of the projection
  * latOriginDegree     : Latitude in decimal degree at the origin of the projection
  * longMeridianDegree  : Longitude n decimal degree at the center of the projection
  * scaleFactor         : Projection scale factor
*/
void CRScheck::set_transverse_mercator_projection(const F64 falseEasting, const F64 falseNorthing, const F64 latOriginDegree, const F64 longMeridianDegree, const F64 scaleFactor, const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParametersTM* tm = new CRSprojectionParametersTM();
  tm->type = CRS_PROJECTION_TM;
  sprintf(tm->name, "Transverse Mercator");
  tm->tm_false_easting_meter = falseEasting;
  tm->tm_false_northing_meter = falseNorthing;
  tm->tm_lat_origin_degree = latOriginDegree;
  tm->tm_long_meridian_degree = longMeridianDegree;
  tm->tm_scale_factor = scaleFactor;
  tm->tm_lat_origin_radian = deg2rad*tm->tm_lat_origin_degree;
  tm->tm_long_meridian_radian = deg2rad*tm->tm_long_meridian_degree;
  set_projection(tm, from_geokeys);
  if (description)
  {
    sprintf(description, "false east/north: %g/%g [m], origin lat/meridian long: %g/%g, scale: %g", tm->tm_false_easting_meter, tm->tm_false_northing_meter, tm->tm_lat_origin_degree, tm->tm_long_meridian_degree, tm->tm_scale_factor);
  }
}

BOOL CRScheck::set_state_plane_nad27_lcc(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_lcc_nad27_list[i].zone)
  {
    if (strcmp(zone, state_plane_lcc_nad27_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD27, from_geokeys);
      set_lambert_conformal_conic_projection(state_plane_lcc_nad27_list[i].falseEasting, state_plane_lcc_nad27_list[i].falseNorthing, state_plane_lcc_nad27_list[i].latOriginDegree, state_plane_lcc_nad27_list[i].longMeridianDegree, state_plane_lcc_nad27_list[i].firstStdParallelDegree, state_plane_lcc_nad27_list[i].secondStdParallelDegree, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane27 %s", state_plane_lcc_nad27_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_state_plane_nad83_lcc(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_lcc_nad83_list[i].zone)
  {
    if (strcmp(zone, state_plane_lcc_nad83_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD83, from_geokeys);
      set_lambert_conformal_conic_projection(state_plane_lcc_nad83_list[i].falseEasting, state_plane_lcc_nad83_list[i].falseNorthing, state_plane_lcc_nad83_list[i].latOriginDegree, state_plane_lcc_nad83_list[i].longMeridianDegree, state_plane_lcc_nad83_list[i].firstStdParallelDegree, state_plane_lcc_nad83_list[i].secondStdParallelDegree, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane83 %s", state_plane_lcc_nad83_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_state_plane_nad27_tm(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_tm_nad27_list[i].zone)
  {
    if (strcmp(zone, state_plane_tm_nad27_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD27, from_geokeys);
      set_transverse_mercator_projection(state_plane_tm_nad27_list[i].falseEasting, state_plane_tm_nad27_list[i].falseNorthing, state_plane_tm_nad27_list[i].latOriginDegree, state_plane_tm_nad27_list[i].longMeridianDegree, state_plane_tm_nad27_list[i].scaleFactor, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane27 %s", state_plane_tm_nad27_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_state_plane_nad83_tm(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_tm_nad83_list[i].zone)
  {
    if (strcmp(zone, state_plane_tm_nad83_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD83, from_geokeys);
      set_transverse_mercator_projection(state_plane_tm_nad83_list[i].falseEasting, state_plane_tm_nad83_list[i].falseNorthing, state_plane_tm_nad83_list[i].latOriginDegree, state_plane_tm_nad83_list[i].longMeridianDegree, state_plane_tm_nad83_list[i].scaleFactor, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane83 %s", state_plane_tm_nad83_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_vertical_from_VerticalCSTypeGeoKey(U16 value)
{
  if ((5000 <= value) && (value <= 5099))      // [5000, 5099] = EPSG Ellipsoid Vertical CS Codes
  {
    vertical_epsg[1] = value;
  }
  else if ((5101 <= value) && (value <= 5199)) // [5100, 5199] = EPSG Orthometric Vertical CS Codes
  {
    vertical_epsg[1] = value;
  }
  else if ((5200 <= value) && (value <= 5999)) // [5200, 5999] = Reserved EPSG
  {
    vertical_epsg[1] = value;
  }
  else
  {
//    fprintf(stderr, "set_VerticalCSTypeGeoKey: look-up for %d not implemented\012", value);
    return FALSE;
  }
  return TRUE;
}

BOOL CRScheck::set_coordinates_from_ProjLinearUnitsGeoKey(U16 value)
{
  switch (value)
  {
  case 9001: // Linear_Meter
    set_coordinates_in_meter(TRUE);
    break;
  case 9002: // Linear_Foot
    set_coordinates_in_feet(TRUE);
    break;
  case 9003: // Linear_Foot_US_Survey
    set_coordinates_in_survey_feet(TRUE);
    break;
  default:
//    fprintf(stderr, "set_VerticalUnitsGeoKey: look-up for %d not implemented\n", value);
    return FALSE;
  }
  return TRUE;
}

BOOL CRScheck::set_elevation_from_VerticalUnitsGeoKey(U16 value)
{
  switch (value)
  {
  case 9001: // Linear_Meter
    set_elevation_in_meter(TRUE);
    break;
  case 9002: // Linear_Foot
    set_elevation_in_feet(TRUE);
    break;
  case 9003: // Linear_Foot_US_Survey
    set_elevation_in_survey_feet(TRUE);
    break;
  default:
//    fprintf(stderr, "set_VerticalUnitsGeoKey: look-up for %d not implemented\n", value);
    return FALSE;
  }
  return TRUE;
}

static const short EPSG_ETRS89_Poland_CS92 = 2180;
static const short EPSG_NZGD2000 = 2193;
static const short EPSG_NAD83_HARN_UTM2_South_American_Samoa = 2195;
static const short EPSG_NAD83_HARN_Virginia_North_ftUS = 2924;
static const short EPSG_NAD83_HARN_Virginia_South_ftUS = 2925;
static const short EPSG_ETRS89_ETRS_LCC = 3034;
static const short EPSG_ETRS89_ETRS_TM34 = 3046;
static const short EPSG_ETRS89_ETRS_TM35 = 3047;
static const short EPSG_ETRS89_ETRS_TM36 = 3048;
static const short EPSG_ETRS89_ETRS_TM35FIN = 3067;
static const short EPSG_Fiji_1956_UTM60_South = 3141;
static const short EPSG_Fiji_1956_UTM1_South = 3142;
static const short EPSG_Fiji_Map_Grid_1986 = 3460;
static const short EPSG_NAD83_NSRS2007_Maryland_ftUS = 3582;
static const short EPSG_Slovene_National_Grid_1996 = 3794;
static const short EPSG_MGI_1901_Slovene_National_Grid = 3912;
static const short EPSG_RGF93_CC42 = 3942;
static const short EPSG_RGF93_CC43 = 3943;
static const short EPSG_RGF93_CC44 = 3944;
static const short EPSG_RGF93_CC45 = 3945;
static const short EPSG_RGF93_CC46 = 3946;
static const short EPSG_RGF93_CC47 = 3947;
static const short EPSG_RGF93_CC48 = 3948;
static const short EPSG_RGF93_CC49 = 3949;
static const short EPSG_RGF93_CC50 = 3950;
static const short EPSG_ETRS89_DKTM1 = 4093;
static const short EPSG_ETRS89_DKTM2 = 4094;
static const short EPSG_ETRS89_DKTM3 = 4095;
static const short EPSG_ETRS89_DKTM4 = 4096;
static const short EPSG_ETRS89_UTM32_north_zE_N = 4647;
static const short EPSG_ETRS89_NTM_zone_5 = 5105;
static const short EPSG_ETRS89_NTM_zone_6 = 5106;
static const short EPSG_ETRS89_NTM_zone_7 = 5107;
static const short EPSG_ETRS89_NTM_zone_8 = 5108;
static const short EPSG_ETRS89_NTM_zone_9 = 5109;
static const short EPSG_ETRS89_NTM_zone_10 = 5110;
static const short EPSG_ETRS89_NTM_zone_11 = 5111;
static const short EPSG_ETRS89_NTM_zone_12 = 5112;
static const short EPSG_ETRS89_NTM_zone_13 = 5113;
static const short EPSG_ETRS89_NTM_zone_14 = 5114;
static const short EPSG_ETRS89_NTM_zone_15 = 5115;
static const short EPSG_ETRS89_NTM_zone_16 = 5116;
static const short EPSG_ETRS89_NTM_zone_17 = 5117;
static const short EPSG_ETRS89_NTM_zone_18 = 5118;
static const short EPSG_ETRS89_NTM_zone_19 = 5119;
static const short EPSG_ETRS89_NTM_zone_20 = 5120;
static const short EPSG_ETRS89_NTM_zone_21 = 5121;
static const short EPSG_ETRS89_NTM_zone_22 = 5122;
static const short EPSG_ETRS89_NTM_zone_23 = 5123;
static const short EPSG_ETRS89_NTM_zone_24 = 5124;
static const short EPSG_ETRS89_NTM_zone_25 = 5125;
static const short EPSG_ETRS89_NTM_zone_26 = 5126;
static const short EPSG_ETRS89_NTM_zone_27 = 5127;
static const short EPSG_ETRS89_NTM_zone_28 = 5128;
static const short EPSG_ETRS89_NTM_zone_29 = 5129;
static const short EPSG_ETRS89_NTM_zone_30 = 5130;
static const short EPSG_ETRS89_UTM33_north_zE_N = 5650;
static const short EPSG_OSGB_1936 = 27700;
static const short EPSG_Belgian_Lambert_1972 = 31370;

BOOL CRScheck::set_projection_from_ProjectedCSTypeGeoKey(const U16 value, CHAR* description)
{
  I32 ellipsoid_id = -1;
  BOOL utm_northern = FALSE;
  I32 utm_zone = -1;
  CHAR* sp = 0;
  BOOL sp_nad27 = FALSE;

  switch (value)
  {
  case 3154: // NAD83(CSRS) / UTM zone 7N
  case 3155: // NAD83(CSRS) / UTM zone 8N
  case 3156: // NAD83(CSRS) / UTM zone 9N
  case 3157: // NAD83(CSRS) / UTM zone 10N
    utm_northern = TRUE; utm_zone = value - 3154 + 7;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 3158: // NAD83(CSRS) / UTM zone 14N
  case 3159: // NAD83(CSRS) / UTM zone 15N
  case 3160: // NAD83(CSRS) / UTM zone 16N
    utm_northern = TRUE; utm_zone = value - 3158 + 14;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 20137: // PCS_Adindan_UTM_zone_37N
  case 20138: // PCS_Adindan_UTM_zone_38N
    utm_northern = TRUE; utm_zone = value-20100;
    break;
  case 20437: // PCS_Ain_el_Abd_UTM_zone_37N
  case 20438: // PCS_Ain_el_Abd_UTM_zone_38N
  case 20439: // PCS_Ain_el_Abd_UTM_zone_39N
    utm_northern = TRUE; utm_zone = value-20400;
    break;                
  case 20538: // PCS_Afgooye_UTM_zone_38N
  case 20539: // PCS_Afgooye_UTM_zone_39N
    utm_northern = TRUE; utm_zone = value-20500;
    break;
  case 20822: // PCS_Aratu_UTM_zone_22S
  case 20823: // PCS_Aratu_UTM_zone_23S
  case 20824: // PCS_Aratu_UTM_zone_24S
    utm_northern = FALSE; utm_zone = value-20800;
    break;
  case 21148: // PCS_Batavia_UTM_zone_48S
  case 21149: // PCS_Batavia_UTM_zone_49S
  case 21150: // PCS_Batavia_UTM_zone_50S
    utm_northern = FALSE; utm_zone = value-21100;
    break;
  case 21817: // PCS_Bogota_UTM_zone_17N
  case 21818: // PCS_Bogota_UTM_zone_18N
    utm_northern = TRUE; utm_zone = value-21800;
    break;
  case 22032: // PCS_Camacupa_UTM_32S
  case 22033: // PCS_Camacupa_UTM_33S
    utm_northern = FALSE; utm_zone = value-22000;
    break; 
  case 22332: // PCS_Carthage_UTM_zone_32N
    utm_northern = TRUE; utm_zone = 32;
    break; 
  case 22523: // PCS_Corrego_Alegre_UTM_23S
  case 22524: // PCS_Corrego_Alegre_UTM_24S
    utm_northern = FALSE; utm_zone = value-22500;
    break;
  case 22832: // PCS_Douala_UTM_zone_32N
    utm_northern = TRUE; utm_zone = 32;
    break;
  case 23028: // PCS_ED50_UTM_zone_28N
  case 23029: // PCS_ED50_UTM_zone_29N
  case 23030: 
  case 23031: 
  case 23032: 
  case 23033: 
  case 23034: 
  case 23035: 
  case 23036: 
  case 23037: 
  case 23038: // PCS_ED50_UTM_zone_38N
    utm_northern = TRUE; utm_zone = value-23000;
    ellipsoid_id = CRS_ELLIPSOID_Inter;
    break;
  case 23239: // PCS_Fahud_UTM_zone_39N
  case 23240: // PCS_Fahud_UTM_zone_40N
    utm_northern = TRUE; utm_zone = value-23200;
    break;
  case 23433: // PCS_Garoua_UTM_zone_33N
    utm_northern = TRUE; utm_zone = 33;
    break;
  case 23846: // PCS_ID74_UTM_zone_46N
  case 23847: // PCS_ID74_UTM_zone_47N
  case 23848:
  case 23849:
  case 23850:
  case 23851:
  case 23852:
  case 23853: // PCS_ID74_UTM_zone_53N
    utm_northern = TRUE; utm_zone = value-23800;
    ellipsoid_id = CRS_ELLIPSOID_ID74;
    break;
  case 23886: // PCS_ID74_UTM_zone_46S
  case 23887: // PCS_ID74_UTM_zone_47S
  case 23888:
  case 23889:
  case 23890:
  case 23891:
  case 23892:
  case 23893:
  case 23894: // PCS_ID74_UTM_zone_54S
    utm_northern = FALSE; utm_zone = value-23840;
    ellipsoid_id = CRS_ELLIPSOID_ID74;
    break;
  case 23947: // PCS_Indian_1954_UTM_47N
  case 23948: // PCS_Indian_1954_UTM_48N
    utm_northern = TRUE; utm_zone = value-23900;
    break;
  case 24047: // PCS_Indian_1975_UTM_47N
  case 24048: // PCS_Indian_1975_UTM_48N
    utm_northern = TRUE; utm_zone = value-24000;
    break;
  case 24547: // PCS_Kertau_UTM_zone_47N
  case 24548: // PCS_Kertau_UTM_zone_48N
    utm_northern = TRUE; utm_zone = value-24500;
    break;
  case 24720: // PCS_La_Canoa_UTM_zone_20N
  case 24721: // PCS_La_Canoa_UTM_zone_21N
    utm_northern = TRUE; utm_zone = value-24700;
    break;
  case 24818: // PCS_PSAD56_UTM_zone_18N
  case 24819: // PCS_PSAD56_UTM_zone_19N
  case 24820: // PCS_PSAD56_UTM_zone_20N
  case 24821: // PCS_PSAD56_UTM_zone_21N
    utm_northern = TRUE; utm_zone = value-24800;
    break;
  case 24877: // PCS_PSAD56_UTM_zone_17S
  case 24878: // PCS_PSAD56_UTM_zone_18S
  case 24879: // PCS_PSAD56_UTM_zone_19S
  case 24880: // PCS_PSAD56_UTM_zone_20S
    utm_northern = FALSE; utm_zone = value-24860;
    break;
  case 25231: // PCS_Lome_UTM_zone_31N
    utm_northern = TRUE; utm_zone = 31;
    break;
  case 25828: // PCS_ETRS89_UTM_zone_28N
  case 25829: // PCS_ETRS89_UTM_zone_29N
  case 25830: // PCS_ETRS89_UTM_zone_30N
  case 25831: // PCS_ETRS89_UTM_zone_31N
  case 25832: // PCS_ETRS89_UTM_zone_32N
  case 25833: // PCS_ETRS89_UTM_zone_33N
  case 25834: // PCS_ETRS89_UTM_zone_34N
  case 25835: // PCS_ETRS89_UTM_zone_35N
  case 25836: // PCS_ETRS89_UTM_zone_36N
  case 25837: // PCS_ETRS89_UTM_zone_37N
  case 25838: // PCS_ETRS89_UTM_zone_38N
    utm_northern = TRUE; utm_zone = value-25800;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 25932: // PCS_Malongo_1987_UTM_32S
    utm_northern = FALSE; utm_zone = 32;
    break;
  case 26237: // PCS_Massawa_UTM_zone_37N
    utm_northern = TRUE; utm_zone = 37;
    break;
  case 26331: // PCS_Minna_UTM_zone_31N
  case 26332: // PCS_Minna_UTM_zone_32N
    utm_northern = TRUE; utm_zone = value-26300;
    break;
  case 26432: // PCS_Mhast_UTM_zone_32S
    utm_northern = FALSE; utm_zone = 32;
    break;
  case 26632: // PCS_M_poraloko_UTM_32N
    utm_northern = TRUE; utm_zone = 32;
    break;
  case 26692: // PCS_Minna_UTM_zone_32S
    utm_northern = FALSE; utm_zone = 32;
    break;
  case 26703: // PCS_NAD27_UTM_zone_3N
  case 26704: // PCS_NAD27_UTM_zone_4N
  case 26705: // PCS_NAD27_UTM_zone_5N
  case 26706: // PCS_NAD27_UTM_zone_6N
  case 26707: // PCS_NAD27_UTM_zone_7N
  case 26708: // PCS_NAD27_UTM_zone_7N
  case 26709: // PCS_NAD27_UTM_zone_9N
  case 26710: // PCS_NAD27_UTM_zone_10N
  case 26711: // PCS_NAD27_UTM_zone_11N
  case 26712: // PCS_NAD27_UTM_zone_12N
  case 26713: // PCS_NAD27_UTM_zone_13N
  case 26714: // PCS_NAD27_UTM_zone_14N
  case 26715: // PCS_NAD27_UTM_zone_15N
  case 26716: // PCS_NAD27_UTM_zone_16N
  case 26717: // PCS_NAD27_UTM_zone_17N
  case 26718: // PCS_NAD27_UTM_zone_18N
  case 26719: // PCS_NAD27_UTM_zone_19N
  case 26720: // PCS_NAD27_UTM_zone_20N
  case 26721: // PCS_NAD27_UTM_zone_21N
  case 26722: // PCS_NAD27_UTM_zone_22N
    utm_northern = TRUE; utm_zone = value-26700;
    ellipsoid_id = CRS_ELLIPSOID_NAD27;
    break;
  case 26729: // PCS_NAD27_Alabama_East
    sp_nad27 = TRUE; sp = "AL_E";
    break;
  case 26730: // PCS_NAD27_Alabama_West
    sp_nad27 = TRUE; sp = "AL_W";
    break;
  case 26731: // PCS_NAD27_Alaska_zone_1
    sp_nad27 = TRUE; sp = "AK_1";
    break;
  case 26732: // PCS_NAD27_Alaska_zone_2
    sp_nad27 = TRUE; sp = "AK_2";
    break;
  case 26733: // PCS_NAD27_Alaska_zone_3
    sp_nad27 = TRUE; sp = "AK_3";
    break;
  case 26734: // PCS_NAD27_Alaska_zone_4
    sp_nad27 = TRUE; sp = "AK_4";
    break;
  case 26735: // PCS_NAD27_Alaska_zone_5
    sp_nad27 = TRUE; sp = "AK_5";
    break;
  case 26736: // PCS_NAD27_Alaska_zone_6
    sp_nad27 = TRUE; sp = "AK_6";
    break;
  case 26737: // PCS_NAD27_Alaska_zone_7
    sp_nad27 = TRUE; sp = "AK_7";
    break;
  case 26738: // PCS_NAD27_Alaska_zone_8
    sp_nad27 = TRUE; sp = "AK_8";
    break;
  case 26739: // PCS_NAD27_Alaska_zone_9
    sp_nad27 = TRUE; sp = "AK_9";
    break;
  case 26740: // PCS_NAD27_Alaska_zone_10
    sp_nad27 = TRUE; sp = "AK_10";
    break;
  case 26741: // PCS_NAD27_California_I
    sp_nad27 = TRUE; sp = "CA_I";
    break;
  case 26742: // PCS_NAD27_California_II
    sp_nad27 = TRUE; sp = "CA_II";
    break;
  case 26743: // PCS_NAD27_California_III
    sp_nad27 = TRUE; sp = "CA_III";
    break;
  case 26744: // PCS_NAD27_California_IV
    sp_nad27 = TRUE; sp = "CA_IV";
    break;
  case 26745: // PCS_NAD27_California_V
    sp_nad27 = TRUE; sp = "CA_V";
    break;
  case 26746: // PCS_NAD27_California_VI
    sp_nad27 = TRUE; sp = "CA_VI";
    break;
  case 26747: // PCS_NAD27_California_VII
    sp_nad27 = TRUE; sp = "CA_VII";
    break;
  case 26748: // PCS_NAD27_Arizona_East
    sp_nad27 = TRUE; sp = "AZ_E";
    break;
  case 26749: // PCS_NAD27_Arizona_Central
    sp_nad27 = TRUE; sp = "AZ_C";
    break;
  case 26750: // PCS_NAD27_Arizona_West
    sp_nad27 = TRUE; sp = "AZ_W";
    break;
  case 26751: // PCS_NAD27_Arkansas_North
    sp_nad27 = TRUE; sp = "AR_N";
    break;
  case 26752: // PCS_NAD27_Arkansas_South
    sp_nad27 = TRUE; sp = "AR_S";
    break;
  case 26753: // PCS_NAD27_Colorado_North
    sp_nad27 = TRUE; sp = "CO_N";
    break;
  case 26754: // PCS_NAD27_Colorado_Central
    sp_nad27 = TRUE; sp = "CO_C";
    break;
  case 26755: // PCS_NAD27_Colorado_South
    sp_nad27 = TRUE; sp = "CO_S";
    break;
  case 26756: // PCS_NAD27_Connecticut
    sp_nad27 = TRUE; sp = "CT";
    break;
  case 26757: // PCS_NAD27_Delaware
    sp_nad27 = TRUE; sp = "DE";
    break;
  case 26758: // PCS_NAD27_Florida_East
    sp_nad27 = TRUE; sp = "FL_E";
    break;
  case 26759: // PCS_NAD27_Florida_West
    sp_nad27 = TRUE; sp = "FL_W";
    break;
  case 26760: // PCS_NAD27_Florida_North
    sp_nad27 = TRUE; sp = "FL_N";
    break;
  case 26761: // PCS_NAD27_Hawaii_zone_1
    sp_nad27 = TRUE; sp = "HI_1";
    break;
  case 26762: // PCS_NAD27_Hawaii_zone_2
    sp_nad27 = TRUE; sp = "HI_2";
    break;
  case 26763: // PCS_NAD27_Hawaii_zone_3
    sp_nad27 = TRUE; sp = "HI_3";
    break;
  case 26764: // PCS_NAD27_Hawaii_zone_4
    sp_nad27 = TRUE; sp = "HI_4";
    break;
  case 26765: // PCS_NAD27_Hawaii_zone_5
    sp_nad27 = TRUE; sp = "HI_5";
    break;
  case 26766: // PCS_NAD27_Georgia_East
    sp_nad27 = TRUE; sp = "GA_E";
    break;
  case 26767: // PCS_NAD27_Georgia_West
    sp_nad27 = TRUE; sp = "GA_W";
    break;
  case 26768: // PCS_NAD27_Idaho_East
    sp_nad27 = TRUE; sp = "ID_E";
    break;
  case 26769: // PCS_NAD27_Idaho_Central
    sp_nad27 = TRUE; sp = "ID_C";
    break;
  case 26770: // PCS_NAD27_Idaho_West
    sp_nad27 = TRUE; sp = "ID_W";
    break;
  case 26771: // PCS_NAD27_Illinois_East
    sp_nad27 = TRUE; sp = "IL_E";
    break;
  case 26772: // PCS_NAD27_Illinois_West
    sp_nad27 = TRUE; sp = "IL_W";
    break;
  case 26773: // PCS_NAD27_Indiana_East
    sp_nad27 = TRUE; sp = "IN_E";
    break;
  case 26774: // PCS_NAD27_Indiana_West
    sp_nad27 = TRUE; sp = "IN_W";
    break;
  case 26775: // PCS_NAD27_Iowa_North
    sp_nad27 = TRUE; sp = "IA_N";
    break;
  case 26776: // PCS_NAD27_Iowa_South
    sp_nad27 = TRUE; sp = "IA_S";
    break;
  case 26777: // PCS_NAD27_Kansas_North
    sp_nad27 = TRUE; sp = "KS_N";
    break;
  case 26778: // PCS_NAD27_Kansas_South
    sp_nad27 = TRUE; sp = "KS_S";
    break;
  case 26779: // PCS_NAD27_Kentucky_North
    sp_nad27 = TRUE; sp = "KY_N";
    break;
  case 26780: // PCS_NAD27_Kentucky_South
    sp_nad27 = TRUE; sp = "KY_S";
    break;
  case 26781: // PCS_NAD27_Louisiana_North
    sp_nad27 = TRUE; sp = "LA_N";
    break;
  case 26782: // PCS_NAD27_Louisiana_South
    sp_nad27 = TRUE; sp = "LA_S";
    break;
  case 26783: // PCS_NAD27_Maine_East
    sp_nad27 = TRUE; sp = "ME_E";
    break;
  case 26784: // PCS_NAD27_Maine_West
    sp_nad27 = TRUE; sp = "ME_W";
    break;
  case 26785: // PCS_NAD27_Maryland
    sp_nad27 = TRUE; sp = "MD";
    break;
  case 26786: // PCS_NAD27_Massachusetts
    sp_nad27 = TRUE; sp = "M_M";
    break;
  case 26787: // PCS_NAD27_Massachusetts_Is
    sp_nad27 = TRUE; sp = "M_I";
    break;
  case 26788: // PCS_NAD27_Michigan_North
    sp_nad27 = TRUE; sp = "MI_N";
    break;
  case 26789: // PCS_NAD27_Michigan_Central
    sp_nad27 = TRUE; sp = "MI_C";
    break;
  case 26790: // PCS_NAD27_Michigan_South
    sp_nad27 = TRUE; sp = "MI_S";
    break;
  case 26791: // PCS_NAD27_Minnesota_North
    sp_nad27 = TRUE; sp = "MN_N";
    break;
  case 26792: // PCS_NAD27_Minnesota_Cent
    sp_nad27 = TRUE; sp = "MN_C";
    break;
  case 26793: // PCS_NAD27_Minnesota_South
    sp_nad27 = TRUE; sp = "MN_S";
    break;
  case 26794: // PCS_NAD27_Mississippi_East
    sp_nad27 = TRUE; sp = "MS_E";
    break;
  case 26795: // PCS_NAD27_Mississippi_West
    sp_nad27 = TRUE; sp = "MS_W";
    break;
  case 26796: // PCS_NAD27_Missouri_East
    sp_nad27 = TRUE; sp = "MO_E";
    break;
  case 26797: // PCS_NAD27_Missouri_Central
    sp_nad27 = TRUE; sp = "MO_C";
    break;
  case 26798: // PCS_NAD27_Missouri_West
    sp_nad27 = TRUE; sp = "MO_W";
    break;
  case 26903: // PCS_NAD83_UTM_zone_3N
  case 26904: // PCS_NAD83_UTM_zone_4N
  case 26905: // PCS_NAD83_UTM_zone_5N
  case 26906: // PCS_NAD83_UTM_zone_6N
  case 26907: // PCS_NAD83_UTM_zone_7N
  case 26908: // PCS_NAD83_UTM_zone_7N
  case 26909: // PCS_NAD83_UTM_zone_9N
  case 26910: // PCS_NAD83_UTM_zone_10N
  case 26911: // PCS_NAD83_UTM_zone_11N
  case 26912: // PCS_NAD83_UTM_zone_12N
  case 26913: // PCS_NAD83_UTM_zone_13N
  case 26914: // PCS_NAD83_UTM_zone_14N
  case 26915: // PCS_NAD83_UTM_zone_15N
  case 26916: // PCS_NAD83_UTM_zone_16N
  case 26917: // PCS_NAD83_UTM_zone_17N
  case 26918: // PCS_NAD83_UTM_zone_18N
  case 26919: // PCS_NAD83_UTM_zone_19N
  case 26920: // PCS_NAD83_UTM_zone_20N
  case 26921: // PCS_NAD83_UTM_zone_21N
  case 26922: // PCS_NAD83_UTM_zone_22N
  case 26923: // PCS_NAD83_UTM_zone_23N
    utm_northern = TRUE; utm_zone = value-26900;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 26929: // PCS_NAD83_Alabama_East
    sp_nad27 = FALSE; sp = "AL_E";
    break;
  case 26930: // PCS_NAD83_Alabama_West
    sp_nad27 = FALSE; sp = "AL_W";
    break;
  case 26931: // PCS_NAD83_Alaska_zone_1
    sp_nad27 = FALSE; sp = "AK_1";
    break;
  case 26932: // PCS_NAD83_Alaska_zone_2
    sp_nad27 = FALSE; sp = "AK_2";
    break;
  case 26933: // PCS_NAD83_Alaska_zone_3
    sp_nad27 = FALSE; sp = "AK_3";
    break;
  case 26934: // PCS_NAD83_Alaska_zone_4
    sp_nad27 = FALSE; sp = "AK_4";
    break;
  case 26935: // PCS_NAD83_Alaska_zone_5
    sp_nad27 = FALSE; sp = "AK_5";
    break;
  case 26936: // PCS_NAD83_Alaska_zone_6
    sp_nad27 = FALSE; sp = "AK_6";
    break;
  case 26937: // PCS_NAD83_Alaska_zone_7
    sp_nad27 = FALSE; sp = "AK_7";
    break;
  case 26938: // PCS_NAD83_Alaska_zone_8
    sp_nad27 = FALSE; sp = "AK_8";
    break;
  case 26939: // PCS_NAD83_Alaska_zone_9
    sp_nad27 = FALSE; sp = "AK_9";
    break;
  case 26940: // PCS_NAD83_Alaska_zone_10
    sp_nad27 = FALSE; sp = "AK_10";
    break;
  case 26941: // PCS_NAD83_California_I
    sp_nad27 = FALSE; sp = "CA_I";
    break;
  case 26942: // PCS_NAD83_California_II
    sp_nad27 = FALSE; sp = "CA_II";
    break;
  case 26943: // PCS_NAD83_California_III
    sp_nad27 = FALSE; sp = "CA_III";
    break;
  case 26944: // PCS_NAD83_California_IV
    sp_nad27 = FALSE; sp = "CA_IV";
    break;
  case 26945: // PCS_NAD83_California_V
    sp_nad27 = FALSE; sp = "CA_V";
    break;
  case 26946: // PCS_NAD83_California_VI
    sp_nad27 = FALSE; sp = "CA_VI";
    break;
  case 26947: // PCS_NAD83_California_VII
    sp_nad27 = FALSE; sp = "CA_VII";
    break;
  case 26948: // PCS_NAD83_Arizona_East
    sp_nad27 = FALSE; sp = "AZ_E";
    break;
  case 26949: // PCS_NAD83_Arizona_Central
    sp_nad27 = FALSE; sp = "AZ_C";
    break;
  case 26950: // PCS_NAD83_Arizona_West
    sp_nad27 = FALSE; sp = "AZ_W";
    break;
  case 26951: // PCS_NAD83_Arkansas_North
    sp_nad27 = FALSE; sp = "AR_N";
    break;
  case 26952: // PCS_NAD83_Arkansas_South
    sp_nad27 = FALSE; sp = "AR_S";
    break;
  case 26953: // PCS_NAD83_Colorado_North
    sp_nad27 = FALSE; sp = "CO_N";
    break;
  case 26954: // PCS_NAD83_Colorado_Central
    sp_nad27 = FALSE; sp = "CO_C";
    break;
  case 26955: // PCS_NAD83_Colorado_South
    sp_nad27 = FALSE; sp = "CO_S";
    break;
  case 26956: // PCS_NAD83_Connecticut
    sp_nad27 = FALSE; sp = "CT";
    break;
  case 26957: // PCS_NAD83_Delaware
    sp_nad27 = FALSE; sp = "DE";
    break;
  case 26958: // PCS_NAD83_Florida_East
    sp_nad27 = FALSE; sp = "FL_E";
    break;
  case 26959: // PCS_NAD83_Florida_West
    sp_nad27 = FALSE; sp = "FL_W";
    break;
  case 26960: // PCS_NAD83_Florida_North
    sp_nad27 = FALSE; sp = "FL_N";
    break;
  case 26961: // PCS_NAD83_Hawaii_zone_1
    sp_nad27 = FALSE; sp = "HI_1";
    break;
  case 26962: // PCS_NAD83_Hawaii_zone_2
    sp_nad27 = FALSE; sp = "HI_2";
    break;
  case 26963: // PCS_NAD83_Hawaii_zone_3
    sp_nad27 = FALSE; sp = "HI_3";
    break;
  case 26964: // PCS_NAD83_Hawaii_zone_4
    sp_nad27 = FALSE; sp = "HI_4";
    break;
  case 26965: // PCS_NAD83_Hawaii_zone_5
    sp_nad27 = FALSE; sp = "HI_5";
    break;
  case 26966: // PCS_NAD83_Georgia_East
    sp_nad27 = FALSE; sp = "GA_E";
    break;
  case 26967: // PCS_NAD83_Georgia_West
    sp_nad27 = FALSE; sp = "GA_W";
    break;
  case 26968: // PCS_NAD83_Idaho_East
    sp_nad27 = FALSE; sp = "ID_E";
    break;
  case 26969: // PCS_NAD83_Idaho_Central
    sp_nad27 = FALSE; sp = "ID_C";
    break;
  case 26970: // PCS_NAD83_Idaho_West
    sp_nad27 = FALSE; sp = "ID_W";
    break;
  case 26971: // PCS_NAD83_Illinois_East
    sp_nad27 = FALSE; sp = "IL_E";
    break;
  case 26972: // PCS_NAD83_Illinois_West
    sp_nad27 = FALSE; sp = "IL_W";
    break;
  case 26973: // PCS_NAD83_Indiana_East
    sp_nad27 = FALSE; sp = "IN_E";
    break;
  case 26974: // PCS_NAD83_Indiana_West
    sp_nad27 = FALSE; sp = "IN_W";
    break;
  case 26975: // PCS_NAD83_Iowa_North
    sp_nad27 = FALSE; sp = "IA_N";
    break;
  case 26976: // PCS_NAD83_Iowa_South
    sp_nad27 = FALSE; sp = "IA_S";
    break;
  case 26977: // PCS_NAD83_Kansas_North
    sp_nad27 = FALSE; sp = "KS_N";
    break;
  case 26978: // PCS_NAD83_Kansas_South
    sp_nad27 = FALSE; sp = "KS_S";
    break;
  case 26979: // PCS_NAD83_Kentucky_North
    sp_nad27 = FALSE; sp = "KY_N";
    break;
  case 26980: // PCS_NAD83_Kentucky_South
    sp_nad27 = FALSE; sp = "KY_S";
    break;
  case 26981: // PCS_NAD83_Louisiana_North
    sp_nad27 = FALSE; sp = "LA_N";
    break;
  case 26982: // PCS_NAD83_Louisiana_South
    sp_nad27 = FALSE; sp = "LA_S";
    break;
  case 26983: // PCS_NAD83_Maine_East
    sp_nad27 = FALSE; sp = "ME_E";
    break;
  case 26984: // PCS_NAD83_Maine_West
    sp_nad27 = FALSE; sp = "ME_W";
    break;
  case 26985: // PCS_NAD83_Maryland
    sp_nad27 = FALSE; sp = "MD";
    break;
  case 26986: // PCS_NAD83_Massachusetts
    sp_nad27 = FALSE; sp = "M_M";
    break;
  case 26987: // PCS_NAD83_Massachusetts_Is
    sp_nad27 = FALSE; sp = "M_I";
    break;
  case 26988: // PCS_NAD83_Michigan_North
    sp_nad27 = FALSE; sp = "MI_N";
    break;
  case 26989: // PCS_NAD83_Michigan_Central
    sp_nad27 = FALSE; sp = "MI_C";
    break;
  case 26990: // PCS_NAD83_Michigan_South
    sp_nad27 = FALSE; sp = "MI_S";
    break;
  case 26991: // PCS_NAD83_Minnesota_North
    sp_nad27 = FALSE; sp = "MN_N";
    break;
  case 26992: // PCS_NAD83_Minnesota_Cent
    sp_nad27 = FALSE; sp = "MN_C";
    break;
  case 26993: // PCS_NAD83_Minnesota_South
    sp_nad27 = FALSE; sp = "MN_S";
    break;
  case 26994: // PCS_NAD83_Mississippi_East
    sp_nad27 = FALSE; sp = "MS_E";
    break;
  case 26995: // PCS_NAD83_Mississippi_West
    sp_nad27 = FALSE; sp = "MS_W";
    break;
  case 26996: // PCS_NAD83_Missouri_East
    sp_nad27 = FALSE; sp = "MO_E";
    break;
  case 26997: // PCS_NAD83_Missouri_Central
    sp_nad27 = FALSE; sp = "MO_C";
    break;
  case 26998: // PCS_NAD83_Missouri_West
    sp_nad27 = FALSE; sp = "MO_W";
    break;
  case 27700:

    break;
  case 28348: // PCS_GDA94_MGA_zone_48
  case 28349:
  case 28350:
  case 28351:
  case 28352:
  case 28353:
  case 28354: // PCS_GDA94_MGA_zone_54
  case 28355: // PCS_GDA94_MGA_zone_55
  case 28356: // PCS_GDA94_MGA_zone_56
  case 28357: // PCS_GDA94_MGA_zone_57
  case 28358: // PCS_GDA94_MGA_zone_58
    utm_northern = FALSE; utm_zone = value-28300;
    ellipsoid_id = CRS_ELLIPSOID_GDA94;
    break;
  case 29118: // PCS_SAD69_UTM_zone_18N
  case 29119: // PCS_SAD69_UTM_zone_19N
  case 29120: // PCS_SAD69_UTM_zone_20N
  case 29121: // PCS_SAD69_UTM_zone_21N
  case 29122: // PCS_SAD69_UTM_zone_22N
    utm_northern = TRUE; utm_zone = value-29100;
    ellipsoid_id = CRS_ELLIPSOID_SAD69;
    break;
  case 29177: // PCS_SAD69_UTM_zone_17S
  case 29178: // PCS_SAD69_UTM_zone_18S
  case 29179: // PCS_SAD69_UTM_zone_19S
  case 29180: // PCS_SAD69_UTM_zone_20S
  case 29181: // PCS_SAD69_UTM_zone_21S
  case 29182: // PCS_SAD69_UTM_zone_22S
  case 29183: // PCS_SAD69_UTM_zone_23S
  case 29184: // PCS_SAD69_UTM_zone_24S
  case 29185: // PCS_SAD69_UTM_zone_25S
    utm_northern = FALSE; utm_zone = value-29160;
    ellipsoid_id = CRS_ELLIPSOID_SAD69;
    break;
  case 29220: // PCS_Sapper_Hill_UTM_20S
  case 29221: // PCS_Sapper_Hill_UTM_21S
    utm_northern = FALSE; utm_zone = value-29200;
    break;
  case 29333: // PCS_Schwarzeck_UTM_33S
    utm_northern = FALSE; utm_zone = 33;
    break;
  case 29635: // PCS_Sudan_UTM_zone_35N
  case 29636: // PCS_Sudan_UTM_zone_35N
    utm_northern = TRUE; utm_zone = value-29600;
    break;
  case 29738: // PCS_Tananarive_UTM_38S
  case 29739: // PCS_Tananarive_UTM_39S
    utm_northern = FALSE; utm_zone = value-29700;
    break;
  case 29849: // PCS_Timbalai_1948_UTM_49N
  case 29850: // PCS_Timbalai_1948_UTM_50N
    utm_northern = TRUE; utm_zone = value-29800;
    break;
  case 30339: // PCS_TC_1948_UTM_zone_39N
  case 30340: // PCS_TC_1948_UTM_zone_40N
    utm_northern = TRUE; utm_zone = value-30300;
    break;
  case 30729: // PCS_Nord_Sahara_UTM_29N
  case 30730: // PCS_Nord_Sahara_UTM_30N
  case 30731: // PCS_Nord_Sahara_UTM_31N
  case 30732: // PCS_Nord_Sahara_UTM_32N
    utm_northern = TRUE; utm_zone = value-30700;
    break;
  case 31028: // PCS_Yoff_UTM_zone_28N
    utm_northern = TRUE; utm_zone = 28;
    break;
  case 31121: // PCS_Zanderij_UTM_zone_21N
    utm_northern = TRUE; utm_zone = 21;
    break;
  case 32001: // PCS_NAD27_Montana_North
    sp_nad27 = TRUE; sp = "MT_N";
    break;
  case 32002: // PCS_NAD27_Montana_Central
    sp_nad27 = TRUE; sp = "MT_C";
    break;
  case 32003: // PCS_NAD27_Montana_South
    sp_nad27 = TRUE; sp = "MT_S";
    break;
  case 32005: // PCS_NAD27_Nebraska_North
    sp_nad27 = TRUE; sp = "NE_N";
    break;
  case 32006: // PCS_NAD27_Nebraska_South
    sp_nad27 = TRUE; sp = "NE_S";
    break;
  case 32007: // PCS_NAD27_Nevada_East
    sp_nad27 = TRUE; sp = "NV_E";
    break;
  case 32008: // PCS_NAD27_Nevada_Central
    sp_nad27 = TRUE; sp = "NV_C";
    break;
  case 32009: // PCS_NAD27_Nevada_West
    sp_nad27 = TRUE; sp = "NV_W";
    break;
  case 32010: // PCS_NAD27_New_Hampshire
    sp_nad27 = TRUE; sp = "NH";
    break;
  case 32011: // PCS_NAD27_New_Jersey
    sp_nad27 = TRUE; sp = "NJ";
    break;
  case 32012: // PCS_NAD27_New_Mexico_East
    sp_nad27 = TRUE; sp = "NM_E";
    break;
  case 32013: // PCS_NAD27_New_Mexico_Cent
    sp_nad27 = TRUE; sp = "NM_C";
    break;
  case 32014: // PCS_NAD27_New_Mexico_West
    sp_nad27 = TRUE; sp = "NM_W";
    break;
  case 32015: // PCS_NAD27_New_York_East
    sp_nad27 = TRUE; sp = "NY_E";
    break;
  case 32016: // PCS_NAD27_New_York_Central
    sp_nad27 = TRUE; sp = "NY_C";
    break;
  case 32017: // PCS_NAD27_New_York_West
    sp_nad27 = TRUE; sp = "NY_W";
    break;
  case 32018: // PCS_NAD27_New_York_Long_Is
    sp_nad27 = TRUE; sp = "NT_LI";
    break;
  case 32019: // PCS_NAD27_North_Carolina
    sp_nad27 = TRUE; sp = "NC";
    break;
  case 32020: // PCS_NAD27_North_Dakota_N
    sp_nad27 = TRUE; sp = "ND_N";
    break;
  case 32021: // PCS_NAD27_North_Dakota_S
    sp_nad27 = TRUE; sp = "ND_S";
    break;
  case 32022: // PCS_NAD27_Ohio_North
    sp_nad27 = TRUE; sp = "OH_N";
    break;
  case 32023: // PCS_NAD27_Ohio_South
    sp_nad27 = TRUE; sp = "OH_S";
    break;
  case 32024: // PCS_NAD27_Oklahoma_North
    sp_nad27 = TRUE; sp = "OK_N";
    break;
  case 32025: // PCS_NAD27_Oklahoma_South
    sp_nad27 = TRUE; sp = "OK_S";
    break;
  case 32026: // PCS_NAD27_Oregon_North
    sp_nad27 = TRUE; sp = "OR_N";
    break;
  case 32027: // PCS_NAD27_Oregon_South
    sp_nad27 = TRUE; sp = "OR_S";
    break;
  case 32028: // PCS_NAD27_Pennsylvania_N
    sp_nad27 = TRUE; sp = "PA_N";
    break;
  case 32029: // PCS_NAD27_Pennsylvania_S
    sp_nad27 = TRUE; sp = "PA_S";
    break;
  case 32030: // PCS_NAD27_Rhode_Island
    sp_nad27 = TRUE; sp = "RI";
    break;
  case 32031: // PCS_NAD27_South_Carolina_N
    sp_nad27 = TRUE; sp = "SC_N";
    break;
  case 32033: // PCS_NAD27_South_Carolina_S
    sp_nad27 = TRUE; sp = "SC_S";
    break;
  case 32034: // PCS_NAD27_South_Dakota_N
    sp_nad27 = TRUE; sp = "SD_N";
    break;
  case 32035: // PCS_NAD27_South_Dakota_S
    sp_nad27 = TRUE; sp = "SD_S";
    break;
  case 32036: // PCS_NAD27_Tennessee
    sp_nad27 = TRUE; sp = "TN";
    break;
  case 32037: // PCS_NAD27_Texas_North
    sp_nad27 = TRUE; sp = "TX_N";
    break;
  case 32038: // PCS_NAD27_Texas_North_Cen
    sp_nad27 = TRUE; sp = "TX_NC";
    break;
  case 32039: // PCS_NAD27_Texas_Central
    sp_nad27 = TRUE; sp = "TX_C";
    break;
  case 32040: // PCS_NAD27_Texas_South_Cen
    sp_nad27 = TRUE; sp = "TX_SC";
    break;
  case 32041: // PCS_NAD27_Texas_South
    sp_nad27 = TRUE; sp = "TX_S";
    break;
  case 32042: // PCS_NAD27_Utah_North
    sp_nad27 = TRUE; sp = "UT_N";
    break;
  case 32043: // PCS_NAD27_Utah_Central
    sp_nad27 = TRUE; sp = "UT_C";
    break;
  case 32044: // PCS_NAD27_Utah_South
    sp_nad27 = TRUE; sp = "UT_S";
    break;
  case 32045: // PCS_NAD27_Vermont
    sp_nad27 = TRUE; sp = "VT";
    break;
  case 32046: // PCS_NAD27_Virginia_North
    sp_nad27 = TRUE; sp = "VA_N";
    break;
  case 32047: // PCS_NAD27_Virginia_South
    sp_nad27 = TRUE; sp = "VA_S";
    break;
  case 32048: // PCS_NAD27_Washington_North
    sp_nad27 = TRUE; sp = "WA_N";
    break;
  case 32049: // PCS_NAD27_Washington_South
    sp_nad27 = TRUE; sp = "WA_S";
    break;
  case 32050: // PCS_NAD27_West_Virginia_N
    sp_nad27 = TRUE; sp = "WV_N";
    break;
  case 32051: // PCS_NAD27_West_Virginia_S
    sp_nad27 = TRUE; sp = "WV_S";
    break;
  case 32052: // PCS_NAD27_Wisconsin_North
    sp_nad27 = TRUE; sp = "WI_N";
    break;
  case 32053: // PCS_NAD27_Wisconsin_Cen
    sp_nad27 = TRUE; sp = "WI_C";
    break;
  case 32054: // PCS_NAD27_Wisconsin_South
    sp_nad27 = TRUE; sp = "WI_S";
    break;
  case 32055: // PCS_NAD27_Wyoming_East
    sp_nad27 = TRUE; sp = "WY_E";
    break;
  case 32056: // PCS_NAD27_Wyoming_E_Cen
    sp_nad27 = TRUE; sp = "WY_EC";
    break;
  case 32057: // PCS_NAD27_Wyoming_W_Cen
    sp_nad27 = TRUE; sp = "WY_WC";
    break;
  case 32058: // PCS_NAD27_Wyoming_West
    sp_nad27 = TRUE; sp = "WY_W";
    break;
  case 32059: // PCS_NAD27_Puerto_Rico
    sp_nad27 = TRUE; sp = "PR";
    break;
  case 32060: // PCS_NAD27_St_Croix
    sp_nad27 = TRUE; sp = "St.Croix";
    break;
  case 32100: // PCS_NAD83_Montana
    sp_nad27 = FALSE; sp = "MT";
    break;
  case 32104: // PCS_NAD83_Nebraska
    sp_nad27 = FALSE; sp = "NE";
    break;
  case 32107: // PCS_NAD83_Nevada_East
    sp_nad27 = FALSE; sp = "NV_E";
    break;
  case 32108: // PCS_NAD83_Nevada_Central
    sp_nad27 = FALSE; sp = "NV_C";
    break;
  case 32109: // PCS_NAD83_Nevada_West
    sp_nad27 = FALSE; sp = "NV_W";
    break;
  case 32110: // PCS_NAD83_New_Hampshire
    sp_nad27 = FALSE; sp = "NH";
    break;
  case 32111: // PCS_NAD83_New_Jersey
    sp_nad27 = FALSE; sp = "NJ";
    break;
  case 32112: // PCS_NAD83_New_Mexico_East
    sp_nad27 = FALSE; sp = "NM_E";
    break;
  case 32113: // PCS_NAD83_New_Mexico_Cent
    sp_nad27 = FALSE; sp = "NM_C";
    break;
  case 32114: // PCS_NAD83_New_Mexico_West
    sp_nad27 = FALSE; sp = "NM_W";
    break;
  case 32115: // PCS_NAD83_New_York_East
    sp_nad27 = FALSE; sp = "NY_E";
    break;
  case 32116: // PCS_NAD83_New_York_Central
    sp_nad27 = FALSE; sp = "NY_C";
    break;
  case 32117: // PCS_NAD83_New_York_West
    sp_nad27 = FALSE; sp = "NY_W";
    break;
  case 32118: // PCS_NAD83_New_York_Long_Is
    sp_nad27 = FALSE; sp = "NT_LI";
    break;
  case 32119: // PCS_NAD83_North_Carolina
    sp_nad27 = FALSE; sp = "NC";
    break;
  case 32120: // PCS_NAD83_North_Dakota_N
    sp_nad27 = FALSE; sp = "ND_N";
    break;
  case 32121: // PCS_NAD83_North_Dakota_S
    sp_nad27 = FALSE; sp = "ND_S";
    break;
  case 32122: // PCS_NAD83_Ohio_North
    sp_nad27 = FALSE; sp = "OH_N";
    break;
  case 32123: // PCS_NAD83_Ohio_South
    sp_nad27 = FALSE; sp = "OH_S";
    break;
  case 32124: // PCS_NAD83_Oklahoma_North
    sp_nad27 = FALSE; sp = "OK_N";
    break;
  case 32125: // PCS_NAD83_Oklahoma_South
    sp_nad27 = FALSE; sp = "OK_S";
    break;
  case 32126: // PCS_NAD83_Oregon_North
    sp_nad27 = FALSE; sp = "OR_N";
    break;
  case 32127: // PCS_NAD83_Oregon_South
    sp_nad27 = FALSE; sp = "OR_S";
    break;
  case 32128: // PCS_NAD83_Pennsylvania_N
    sp_nad27 = FALSE; sp = "PA_N";
    break;
  case 32129: // PCS_NAD83_Pennsylvania_S
    sp_nad27 = FALSE; sp = "PA_S";
    break;
  case 32130: // PCS_NAD83_Rhode_Island
    sp_nad27 = FALSE; sp = "RI";
    break;
  case 32133: // PCS_NAD83_South_Carolina
    sp_nad27 = FALSE; sp = "SC";
    break;
  case 32134: // PCS_NAD83_South_Dakota_N
    sp_nad27 = FALSE; sp = "SD_N";
    break;
  case 32135: // PCS_NAD83_South_Dakota_S
    sp_nad27 = FALSE; sp = "SD_S";
    break;
  case 32136: // PCS_NAD83_Tennessee
    sp_nad27 = FALSE; sp = "TN";
    break;
  case 32137: // PCS_NAD83_Texas_North
    sp_nad27 = FALSE; sp = "TX_N";
    break;
  case 32138: // PCS_NAD83_Texas_North_Cen
    sp_nad27 = FALSE; sp = "TX_NC";
    break;
  case 32139: // PCS_NAD83_Texas_Central
    sp_nad27 = FALSE; sp = "TX_C";
    break;
  case 32140: // PCS_NAD83_Texas_South_Cen
    sp_nad27 = FALSE; sp = "TX_SC";
    break;
  case 32141: // PCS_NAD83_Texas_South
    sp_nad27 = FALSE; sp = "TX_S";
    break;
  case 32142: // PCS_NAD83_Utah_North
    sp_nad27 = FALSE; sp = "UT_N";
    break;
  case 32143: // PCS_NAD83_Utah_Central
    sp_nad27 = FALSE; sp = "UT_C";
    break;
  case 32144: // PCS_NAD83_Utah_South
    sp_nad27 = FALSE; sp = "UT_S";
    break;
  case 32145: // PCS_NAD83_Vermont
    sp_nad27 = FALSE; sp = "VT";
    break;
  case 32146: // PCS_NAD83_Virginia_North
    sp_nad27 = FALSE; sp = "VA_N";
    break;
  case 32147: // PCS_NAD83_Virginia_South
    sp_nad27 = FALSE; sp = "VA_S";
    break;
  case 32148: // PCS_NAD83_Washington_North
    sp_nad27 = FALSE; sp = "WA_N";
    break;
  case 32149: // PCS_NAD83_Washington_South
    sp_nad27 = FALSE; sp = "WA_S";
    break;
  case 32150: // PCS_NAD83_West_Virginia_N
    sp_nad27 = FALSE; sp = "WV_N";
    break;
  case 32151: // PCS_NAD83_West_Virginia_S
    sp_nad27 = FALSE; sp = "WV_S";
    break;
  case 32152: // PCS_NAD83_Wisconsin_North
    sp_nad27 = FALSE; sp = "WI_N";
    break;
  case 32153: // PCS_NAD83_Wisconsin_Cen
    sp_nad27 = FALSE; sp = "WI_C";
    break;
  case 32154: // PCS_NAD83_Wisconsin_South
    sp_nad27 = FALSE; sp = "WI_S";
    break;
  case 32155: // PCS_NAD83_Wyoming_East
    sp_nad27 = FALSE; sp = "WY_E";
    break;
  case 32156: // PCS_NAD83_Wyoming_E_Cen
    sp_nad27 = FALSE; sp = "WY_EC";
    break;
  case 32157: // PCS_NAD83_Wyoming_W_Cen
    sp_nad27 = FALSE; sp = "WY_WC";
    break;
  case 32158: // PCS_NAD83_Wyoming_West
    sp_nad27 = FALSE; sp = "WY_W";
    break;
  case 32161: // PCS_NAD83_Puerto_Rico_Virgin_Is
    sp_nad27 = FALSE; sp = "PR";
    break;
  case 32201: // PCS_WGS72_UTM_zone_1N 
  case 32202: // PCS_WGS72_UTM_zone_2N 
  case 32203: // PCS_WGS72_UTM_zone_3N 
  case 32204: // PCS_WGS72_UTM_zone_4N 
  case 32205: // PCS_WGS72_UTM_zone_5N 
  case 32206: // PCS_WGS72_UTM_zone_6N 
  case 32207: // PCS_WGS72_UTM_zone_7N 
  case 32208:
  case 32209:
  case 32210:
  case 32211:
  case 32212:
  case 32213:
  case 32214:
  case 32215:
  case 32216:
  case 32217:
  case 32218:
  case 32219:
  case 32220:
  case 32221:
  case 32222:
  case 32223:
  case 32224:
  case 32225:
  case 32226:
  case 32227:
  case 32228:
  case 32229:
  case 32230:
  case 32231:
  case 32232:
  case 32233:
  case 32234:
  case 32235:
  case 32236:
  case 32237:
  case 32238:
  case 32239:
  case 32240:
  case 32241:
  case 32242:
  case 32243:
  case 32244:
  case 32245:
  case 32246:
  case 32247:
  case 32248:
  case 32249:
  case 32250:
  case 32251:
  case 32252:
  case 32253:
  case 32254:
  case 32255:
  case 32256:
  case 32257:
  case 32258:
  case 32259: // PCS_WGS72_UTM_zone_59N 
  case 32260: // PCS_WGS72_UTM_zone_60N 
    utm_northern = TRUE; utm_zone = value-32200;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32301: // PCS_WGS72_UTM_zone_1S
  case 32302: // PCS_WGS72_UTM_zone_2S
  case 32303: // PCS_WGS72_UTM_zone_3S
  case 32304: // PCS_WGS72_UTM_zone_4S
  case 32305: // PCS_WGS72_UTM_zone_5S
  case 32306: // PCS_WGS72_UTM_zone_6S
  case 32307: // PCS_WGS72_UTM_zone_7S
  case 32308:
  case 32309:
  case 32310:
  case 32311:
  case 32312:
  case 32313:
  case 32314:
  case 32315:
  case 32316:
  case 32317:
  case 32318:
  case 32319:
  case 32320:
  case 32321:
  case 32322:
  case 32323:
  case 32324:
  case 32325:
  case 32326:
  case 32327:
  case 32328:
  case 32329:
  case 32330:
  case 32331:
  case 32332:
  case 32333:
  case 32334:
  case 32335:
  case 32336:
  case 32337:
  case 32338:
  case 32339:
  case 32340:
  case 32341:
  case 32342:
  case 32343:
  case 32344:
  case 32345:
  case 32346:
  case 32347:
  case 32348:
  case 32349:
  case 32350:
  case 32351:
  case 32352:
  case 32353:
  case 32354:
  case 32355:
  case 32356:
  case 32357:
  case 32358:
  case 32359: // PCS_WGS72_UTM_zone_59S
  case 32360: // PCS_WGS72_UTM_zone_60S
    utm_northern = FALSE; utm_zone = value-32300;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32401: // PCS_WGS72BE_UTM_zone_1N
  case 32402: // PCS_WGS72BE_UTM_zone_2N
  case 32403: // PCS_WGS72BE_UTM_zone_3N
  case 32404: // PCS_WGS72BE_UTM_zone_4N
  case 32405: // PCS_WGS72BE_UTM_zone_5N
  case 32406: // PCS_WGS72BE_UTM_zone_6N
  case 32407: // PCS_WGS72BE_UTM_zone_7N
  case 32408:
  case 32409:
  case 32410:
  case 32411:
  case 32412:
  case 32413:
  case 32414:
  case 32415:
  case 32416:
  case 32417:
  case 32418:
  case 32419:
  case 32420:
  case 32421:
  case 32422:
  case 32423:
  case 32424:
  case 32425:
  case 32426:
  case 32427:
  case 32428:
  case 32429:
  case 32430:
  case 32431:
  case 32432:
  case 32433:
  case 32434:
  case 32435:
  case 32436:
  case 32437:
  case 32438:
  case 32439:
  case 32440:
  case 32441:
  case 32442:
  case 32443:
  case 32444:
  case 32445:
  case 32446:
  case 32447:
  case 32448:
  case 32449:
  case 32450:
  case 32451:
  case 32452:
  case 32453:
  case 32454:
  case 32455:
  case 32456:
  case 32457:
  case 32458:
  case 32459: // PCS_WGS72BE_UTM_zone_59N
  case 32460: // PCS_WGS72BE_UTM_zone_60N
    utm_northern = TRUE; utm_zone = value-32400;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32501: // PCS_WGS72BE_UTM_zone_1S
  case 32502: // PCS_WGS72BE_UTM_zone_2S
  case 32503: // PCS_WGS72BE_UTM_zone_3S
  case 32504: // PCS_WGS72BE_UTM_zone_4S
  case 32505: // PCS_WGS72BE_UTM_zone_5S
  case 32506: // PCS_WGS72BE_UTM_zone_6S
  case 32507: // PCS_WGS72BE_UTM_zone_7S
  case 32508:
  case 32509:
  case 32510:
  case 32511:
  case 32512:
  case 32513:
  case 32514:
  case 32515:
  case 32516:
  case 32517:
  case 32518:
  case 32519:
  case 32520:
  case 32521:
  case 32522:
  case 32523:
  case 32524:
  case 32525:
  case 32526:
  case 32527:
  case 32528:
  case 32529:
  case 32530:
  case 32531:
  case 32532:
  case 32533:
  case 32534:
  case 32535:
  case 32536:
  case 32537:
  case 32538:
  case 32539:
  case 32540:
  case 32541:
  case 32542:
  case 32543:
  case 32544:
  case 32545:
  case 32546:
  case 32547:
  case 32548:
  case 32549:
  case 32550:
  case 32551:
  case 32552:
  case 32553:
  case 32554:
  case 32555:
  case 32556:
  case 32557:
  case 32558:
  case 32559: // PCS_WGS72BE_UTM_zone_59S
  case 32560: // PCS_WGS72BE_UTM_zone_60S
    utm_northern = FALSE; utm_zone = value-32500;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32601: // PCS_WGS84_UTM_zone_1N
  case 32602: // PCS_WGS84_UTM_zone_2N
  case 32603: // PCS_WGS84_UTM_zone_3N
  case 32604: // PCS_WGS84_UTM_zone_4N
  case 32605: // PCS_WGS84_UTM_zone_5N
  case 32606: // PCS_WGS84_UTM_zone_6N
  case 32607: // PCS_WGS84_UTM_zone_7N
  case 32608:
  case 32609:
  case 32610:
  case 32611:
  case 32612:
  case 32613:
  case 32614:
  case 32615:
  case 32616:
  case 32617:
  case 32618:
  case 32619:
  case 32620:
  case 32621:
  case 32622:
  case 32623:
  case 32624:
  case 32625:
  case 32626:
  case 32627:
  case 32628:
  case 32629:
  case 32630:
  case 32631:
  case 32632:
  case 32633:
  case 32634:
  case 32635:
  case 32636:
  case 32637:
  case 32638:
  case 32639:
  case 32640:
  case 32641:
  case 32642:
  case 32643:
  case 32644:
  case 32645:
  case 32646:
  case 32647:
  case 32648:
  case 32649:
  case 32650:
  case 32651:
  case 32652:
  case 32653:
  case 32654:
  case 32655:
  case 32656:
  case 32657:
  case 32658:
  case 32659: // PCS_WGS84_UTM_zone_59N
  case 32660: // PCS_WGS84_UTM_zone_60N
    utm_northern = TRUE; utm_zone = value-32600;
    ellipsoid_id = CRS_ELLIPSOID_WGS84;
    break;
  case 32701: // PCS_WGS84_UTM_zone_1S
  case 32702: // PCS_WGS84_UTM_zone_2S
  case 32703: // PCS_WGS84_UTM_zone_3S
  case 32704: // PCS_WGS84_UTM_zone_4S
  case 32705: // PCS_WGS84_UTM_zone_5S
  case 32706: // PCS_WGS84_UTM_zone_6S
  case 32707: // PCS_WGS84_UTM_zone_7S
  case 32708:
  case 32709:
  case 32710:
  case 32711:
  case 32712:
  case 32713:
  case 32714:
  case 32715:
  case 32716:
  case 32717:
  case 32718:
  case 32719:
  case 32720:
  case 32721:
  case 32722:
  case 32723:
  case 32724:
  case 32725:
  case 32726:
  case 32727:
  case 32728:
  case 32729:
  case 32730:
  case 32731:
  case 32732:
  case 32733:
  case 32734:
  case 32735:
  case 32736:
  case 32737:
  case 32738:
  case 32739:
  case 32740:
  case 32741:
  case 32742:
  case 32743:
  case 32744:
  case 32745:
  case 32746:
  case 32747:
  case 32748:
  case 32749:
  case 32750:
  case 32751:
  case 32752:
  case 32753:
  case 32754:
  case 32755:
  case 32756:
  case 32757:
  case 32758:
  case 32759: // PCS_WGS84_UTM_zone_59S
  case 32760: // PCS_WGS84_UTM_zone_60S
    utm_northern = FALSE; utm_zone = value-32700;
    ellipsoid_id = CRS_ELLIPSOID_WGS84;
    break;
  }

  if (ellipsoid_id == -1)
  {
    if (utm_zone != -1) ellipsoid_id = CRS_ELLIPSOID_WGS84;
    else if (sp != 0) ellipsoid_id = (sp_nad27 ? CRS_ELLIPSOID_NAD27 : CRS_ELLIPSOID_NAD83);
  }

  if (ellipsoid_id != -1)
  {
    set_ellipsoid(ellipsoid_id, TRUE);
  }

  if (utm_zone != -1)
  {
    if (set_utm_projection(utm_zone, utm_northern, TRUE, description))
    {
      return TRUE;
    }
  }

  if (sp)
  {
    if (sp_nad27)
    {
      if (set_state_plane_nad27_lcc(sp, TRUE, description))
      {
        return TRUE;
      }
      if (set_state_plane_nad27_tm(sp, TRUE, description))
      {
        return TRUE;
      }
    }
    else
    {
      if (set_state_plane_nad83_lcc(sp, TRUE, description))
      {
        return TRUE;
      }
      if (set_state_plane_nad83_tm(sp, TRUE, description))
      {
        return TRUE;
      }
    }
  }

  if (value == EPSG_ETRS89_Poland_CS92)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, -5300000.0, 0.0, 19.0, 0.9993, 0); // "ETRS89 / Poland CS92"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / Poland CS92");
    return TRUE;
  }
  else if (value == EPSG_NZGD2000)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(1600000.0, 10000000.0, 0.0, 173.0, 0.9996, 0); // "NZGD2000 / New Zealand Transverse Mercator 2000"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "NZGD2000");
    return TRUE;
  }
  else if (value == EPSG_NAD83_HARN_UTM2_South_American_Samoa)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 10000000.0, 0.0, -171.0, 0.9996, 0); // "NAD83(HARN) / UTM zone 2S (American Samoa)"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "UTM zone 2S (American Samoa)");
    return TRUE;
  }
  else if (value == EPSG_NAD83_HARN_Virginia_North_ftUS)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(11482916.667, 6561666.667, 37.66666666666666, -78.5, 39.2, 38.03333333333333, 0); // "NAD83(HARN) / Virginia North (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83(HARN) / Virginia North (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_NAD83_HARN_Virginia_South_ftUS)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(11482916.667, 3280833.333, 36.33333333333334, -78.5, 37.96666666666667, 36.76666666666667, 0); // "NAD83(HARN) / Virginia South (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83(HARN) / Virginia South (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_LCC)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(4000000.0, 2800000.0, 52.0, 10.0, 35.0, 65.0, 0); // "ETRS89 / ETRS-LCC"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-LCC");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM34)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 21.0, 0.9996, 0); // "ETRS89 / ETRS-TM34"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM34");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM35)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 27.0, 0.9996, 0); // "ETRS89 / ETRS-TM35"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM35");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM36)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 33.0, 0.9996, 0); // "ETRS89 / ETRS-TM36"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM36");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM35FIN)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 27.0, 0.9996, 0); // "ETRS89 / ETRS-TM35FIN"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM35FIN");
    return TRUE;
  }
  else if (value == EPSG_Fiji_1956_UTM60_South)
  {
    set_ellipsoid(CRS_ELLIPSOID_Inter, TRUE);
    set_utm_projection(60, false, 0); // "Fiji 1956 / UTM zone 60S"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Fiji 1956 / UTM zone 60S");
  }
  else if (value == EPSG_Fiji_1956_UTM1_South)
  {
    set_ellipsoid(CRS_ELLIPSOID_Inter, TRUE);
    set_utm_projection(1, false, 0); // "Fiji 1956 / UTM zone 1S"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Fiji 1956 / UTM zone 1S");
  }
  else if (value == EPSG_Fiji_Map_Grid_1986)
  {
    set_ellipsoid(CRS_ELLIPSOID_WGS72, TRUE);
    set_transverse_mercator_projection(2000000.0, 4000000.0, -17.0, 178.75, 0.99985, 0); // "Fiji 1986 / Fiji Map Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Fiji 1986 / Fiji Map Grid");
    return TRUE;
  }
  else if (value == EPSG_Slovene_National_Grid_1996)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(1312333.333 * surveyfeet2meter, 0.0 * surveyfeet2meter, 37.66666666666666, -77, 39.45, 38.3, 0); // "NAD83(NSRS2007) / Maryland (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83(NSRS2007) / Maryland (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_Slovene_National_Grid_1996)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, -5000000.0, 0.0, 15.0, 0.9999, 0); // "Slovenia 1996 / Slovene National Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Slovenia 1996 / Slovene National Grid");
    return TRUE;
  }
  else if (value == EPSG_MGI_1901_Slovene_National_Grid)
  {
    set_ellipsoid(3, TRUE); // Bessel 1841
    set_transverse_mercator_projection(500000.0, -5000000.0, 0.0, 15.0, 0.9999, 0); // "MGI 1901 / Slovene National Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "MGI 1901 / Slovene National Grid");
    return TRUE;    
  }
  else if ((EPSG_RGF93_CC42 <= value) && (value <= EPSG_RGF93_CC50))
  {
    int v = value - EPSG_RGF93_CC42;
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(1700000.0, 1200000.0+v*1000000, 42.0+v, 3.0, 41.25+v, 42.75+v, 0);
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "RGF93 / CC%d Reseau_Geodesique_Francais_1993", value-3900);
    return TRUE;
  }
  else if ((EPSG_ETRS89_DKTM1 <= value) && (value <= EPSG_ETRS89_DKTM4))
  {
    int v = ((value - EPSG_ETRS89_DKTM1)%4) + 1;
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(200000.0*v, -5000000.0, 0.0, 9.0 + (v == 1 ? 0.0 : (v == 2 ? 1.0 : (v == 3 ? 2.75 : 6.0))), 0.99998, 0);
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / DKTM%d", v);
    return TRUE;
  }
  else if (value == EPSG_ETRS89_UTM32_north_zE_N)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(32500000.0, 0.0, 0.0, 9.0, 0.9996, 0); // "ETRS89 / UTM zone 32N (zE-N)"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / UTM zone 32N (zE-N)");
    return TRUE;
  }
  else if ((EPSG_ETRS89_NTM_zone_5 <= value) && (value <= EPSG_ETRS89_NTM_zone_30))
  {
    int v = value - 5100;
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(100000.0, 1000000.0, 58.0, 0.3+v, 1.0, 0);
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / NTM zone %d", v);
    return TRUE;
  }
  else if (value == EPSG_ETRS89_UTM33_north_zE_N)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(33500000.0, 0.0, 0.0, 15.0, 0.9996, 0); // "ETRS89 / UTM zone 33N (zE-N)"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / UTM zone 33N (zE-N)");
    return TRUE;
  }
  else if (value == EPSG_OSGB_1936)
  {
    set_ellipsoid(1, TRUE); // Airy 1830
    set_transverse_mercator_projection(400000.0, -100000.0, 49.0, -2.0, 0.9996012717, 0); // "OSGB 1936 / British National Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "OSGB 1936 / British National Grid");
    return TRUE;
  } 
  else if (value == EPSG_Belgian_Lambert_1972)
  {
    set_ellipsoid(CRS_ELLIPSOID_Inter, TRUE);
    set_lambert_conformal_conic_projection(150000.013, 5400088.438, 90, 4.367486666666666, 51.16666723333333, 49.8333339, 0); // "Belge 1972 / Belgian Lambert 72"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Belge 1972 / Belgian Lambert 72");
    return TRUE;
  }
  fprintf(stderr, "CRScheck::set_projection_from_ProjectedCSTypeGeoKey: %d not implemented\n", value);
  return FALSE;
}

BOOL CRScheck::check_geokeys(LASheader* lasheader, CHAR* description)
{
  BOOL has_projection = FALSE;
  BOOL user_defined_ellipsoid = FALSE;
  I32 user_defined_projection = 0;
  I32 offsetProjStdParallel1GeoKey = -1;
  I32 offsetProjStdParallel2GeoKey = -1;
  I32 offsetProjNatOriginLatGeoKey = -1;
  I32 offsetProjFalseEastingGeoKey = -1;
  I32 offsetProjFalseNorthingGeoKey = -1;
  I32 offsetProjCenterLongGeoKey = -1;
  I32 offsetProjScaleAtNatOriginGeoKey = -1;
  I32 ellipsoid = -1;

  U32 num_geokey_entries = lasheader->geokeys->number_of_keys;
  LASgeokey_entry* geokey_entries = lasheader->geokey_entries;
  F64* geokey_double_params = lasheader->geokey_double_params;

  for (U32 i = 0; i < num_geokey_entries; i++)
  {
    switch (geokey_entries[i].key_id)
    {
    case 1024: // GTModelTypeGeoKey
      if (geokey_entries[i].value_offset == 2) // ModelTypeGeographic
      {
        has_projection = set_longlat_projection(TRUE, description);
      }
      break;
    case 2048: // GeographicTypeGeoKey
      switch (geokey_entries[i].value_offset)
      {
      case 32767: // user-defined GCS
        user_defined_ellipsoid = TRUE;
        break;
      case 4001: // GCSE_Airy1830
        ellipsoid = 1;
        break;
      case 4002: // GCSE_AiryModified1849 
        ellipsoid = 16;
        break;
      case 4003: // GCSE_AustralianNationalSpheroid
        ellipsoid = 2;
        break;
      case 4004: // GCSE_Bessel1841
      case 4005: // GCSE_Bessel1841Modified
        ellipsoid = 3;
        break;
      case 4006: // GCSE_BesselNamibia
        ellipsoid = 4;
        break;
      case 4008: // GCSE_Clarke1866
      case 4009: // GCSE_Clarke1866Michigan
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 4010: // GCSE_Clarke1880_Benoit
      case 4011: // GCSE_Clarke1880_IGN
      case 4012: // GCSE_Clarke1880_RGS
      case 4013: // GCSE_Clarke1880_Arc
      case 4014: // GCSE_Clarke1880_SGA1922
      case 4034: // GCSE_Clarke1880
        ellipsoid = 6;
        break;
      case 4015: // GCSE_Everest1830_1937Adjustment
      case 4016: // GCSE_Everest1830_1967Definition
      case 4017: // GCSE_Everest1830_1975Definition
        ellipsoid = 7;
        break;
      case 4018: // GCSE_Everest1830Modified
        ellipsoid = 17;
        break;
      case 4019: // GCSE_GRS1980
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 4020: // GCSE_Helmert1906
        ellipsoid = 12;
        break;
      case 4022: // GCSE_International1924
      case 4023: // GCSE_International1967
        ellipsoid = 14;
        break;
      case 4024: // GCSE_Krassowsky1940
        ellipsoid = 15;
        break;
      case 4030: // GCSE_WGS84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      case 4267: // GCS_NAD27
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 4269: // GCS_NAD83
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 4322: // GCS_WGS_72
        ellipsoid = CRS_ELLIPSOID_WGS72;
        break;
      case 4326: // GCS_WGS_84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      default:
        fprintf(stderr, "GeographicTypeGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 2050: // GeogGeodeticDatumGeoKey 
      switch (geokey_entries[i].value_offset)
      {
      case 32767: // user-defined GCS
        user_defined_ellipsoid = TRUE;
        break;
      case 6202: // Datum_Australian_Geodetic_Datum_1966
      case 6203: // Datum_Australian_Geodetic_Datum_1984
        ellipsoid = 2;
        break;
      case 6267: // Datum_North_American_Datum_1927
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 6269: // Datum_North_American_Datum_1983
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 6322: // Datum_WGS72
        ellipsoid = CRS_ELLIPSOID_WGS72;
        break;
      case 6326: // Datum_WGS84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      case 6001: // DatumE_Airy1830
        ellipsoid = 1;
        break;
      case 6002: // DatumE_AiryModified1849
        ellipsoid = 16;
        break;
      case 6003: // DatumE_AustralianNationalSpheroid
        ellipsoid = 2;
        break;
      case 6004: // DatumE_Bessel1841
      case 6005: // DatumE_BesselModified
        ellipsoid = 3;
        break;
      case 6006: // DatumE_BesselNamibia
        ellipsoid = 4;
        break;
      case 6008: // DatumE_Clarke1866
      case 6009: // DatumE_Clarke1866Michigan
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 6010: // DatumE_Clarke1880_Benoit
      case 6011: // DatumE_Clarke1880_IGN
      case 6012: // DatumE_Clarke1880_RGS
      case 6013: // DatumE_Clarke1880_Arc
      case 6014: // DatumE_Clarke1880_SGA1922
      case 6034: // DatumE_Clarke1880
        ellipsoid = 6;
        break;
      case 6015: // DatumE_Everest1830_1937Adjustment
      case 6016: // DatumE_Everest1830_1967Definition
      case 6017: // DatumE_Everest1830_1975Definition
        ellipsoid = 7;
        break;
      case 6018: // DatumE_Everest1830Modified
        ellipsoid = 17;
        break;
      case 6019: // DatumE_GRS1980
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 6020: // DatumE_Helmert1906
        ellipsoid = 12;
        break;
      case 6022: // DatumE_International1924
      case 6023: // DatumE_International1967
        ellipsoid = 14;
        break;
      case 6024: // DatumE_Krassowsky1940
        ellipsoid = 15;
        break;
      case 6030: // DatumE_WGS84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      default:
        fprintf(stderr, "GeogGeodeticDatumGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 2052: // GeogLinearUnitsGeoKey 
      switch (geokey_entries[i].value_offset)
      {
      case 9001: // Linear_Meter
        break;
      case 9002: // Linear_Foot
        break;
      case 9003: // Linear_Foot_US_Survey
        break;
      default:
        fprintf(stderr, "GeogLinearUnitsGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 2056: // GeogEllipsoidGeoKey
      switch (geokey_entries[i].value_offset)
      {
      case 7001: // Ellipse_Airy_1830
        ellipsoid = 1;
        break;
      case 7002: // Ellipse_Airy_Modified_1849
        ellipsoid = 16;
        break;
      case 7003: // Ellipse_Australian_National_Spheroid
        ellipsoid = 2;
        break;
      case 7004: // Ellipse_Bessel_1841
      case 7005: // Ellipse_Bessel_Modified
        ellipsoid = 3;
        break;
      case 7006: // Ellipse_Bessel_Namibia
        ellipsoid = 4;
        break;
      case 7008: // Ellipse_Clarke_1866
      case 7009: // Ellipse_Clarke_1866_Michigan
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 7010: // Ellipse_Clarke1880_Benoit
      case 7011: // Ellipse_Clarke1880_IGN
      case 7012: // Ellipse_Clarke1880_RGS
      case 7013: // Ellipse_Clarke1880_Arc
      case 7014: // Ellipse_Clarke1880_SGA1922
      case 7034: // Ellipse_Clarke1880
        ellipsoid = 6;
        break;
      case 7015: // Ellipse_Everest1830_1937Adjustment
      case 7016: // Ellipse_Everest1830_1967Definition
      case 7017: // Ellipse_Everest1830_1975Definition
        ellipsoid = 7;
        break;
      case 7018: // Ellipse_Everest1830Modified
        ellipsoid = 17;
        break;
      case 7019: // Ellipse_GRS_1980
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 7020: // Ellipse_Helmert1906
        ellipsoid = 12;
        break;
      case 7022: // Ellipse_International1924
      case 7023: // Ellipse_International1967
        ellipsoid = 14;
        break;
      case 7024: // Ellipse_Krassowsky1940
        ellipsoid = 15;
        break;
      case 7030: // Ellipse_WGS_84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      default:
        fprintf(stderr, "GeogEllipsoidGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 3072: // ProjectedCSTypeGeoKey
      if (geokey_entries[i].value_offset != 32767)
        has_projection = set_projection_from_ProjectedCSTypeGeoKey(geokey_entries[i].value_offset, description);
      break;
    case 3075: // ProjCoordTransGeoKey
      user_defined_projection = 0;
      switch (geokey_entries[i].value_offset)
      {
      case 1: // CT_TransverseMercator
        user_defined_projection = 1;
        break;
      case 8: // CT_LambertConfConic_2SP
        user_defined_projection = 8;
        break;
      case 2: // CT_TransvMercator_Modified_Alaska
        fprintf(stderr, "ProjCoordTransGeoKey: CT_TransvMercator_Modified_Alaska not implemented\n");
        break;
      case 3: // CT_ObliqueMercator
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator not implemented\n");
        break;
      case 4: // CT_ObliqueMercator_Laborde
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator_Laborde not implemented\n");
        break;
      case 5: // CT_ObliqueMercator_Rosenmund
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator_Rosenmund not implemented\n");
        break;
      case 6: // CT_ObliqueMercator_Spherical
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator_Spherical not implemented\n");
        break;
      case 7: // CT_Mercator
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Mercator not implemented\n");
        break;
      case 9: // CT_LambertConfConic_Helmert
        fprintf(stderr, "ProjCoordTransGeoKey: CT_LambertConfConic_Helmert not implemented\n");
        break;
      case 10: // CT_LambertAzimEqualArea
        fprintf(stderr, "ProjCoordTransGeoKey: CT_LambertAzimEqualArea not implemented\n");
        break;
      case 11: // CT_AlbersEqualArea
        fprintf(stderr, "ProjCoordTransGeoKey: CT_AlbersEqualArea not implemented\n");
        break;
      case 12: // CT_AzimuthalEquidistant
        fprintf(stderr, "ProjCoordTransGeoKey: CT_AzimuthalEquidistant not implemented\n");
        break;
      case 13: // CT_EquidistantConic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_EquidistantConic not implemented\n");
        break;
      case 14: // CT_Stereographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Stereographic not implemented\n");
        break;
      case 15: // CT_PolarStereographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_PolarStereographic not implemented\n");
        break;
      case 16: // CT_ObliqueStereographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueStereographic not implemented\n");
        break;
      case 17: // CT_Equirectangular
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Equirectangular not implemented\n");
        break;
      case 18: // CT_CassiniSoldner
        fprintf(stderr, "ProjCoordTransGeoKey: CT_CassiniSoldner not implemented\n");
        break;
      case 19: // CT_Gnomonic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Gnomonic not implemented\n");
        break;
      case 20: // CT_MillerCylindrical
        fprintf(stderr, "ProjCoordTransGeoKey: CT_MillerCylindrical not implemented\n");
        break;
      case 21: // CT_Orthographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Orthographic not implemented\n");
        break;
      case 22: // CT_Polyconic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Polyconic not implemented\n");
        break;
      case 23: // CT_Robinson
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Robinson not implemented\n");
        break;
      case 24: // CT_Sinusoidal
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Sinusoidal not implemented\n");
        break;
      case 25: // CT_VanDerGrinten
        fprintf(stderr, "ProjCoordTransGeoKey: CT_VanDerGrinten not implemented\n");
        break;
      case 26: // CT_NewZealandMapGrid
        fprintf(stderr, "ProjCoordTransGeoKey: CT_NewZealandMapGrid not implemented\n");
        break;
      case 27: // CT_TransvMercator_SouthOriented
        fprintf(stderr, "ProjCoordTransGeoKey: CT_TransvMercator_SouthOriented not implemented\n");
        break;
      default:
        fprintf(stderr, "ProjCoordTransGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 3076: // ProjLinearUnitsGeoKey
      set_coordinates_from_ProjLinearUnitsGeoKey(geokey_entries[i].value_offset);
      break;
    case 3078: // ProjStdParallel1GeoKey
      offsetProjStdParallel1GeoKey = geokey_entries[i].value_offset;
      break;
    case 3079: // ProjStdParallel2GeoKey
      offsetProjStdParallel2GeoKey = geokey_entries[i].value_offset;
      break;        
    case 3081: // ProjNatOriginLatGeoKey
      offsetProjNatOriginLatGeoKey = geokey_entries[i].value_offset;
      break;
    case 3082: // ProjFalseEastingGeoKey
      offsetProjFalseEastingGeoKey = geokey_entries[i].value_offset;
      break;
    case 3083: // ProjFalseNorthingGeoKey
      offsetProjFalseNorthingGeoKey = geokey_entries[i].value_offset;
      break;
    case 3088: // ProjCenterLongGeoKey
      offsetProjCenterLongGeoKey = geokey_entries[i].value_offset;
      break;
    case 3092: // ProjScaleAtNatOriginGeoKey
      offsetProjScaleAtNatOriginGeoKey = geokey_entries[i].value_offset;
      break;
    case 4096: // VerticalCSTypeGeoKey 
      set_vertical_from_VerticalCSTypeGeoKey(geokey_entries[i].value_offset);
      break;
    case 4099: // VerticalUnitsGeoKey
      set_elevation_from_VerticalUnitsGeoKey(geokey_entries[i].value_offset);
      break;
    }
  }

  if (ellipsoid != -1)
  {
    set_ellipsoid(ellipsoid, TRUE);
  }

  if (!has_projection)
  {
    if (user_defined_projection == 1)
    {
      if ((offsetProjFalseEastingGeoKey >= 0) &&
          (offsetProjFalseNorthingGeoKey >= 0) &&
          (offsetProjNatOriginLatGeoKey >= 0) &&
          (offsetProjCenterLongGeoKey >= 0) &&
          (offsetProjScaleAtNatOriginGeoKey >= 0))
      {
        F64 falseEasting = geokey_double_params[offsetProjFalseEastingGeoKey];
        F64 falseNorthing = geokey_double_params[offsetProjFalseNorthingGeoKey];
        F64 latOriginDeg = geokey_double_params[offsetProjNatOriginLatGeoKey];
        F64 longMeridianDeg = geokey_double_params[offsetProjCenterLongGeoKey];
        F64 scaleFactor = geokey_double_params[offsetProjScaleAtNatOriginGeoKey];
        set_transverse_mercator_projection(falseEasting, falseNorthing, latOriginDeg, longMeridianDeg, scaleFactor, TRUE, description);
        if (description)
        {
          sprintf(description, "generic transverse mercator");
        }
        has_projection = TRUE;
      }
    }
    else if (user_defined_projection == 8)
    {
      if ((offsetProjFalseEastingGeoKey >= 0) &&
          (offsetProjFalseNorthingGeoKey >= 0) &&
          (offsetProjNatOriginLatGeoKey >= 0) &&
          (offsetProjCenterLongGeoKey >= 0) &&
          (offsetProjStdParallel1GeoKey >= 0) &&
          (offsetProjStdParallel2GeoKey >= 0))
      {
        F64 falseEasting = geokey_double_params[offsetProjFalseEastingGeoKey];
        F64 falseNorthing = geokey_double_params[offsetProjFalseNorthingGeoKey];
        F64 latOriginDeg = geokey_double_params[offsetProjNatOriginLatGeoKey];
        F64 longOriginDeg = geokey_double_params[offsetProjCenterLongGeoKey];
        F64 firstStdParallelDeg = geokey_double_params[offsetProjStdParallel1GeoKey];
        F64 secondStdParallelDeg = geokey_double_params[offsetProjStdParallel2GeoKey];
        set_lambert_conformal_conic_projection(falseEasting, falseNorthing, latOriginDeg, longOriginDeg, firstStdParallelDeg, secondStdParallelDeg, TRUE, description);
        if (description)
        {
          sprintf(description, "generic lambert conformal conic");
        }
        has_projection = TRUE;
      }
    }
  }
  return has_projection;
}

void CRScheck::check(LASheader* lasheader, CHAR* description)
{
  CHAR note[512];

  if (lasheader->geokeys || lasheader->ogc_wkt)
  {
    if (lasheader->geokeys)
    {
      if (!check_geokeys(lasheader, description))
      {
        sprintf(note, "the %u geokeys do not properly specify a Coordinate Reference System", lasheader->geokeys->number_of_keys);
        lasheader->add_fail("CRS", note);
      }
    }
    if (lasheader->ogc_wkt)
    {
      sprintf(note, "there is a OGC WKT string but its check is not yet implemented");
      lasheader->add_warning("CRS", note);
    }
  }
  else
  {
    sprintf(note, "neither GEOTIFF tags nor OGC WKT specify Coordinate Reference System");
    lasheader->add_fail("CRS", note);
  }
}

CRScheck::CRScheck()
{
  coordinate_units[0] = coordinate_units[1] = 0;
  elevation_units[0] = elevation_units[1] = 0;
  vertical_epsg[0] = vertical_epsg[1] = 0;
  ellipsoids[0] = ellipsoids[1] = 0;
  projections[0] = projections[1] = 0;
};

CRScheck::~CRScheck()
{
  if (ellipsoids[0]) delete ellipsoids[0];
  if (ellipsoids[1]) delete ellipsoids[1];
  if (projections[0]) delete projections[0];
  if (projections[1]) delete projections[1];
};
