#pragma once

// All regular expression engines must include a check for null or value 0.
// If you need to check size limits, then the number you require is as follows:
// SIZE_MAX + 1 (null terminator) + 1 (pointer is incremented prior to the code block)

#define YYCTYPE          unsigned char
#define YYCTYPE2         const char
#define YYCURSOR         p
#define YYSTART          s
#define YYMARKER         m
#define YYSILENCE        YYSTART = YYSTART;
#define YYLEN            (YYCURSOR - YYSTART)
#define YYMAX(S)         ((S) + 1 + 1)
#define YYLENCHECK(S)    (YYLEN < YYMAX(S))
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
  "/" lower* null { return true;  }
  any             { return false; }
*/
,
YYSILENCE
)

YYDECLARE(
is_raid_device
,
/*!re2c
  "md" digit+ null { return true;  }
  any              { return false; }
*/
,
YYSILENCE
)

YYDECLARE(
is_disk_device
,
/*!re2c
  [hsv] "d" lower null { return true;  }
  any                  { return false; }
*/
,
YYSILENCE
)

YYDECLARE(
is_partition_name
,
/*!re2c
  ascii+ null { return YYLENCHECK(PARTITION_NAME_MAX); }
  any         { return false;                          }
*/
,
)

YYDECLARE(
is_user_name
,
/*!re2c
  (lower|[_]) (lowerdigit|[_-])* [$]? null { return YYLENCHECK((ssize_t) USER_NAME_MAX); }
  any                                      { return false;                               }
*/
,
)

#undef YYCTYPE
#undef YYCTYPE2
#undef YYCURSOR
#undef YYSTART
#undef YYMARKER
#undef YYSILENCE
#undef YYLEN
#undef YYMAX
#undef YYLENCHECK
#undef YYDECLARE
