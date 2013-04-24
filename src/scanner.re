#pragma once

#define YYPREFIX  static inline bool
#define YYCTYPE   unsigned char
#define YYCTYPE2  const char
#define YYCURSOR  p
#define YYSTART   s
#define YYMARKER  m

/*!re2c
  re2c:yyfill:enable   = 0;
  re2c:yych:conversion = 1;
  re2c:indent:string   = "  ";
  ascii                = [\x20-\x7e];
  lower                = [a-z];
  upper                = [A-Z];
  digit                = [0-9];
  null                 = [\000];
  any                  = [^];
*/

YYPREFIX is_root_path(YYCTYPE2 *YYCURSOR)
{
  YYCTYPE2 *YYSTART = YYCURSOR;
  YYCTYPE2 *YYMARKER = YYCURSOR;

/*!re2c
  "/" lower* null { return true;  }
  any             { return false; }
*/
}

#undef YYPREFIX
#undef YYCTYPE
#undef YYCTYPE2
#undef YYCURSOR
#undef YYMARKER
