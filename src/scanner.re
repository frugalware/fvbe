#pragma once

#define YYCTYPE          unsigned char
#define YYCTYPE2         const char
#define YYCURSOR         p
#define YYSTART          s
#define YYMARKER         m
#define YYDECLARE(A,B,C)                 \
static inline bool A(YYCTYPE2 *YYCURSOR) \
{                                        \
  YYCTYPE2 *YYSTART = YYCURSOR;          \
  YYCTYPE2 *YYMARKER = YYCURSOR;         \
  B                                      \
  C                                      \
}

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

YYDECLARE(
is_root_path
,
/*!re2c
  "/" lower* null { return true;  }
  any             { return false; }
*/
,
)

YYDECLARE(
is_raid_device
,
/*!re2c
  "md" digit+ null { return true;  }
  any              { return false; }
*/
,
)

YYDECLARE(
is_disk_device
,
/*!re2c
  [hsv] "d" lower null { return true;  }
  any                  { return false; }
*/
,
)

#undef YYCTYPE
#undef YYCTYPE2
#undef YYCURSOR
#undef YYSTART
#undef YYMARKER
#undef YYDECLARE
