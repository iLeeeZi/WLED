#include "wled.h"

/*
 * Color conversion & utility methods
 */

/*
 * color blend function
 */
uint32_t color_blend(uint32_t color1, uint32_t color2, uint16_t blend, bool b16) {
  if(blend == 0)   return color1;
  uint16_t blendmax = b16 ? 0xFFFF : 0xFF;
  if(blend == blendmax) return color2;
  uint8_t shift = b16 ? 16 : 8;

  uint32_t w1 = W(color1);
  uint32_t r1 = R(color1);
  uint32_t g1 = G(color1);
  uint32_t b1 = B(color1);

  uint32_t w2 = W(color2);
  uint32_t r2 = R(color2);
  uint32_t g2 = G(color2);
  uint32_t b2 = B(color2);

  uint32_t w3 = ((w2 * blend) + (w1 * (blendmax - blend))) >> shift;
  uint32_t r3 = ((r2 * blend) + (r1 * (blendmax - blend))) >> shift;
  uint32_t g3 = ((g2 * blend) + (g1 * (blendmax - blend))) >> shift;
  uint32_t b3 = ((b2 * blend) + (b1 * (blendmax - blend))) >> shift;

  return RGBW32(r3, g3, b3, w3);
}

/*
 * color add function that preserves ratio
 * idea: https://github.com/Aircoookie/WLED/pull/2465 by https://github.com/Proto-molecule
 */
uint32_t color_add(uint32_t c1, uint32_t c2)
{
  uint32_t r = R(c1) + R(c2);
  uint32_t g = G(c1) + G(c2);
  uint32_t b = B(c1) + B(c2);
  uint32_t w = W(c1) + W(c2);
  uint16_t max = r;
  if (g > max) max = g;
  if (b > max) max = b;
  if (w > max) max = w;
  if (max < 256) return RGBW32(r, g, b, w);
  else           return RGBW32(r * 255 / max, g * 255 / max, b * 255 / max, w * 255 / max);
}

void setRandomColor(byte* rgb)
{
  lastRandomIndex = strip.getMainSegment().get_random_wheel_index(lastRandomIndex);
  colorHStoRGB(lastRandomIndex*256,255,rgb);
}

void colorHStoRGB(uint16_t hue, byte sat, byte* rgb) //hue, sat to rgb
{
  float h = ((float)hue)/65535.0;
  float s = ((float)sat)/255.0;
  byte i = floor(h*6);
  float f = h * 6-i;
  float p = 255 * (1-s);
  float q = 255 * (1-f*s);
  float t = 255 * (1-(1-f)*s);
  switch (i%6) {
    case 0: rgb[0]=255,rgb[1]=t,rgb[2]=p;break;
    case 1: rgb[0]=q,rgb[1]=255,rgb[2]=p;break;
    case 2: rgb[0]=p,rgb[1]=255,rgb[2]=t;break;
    case 3: rgb[0]=p,rgb[1]=q,rgb[2]=255;break;
    case 4: rgb[0]=t,rgb[1]=p,rgb[2]=255;break;
    case 5: rgb[0]=255,rgb[1]=p,rgb[2]=q;
  }
}

//get RGB values from color temperature in K (https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html)
void colorKtoRGB(uint16_t kelvin, byte* rgb) //white spectrum to rgb, calc
{
  float r = 0, g = 0, b = 0;
  float temp = kelvin / 100;
  if (temp <= 66) {
    r = 255;
    g = round(99.4708025861 * log(temp) - 161.1195681661);
    if (temp <= 19) {
      b = 0;
    } else {
      b = round(138.5177312231 * log((temp - 10)) - 305.0447927307);
    }
  } else {
    r = round(329.698727446 * pow((temp - 60), -0.1332047592));
    g = round(288.1221695283 * pow((temp - 60), -0.0755148492));
    b = 255;
  }
  //g += 12; //mod by Aircoookie, a bit less accurate but visibly less pinkish
  rgb[0] = (uint8_t) constrain(r, 0, 255);
  rgb[1] = (uint8_t) constrain(g, 0, 255);
  rgb[2] = (uint8_t) constrain(b, 0, 255);
  rgb[3] = 0;
}

void colorCTtoRGB(uint16_t mired, byte* rgb) //white spectrum to rgb, bins
{
  //this is only an approximation using WS2812B with gamma correction enabled
  if (mired > 475) {
    rgb[0]=255;rgb[1]=199;rgb[2]=92;//500
  } else if (mired > 425) {
    rgb[0]=255;rgb[1]=213;rgb[2]=118;//450
  } else if (mired > 375) {
    rgb[0]=255;rgb[1]=216;rgb[2]=118;//400
  } else if (mired > 325) {
    rgb[0]=255;rgb[1]=234;rgb[2]=140;//350
  } else if (mired > 275) {
    rgb[0]=255;rgb[1]=243;rgb[2]=160;//300
  } else if (mired > 225) {
    rgb[0]=250;rgb[1]=255;rgb[2]=188;//250
  } else if (mired > 175) {
    rgb[0]=247;rgb[1]=255;rgb[2]=215;//200
  } else {
    rgb[0]=237;rgb[1]=255;rgb[2]=239;//150
  }
}

#ifndef WLED_DISABLE_HUESYNC
void colorXYtoRGB(float x, float y, byte* rgb) //coordinates to rgb (https://www.developers.meethue.com/documentation/color-conversions-rgb-xy)
{
  float z = 1.0f - x - y;
  float X = (1.0f / y) * x;
  float Z = (1.0f / y) * z;
  float r = (int)255*(X * 1.656492f - 0.354851f - Z * 0.255038f);
  float g = (int)255*(-X * 0.707196f + 1.655397f + Z * 0.036152f);
  float b = (int)255*(X * 0.051713f - 0.121364f + Z * 1.011530f);
  if (r > b && r > g && r > 1.0f) {
    // red is too big
    g = g / r;
    b = b / r;
    r = 1.0f;
  } else if (g > b && g > r && g > 1.0f) {
    // green is too big
    r = r / g;
    b = b / g;
    g = 1.0f;
  } else if (b > r && b > g && b > 1.0f) {
    // blue is too big
    r = r / b;
    g = g / b;
    b = 1.0f;
  }
  // Apply gamma correction
  r = r <= 0.0031308f ? 12.92f * r : (1.0f + 0.055f) * pow(r, (1.0f / 2.4f)) - 0.055f;
  g = g <= 0.0031308f ? 12.92f * g : (1.0f + 0.055f) * pow(g, (1.0f / 2.4f)) - 0.055f;
  b = b <= 0.0031308f ? 12.92f * b : (1.0f + 0.055f) * pow(b, (1.0f / 2.4f)) - 0.055f;

  if (r > b && r > g) {
    // red is biggest
    if (r > 1.0f) {
      g = g / r;
      b = b / r;
      r = 1.0f;
    }
  } else if (g > b && g > r) {
    // green is biggest
    if (g > 1.0f) {
      r = r / g;
      b = b / g;
      g = 1.0f;
    }
  } else if (b > r && b > g) {
    // blue is biggest
    if (b > 1.0f) {
      r = r / b;
      g = g / b;
      b = 1.0f;
    }
  }
  rgb[0] = 255.0*r;
  rgb[1] = 255.0*g;
  rgb[2] = 255.0*b;
}

void colorRGBtoXY(byte* rgb, float* xy) //rgb to coordinates (https://www.developers.meethue.com/documentation/color-conversions-rgb-xy)
{
  float X = rgb[0] * 0.664511f + rgb[1] * 0.154324f + rgb[2] * 0.162028f;
  float Y = rgb[0] * 0.283881f + rgb[1] * 0.668433f + rgb[2] * 0.047685f;
  float Z = rgb[0] * 0.000088f + rgb[1] * 0.072310f + rgb[2] * 0.986039f;
  xy[0] = X / (X + Y + Z);
  xy[1] = Y / (X + Y + Z);
}
#endif // WLED_DISABLE_HUESYNC

//RRGGBB / WWRRGGBB order for hex
void colorFromDecOrHexString(byte* rgb, char* in)
{
  if (in[0] == 0) return;
  char first = in[0];
  uint32_t c = 0;

  if (first == '#' || first == 'h' || first == 'H') //is HEX encoded
  {
    c = strtoul(in +1, NULL, 16);
  } else
  {
    c = strtoul(in, NULL, 10);
  }

  rgb[0] = R(c);
  rgb[1] = G(c);
  rgb[2] = B(c);
  rgb[3] = W(c);
}

//contrary to the colorFromDecOrHexString() function, this uses the more standard RRGGBB / RRGGBBWW order
bool colorFromHexString(byte* rgb, const char* in) {
  if (in == nullptr) return false;
  size_t inputSize = strnlen(in, 9);
  if (inputSize != 6 && inputSize != 8) return false;

  uint32_t c = strtoul(in, NULL, 16);

  if (inputSize == 6) {
    rgb[0] = (c >> 16);
    rgb[1] = (c >>  8);
    rgb[2] =  c       ;
  } else {
    rgb[0] = (c >> 24);
    rgb[1] = (c >> 16);
    rgb[2] = (c >>  8);
    rgb[3] =  c       ;
  }
  return true;
}

float minf (float v, float w)
{
  if (w > v) return v;
  return w;
}

float maxf (float v, float w)
{
  if (w > v) return w;
  return v;
}

/*
uint32_t colorRGBtoRGBW(uint32_t c)
{
  byte rgb[4];
  rgb[0] = R(c);
  rgb[1] = G(c);
  rgb[2] = B(c);
  rgb[3] = W(c);
  colorRGBtoRGBW(rgb);
  return RGBW32(rgb[0], rgb[1], rgb[2], rgb[3]);
}

void colorRGBtoRGBW(byte* rgb) //rgb to rgbw (http://codewelt.com/rgbw). (RGBW_MODE_LEGACY)
{
  float low = minf(rgb[0],minf(rgb[1],rgb[2]));
  float high = maxf(rgb[0],maxf(rgb[1],rgb[2]));
  if (high < 0.1f) return;
  float sat = 100.0f * ((high - low) / high);   // maximum saturation is 100  (corrected from 255)
  rgb[3] = (byte)((255.0f - sat) / 255.0f * (rgb[0] + rgb[1] + rgb[2]) / 3);
}
*/

byte correctionRGB[4] = {0,0,0,0};
uint16_t lastKelvin = 0;

// adjust RGB values based on color temperature in K (range [2800-10200]) (https://en.wikipedia.org/wiki/Color_balance)
uint32_t colorBalanceFromKelvin(uint16_t kelvin, uint32_t rgb)
{
  //remember so that slow colorKtoRGB() doesn't have to run for every setPixelColor()
  if (lastKelvin != kelvin) colorKtoRGB(kelvin, correctionRGB);  // convert Kelvin to RGB
  lastKelvin = kelvin;
  byte rgbw[4];
  rgbw[0] = ((uint16_t) correctionRGB[0] * R(rgb)) /255; // correct R
  rgbw[1] = ((uint16_t) correctionRGB[1] * G(rgb)) /255; // correct G
  rgbw[2] = ((uint16_t) correctionRGB[2] * B(rgb)) /255; // correct B
  rgbw[3] =                                W(rgb);
  return RGBW32(rgbw[0],rgbw[1],rgbw[2],rgbw[3]);
}

//approximates a Kelvin color temperature from an RGB color.
//this does no check for the "whiteness" of the color,
//so should be used combined with a saturation check (as done by auto-white)
//values from http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html (10deg)
//equation spreadsheet at https://bit.ly/30RkHaN
//accuracy +-50K from 1900K up to 8000K
//minimum returned: 1900K, maximum returned: 10091K (range of 8192)
uint16_t approximateKelvinFromRGB(uint32_t rgb) {
  //if not either red or blue is 255, color is dimmed. Scale up
  uint8_t r = R(rgb), b = B(rgb);
  if (r == b) return 6550; //red == blue at about 6600K (also can't go further if both R and B are 0)

  if (r > b) {
    //scale blue up as if red was at 255
    uint16_t scale = 0xFFFF / r; //get scale factor (range 257-65535)
    b = ((uint16_t)b * scale) >> 8;
    //For all temps K<6600 R is bigger than B (for full bri colors R=255)
    //-> Use 9 linear approximations for blackbody radiation blue values from 2000-6600K (blue is always 0 below 2000K)
    if (b < 33)  return 1900 + b       *6;
    if (b < 72)  return 2100 + (b-33)  *10;
    if (b < 101) return 2492 + (b-72)  *14;
    if (b < 132) return 2900 + (b-101) *16;
    if (b < 159) return 3398 + (b-132) *19;
    if (b < 186) return 3906 + (b-159) *22;
    if (b < 210) return 4500 + (b-186) *25;
    if (b < 230) return 5100 + (b-210) *30;
                 return 5700 + (b-230) *34;
  } else {
    //scale red up as if blue was at 255
    uint16_t scale = 0xFFFF / b; //get scale factor (range 257-65535)
    r = ((uint16_t)r * scale) >> 8;
    //For all temps K>6600 B is bigger than R (for full bri colors B=255)
    //-> Use 2 linear approximations for blackbody radiation red values from 6600-10091K (blue is always 0 below 2000K)
    if (r > 225) return 6600 + (254-r) *50;
    uint16_t k = 8080 + (225-r) *86;
    return (k > 10091) ? 10091 : k;
  }
}

//8bit CIE1931 lookup table used for brightness and color correction
static byte gammaT8[] = {
    0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,
    2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,
    4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,
    7,  7,  7,  8,  8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 11, 11,
   11, 12, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17,
   17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25,
   25, 26, 26, 27, 28, 28, 29, 29, 30, 31, 31, 32, 32, 33, 34, 34,
   35, 36, 37, 37, 38, 39, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47,
   47, 48, 49, 50, 51, 52, 53, 54, 54, 55, 56, 57, 58, 59, 60, 61,
   62, 63, 64, 65, 66, 67, 68, 70, 71, 72, 73, 74, 75, 76, 77, 79,
   80, 81, 82, 83, 85, 86, 87, 88, 90, 91, 92, 94, 95, 96, 98, 99,
  100,102,103,105,106,108,109,110,112,113,115,116,118,120,121,123,
  124,126,128,129,131,132,134,136,138,139,141,143,145,146,148,150,
  152,154,155,157,159,161,163,165,167,169,171,173,175,177,179,181,
  183,185,187,189,191,193,196,198,200,202,204,207,209,211,214,216,
  218,220,223,225,228,230,232,235,237,240,242,245,247,250,252,255
  };

//10bit CIE1931 lookup table used for brightness correction
static uint16_t gammaT10[] = {
    0,    0,    0,    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,    1,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    6,    6,    6,    6,    6,    6,    6,    6,    6,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    8,    8,    8,    8,    8,    8,    8,    8,    8,    9,    9,    9,
    9,    9,    9,    9,    9,    9,   10,   10,   10,   10,   10,   10,   10,   10,   10,   11,
   11,   11,   11,   11,   11,   11,   11,   12,   12,   12,   12,   12,   12,   12,   13,   13,
   13,   13,   13,   13,   13,   14,   14,   14,   14,   14,   14,   14,   15,   15,   15,   15,
   15,   15,   16,   16,   16,   16,   16,   16,   16,   17,   17,   17,   17,   17,   17,   18,
   18,   18,   18,   18,   19,   19,   19,   19,   19,   19,   20,   20,   20,   20,   20,   21,
   21,   21,   21,   21,   22,   22,   22,   22,   22,   23,   23,   23,   23,   23,   24,   24,
   24,   24,   24,   25,   25,   25,   25,   26,   26,   26,   26,   26,   27,   27,   27,   27,
   28,   28,   28,   28,   28,   29,   29,   29,   29,   30,   30,   30,   30,   31,   31,   31,
   31,   32,   32,   32,   32,   33,   33,   33,   34,   34,   34,   34,   35,   35,   35,   35,
   36,   36,   36,   37,   37,   37,   37,   38,   38,   38,   39,   39,   39,   39,   40,   40,
   40,   41,   41,   41,   41,   42,   42,   42,   43,   43,   43,   44,   44,   44,   45,   45,
   45,   46,   46,   46,   47,   47,   47,   48,   48,   48,   49,   49,   49,   50,   50,   50,
   51,   51,   51,   52,   52,   52,   53,   53,   53,   54,   54,   55,   55,   55,   56,   56,
   56,   57,   57,   58,   58,   58,   59,   59,   59,   60,   60,   61,   61,   61,   62,   62,
   63,   63,   63,   64,   64,   65,   65,   65,   66,   66,   67,   67,   68,   68,   68,   69,
   69,   70,   70,   71,   71,   71,   72,   72,   73,   73,   74,   74,   75,   75,   75,   76,
   76,   77,   77,   78,   78,   79,   79,   80,   80,   81,   81,   82,   82,   82,   83,   83,
   84,   84,   85,   85,   86,   86,   87,   87,   88,   88,   89,   89,   90,   90,   91,   91,
   92,   93,   93,   94,   94,   95,   95,   96,   96,   97,   97,   98,   98,   99,   99,  100,
  101,  101,  102,  102,  103,  103,  104,  104,  105,  106,  106,  107,  107,  108,  108,  109,
  110,  110,  111,  111,  112,  113,  113,  114,  114,  115,  116,  116,  117,  117,  118,  119,
  119,  120,  120,  121,  122,  122,  123,  124,  124,  125,  126,  126,  127,  127,  128,  129,
  129,  130,  131,  131,  132,  133,  133,  134,  135,  135,  136,  137,  137,  138,  139,  139,
  140,  141,  141,  142,  143,  144,  144,  145,  146,  146,  147,  148,  149,  149,  150,  151,
  151,  152,  153,  154,  154,  155,  156,  157,  157,  158,  159,  159,  160,  161,  162,  163,
  163,  164,  165,  166,  166,  167,  168,  169,  169,  170,  171,  172,  173,  173,  174,  175,
  176,  177,  177,  178,  179,  180,  181,  181,  182,  183,  184,  185,  186,  186,  187,  188,
  189,  190,  191,  191,  192,  193,  194,  195,  196,  196,  197,  198,  199,  200,  201,  202,
  203,  203,  204,  205,  206,  207,  208,  209,  210,  211,  211,  212,  213,  214,  215,  216,
  217,  218,  219,  220,  221,  222,  223,  223,  224,  225,  226,  227,  228,  229,  230,  231,
  232,  233,  234,  235,  236,  237,  238,  239,  240,  241,  242,  243,  244,  245,  246,  247,
  248,  249,  250,  251,  252,  253,  254,  255,  256,  257,  258,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  271,  272,  273,  274,  275,  276,  277,  278,  279,  280,
  281,  282,  284,  285,  286,  287,  288,  289,  290,  291,  292,  294,  295,  296,  297,  298,
  299,  300,  301,  303,  304,  305,  306,  307,  308,  310,  311,  312,  313,  314,  315,  317,
  318,  319,  320,  321,  323,  324,  325,  326,  327,  329,  330,  331,  332,  333,  335,  336,
  337,  338,  340,  341,  342,  343,  345,  346,  347,  348,  350,  351,  352,  353,  355,  356,
  357,  359,  360,  361,  362,  364,  365,  366,  368,  369,  370,  372,  373,  374,  376,  377,
  378,  380,  381,  382,  384,  385,  386,  388,  389,  390,  392,  393,  394,  396,  397,  399,
  400,  401,  403,  404,  405,  407,  408,  410,  411,  412,  414,  415,  417,  418,  420,  421,
  422,  424,  425,  427,  428,  430,  431,  433,  434,  435,  437,  438,  440,  441,  443,  444,
  446,  447,  449,  450,  452,  453,  455,  456,  458,  459,  461,  462,  464,  465,  467,  468,
  470,  472,  473,  475,  476,  478,  479,  481,  482,  484,  486,  487,  489,  490,  492,  493,
  495,  497,  498,  500,  501,  503,  505,  506,  508,  510,  511,  513,  514,  516,  518,  519,
  521,  523,  524,  526,  528,  529,  531,  533,  534,  536,  538,  539,  541,  543,  544,  546,
  548,  550,  551,  553,  555,  556,  558,  560,  562,  563,  565,  567,  569,  570,  572,  574,
  576,  577,  579,  581,  583,  584,  586,  588,  590,  592,  593,  595,  597,  599,  601,  602,
  604,  606,  608,  610,  612,  613,  615,  617,  619,  621,  623,  625,  626,  628,  630,  632,
  634,  636,  638,  640,  641,  643,  645,  647,  649,  651,  653,  655,  657,  659,  661,  662,
  664,  666,  668,  670,  672,  674,  676,  678,  680,  682,  684,  686,  688,  690,  692,  694,
  696,  698,  700,  702,  704,  706,  708,  710,  712,  714,  716,  718,  720,  722,  724,  726,
  728,  731,  733,  735,  737,  739,  741,  743,  745,  747,  749,  751,  753,  756,  758,  760,
  762,  764,  766,  768,  770,  773,  775,  777,  779,  781,  783,  786,  788,  790,  792,  794,
  796,  799,  801,  803,  805,  807,  810,  812,  814,  816,  819,  821,  823,  825,  827,  830,
  832,  834,  837,  839,  841,  843,  846,  848,  850,  852,  855,  857,  859,  862,  864,  866,
  869,  871,  873,  876,  878,  880,  883,  885,  887,  890,  892,  894,  897,  899,  901,  904,
  906,  909,  911,  913,  916,  918,  921,  923,  925,  928,  930,  933,  935,  938,  940,  942,
  945,  947,  950,  952,  955,  957,  960,  962,  965,  967,  970,  972,  975,  977,  980,  982,
  985,  987,  990,  992,  995,  997, 1000, 1002, 1005, 1008, 1010, 1013, 1015, 1018, 1020, 1023
};

uint8_t gamma8_cal(uint8_t b, float gamma)
{
  return (int)(powf((float)b / 255.0f, gamma) * 255.0f + 0.5f);
}

// re-calculates & fills gamma table
void calcGammaTable(float gamma)
{
  for (uint16_t i = 0; i < 256; i++) {
    gammaT8[i] = gamma8_cal(i, gamma);
  }
}

// used for individual channel or brightness gamma correction
uint8_t gamma8(uint8_t b)
{
  return gammaT8[b];
}

// used for individual channel or brightness gamma correction
uint16_t gamma10(uint16_t b)
{
  return gammaT10[b];
}

// used for color gamma correction
uint32_t gamma32(uint32_t color)
{
  if (!gammaCorrectCol) return color;
  uint8_t w = W(color);
  uint8_t r = R(color);
  uint8_t g = G(color);
  uint8_t b = B(color);
  w = gammaT8[w];
  r = gammaT8[r];
  g = gammaT8[g];
  b = gammaT8[b];
  return RGBW32(r, g, b, w);
}
