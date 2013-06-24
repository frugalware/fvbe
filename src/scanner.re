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
  hex                  = [0-9a-fA-F];
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

YYDECLARE(
is_ip_v4
,
/*!re2c
  (digit+ [.]) {3} digit+ null { const char *x = YYSTART; while(true) { long n = strtol(x,(char **) &x,10); if(n < 0 || n > 255) return false; if(*x == 0) break; ++x; } return true; }
  any                          { return false;                                                                                                                                        }
*/
)

YYDECLARE(
is_ip_v6
,
/*!re2c
                       [:]   ([:] hex {1,4}) {1,7}  null { return true;  }
  (hex {1,4} [:]) {1} ([:] | ([:] hex {1,4}) {1,6}) null { return true;  }
  (hex {1,4} [:]) {2} ([:] | ([:] hex {1,4}) {1,5}) null { return true;  }
  (hex {1,4} [:]) {3} ([:] | ([:] hex {1,4}) {1,4}) null { return true;  }
  (hex {1,4} [:]) {4} ([:] | ([:] hex {1,4}) {1,3}) null { return true;  }
  (hex {1,4} [:]) {5} ([:] | ([:] hex {1,4}) {1,2}) null { return true;  }
  (hex {1,4} [:]) {6} ([:] | ([:] hex {1,4}) {1,1}) null { return true;  }
  (hex {1,4} [:]) {7} ([:] | (    hex {1,4}) {1,1}) null { return true;  }
  any                                                    { return false; }
*/
)

YYDECLARE(
is_dns_domain
,
/*!re2c
  ((alphadigit | alphadigit (alphadigit|[-]) {0,62} alphadigit) [.])* (alphadigit | alphadigit (alphadigit|[-]) {0,62} alphadigit) null { return YYLENCHECK(255); }
  any                                                                                                                                   { return false;           }
*/
)

YYDECLARE(
is_positive_integer
,
/*!re2c
  digit+ null { return true;  }
  any         { return false; }
*/
)

YYDECLARE(
is_uuid
,
/*!re2c
  hex {8} [-] hex {4} [-] hex {4} [-] hex {4} [-] hex {12} null { return true;  }
  any                                                           { return false; }
*/
)

YYDECLARE(
is_mac_address
,
/*!re2c
  hex {2} [:] hex {2} [:] hex {2} [:] hex {2} [:] hex {2} [:] hex {2} null { return true;  }
  any                                                                      { return false; }
*/
)

YYDECLARE(
is_wpa_pp
,
/*!re2c
  ascii {8,63} null { return true;  }
  any               { return false; }
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
