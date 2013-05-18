#pragma once

// All regular expression engines must include a check for null or value 0.
// If you need to check size limits, then the number you require is as follows:
// SIZE_MAX + 1 (null terminator) + 1 (pointer is incremented prior to the code block)

#define YYCTYPE          unsigned char
#define YYCTYPE2         const char
#define YYCURSOR         p
#define YYSTART          s
#define YYMARKER         m
#define YYLEN            (YYCURSOR - YYSTART)
#define YYMAX(S)         ((S) + 1 + 1)
#define YYLENCHECK(S)    (YYLEN < YYMAX(S))
#define YYDECLARE(A,B)                   \
static inline bool A(YYCTYPE2 *YYCURSOR) \
{                                        \
  YYCTYPE2 *YYSTART = YYCURSOR;          \
  YYCTYPE2 *YYMARKER = YYCURSOR;         \
  B                                      \
  YYSTART = YYSTART;                     \
}

/*!re2c
  re2c:yyfill:enable   = 0;
  re2c:yych:conversion = 1;
  re2c:indent:string   = "  ";
  ascii                = [\x20-\x7e];
  lower                = [a-z];
  upper                = [A-Z];
  digit                = [0-9];
  lowerdigit           = [a-z0-9];
  upperdigit           = [A-Z0-9];
  alpha                = [a-zA-Z];
  alphadigit           = [a-zA-Z0-9];
  null                 = [\000];
  any                  = [^];
*/

YYDECLARE(
is_root_path
,
/*!re2c
  "/" lower {0,254} null { return true;  }
  any                    { return false; }
*/
)

YYDECLARE(
is_utf8_locale
,
/*!re2c
  lower+ [_] upper+ ".utf8" null { return true;  }
  any                            { return false; }
*/
)

YYDECLARE(
is_raid_device
,
/*!re2c
  "md" digit {1,3} null { int n = atoi(YYSTART+2); return (n >= 0 && n <= 255); }
  any                   { return false;                                         }
*/
)

YYDECLARE(
is_disk_device
,
/*!re2c
  [hsv] "d" lower null { return true;  }
  any                  { return false; }
*/
)

YYDECLARE(
is_partition_name
,
/*!re2c
  ascii {0,36} null { return true;  }
  any               { return false; }
*/
)

YYDECLARE(
is_user_name
,
/*!re2c
  (lower|[_]) ((lowerdigit|[_-]) {0,31} | (lowerdigit|[_-]) {0,30} [$]) null { return true;  }
  any                                                                        { return false; }
*/
)

YYDECLARE(
is_dns_label
,
/*!re2c
  (alphadigit | alphadigit (alphadigit|[-]) {0,62} alphadigit) null { return true;  }
  any                                                               { return false; }
*/
)

#undef YYCTYPE
#undef YYCTYPE2
#undef YYCURSOR
#undef YYSTART
#undef YYMARKER
#undef YYLEN
#undef YYMAX
#undef YYLENCHECK
#undef YYDECLARE
