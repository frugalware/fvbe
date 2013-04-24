#pragma once

#define YYCTYPE  unsigned char
#define YYCTYPE2 const char
#define YYCURSOR s
#define YYMARKER m

/*!re2c
  re2c:yyfill:enable   = 0;
  re2c:yych:conversion = 1;
  re2c:indent:string   = "  ";
*/

static inline bool is_root_path(YYCTYPE2 *YYCURSOR)
{
  YYCTYPE2 *YYMARKER = YYCURSOR;

/*!re2c
  [/] [a-z]* [\000] { return true;  }
  [^]               { return false; }
*/
}

#undef YYCTYPE
#undef YYCTYPE2
#undef YYCURSOR
#undef YYMARKER
