#pragma once

#define PPF_WIDEN2(s) L ## s
#define PPF_WIDEN(s) PPF_WIDEN2(s)

#define PPF_STR2(s) #s
#define PPF_STR(s) PPF_STR2(s)

#define PPF_JOIN_TOKEN2(t1, t2) t1 ## t2
#define PPF_JOIN_TOKEN(t1, t2) PPF_JOIN_TOKEN2(t1, t2)