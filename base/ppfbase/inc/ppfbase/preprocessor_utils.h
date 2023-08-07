#pragma once

#define TDD_WIDEN2(s) L ## s
#define TDD_WIDEN(s) TDD_WIDEN2(s)

#define TDD_STR2(s) #s
#define TDD_STR(s) TDD_STR2(s)

#define TDD_JOIN_TOKEN2(t1, t2) t1 ## t2
#define TDD_JOIN_TOKEN(t1, t2) TDD_JOIN_TOKEN2(t1, t2)


#define TDD_DISABLE_COPY(TypeName) \
   TypeName(const TypeName&) = delete;    \
   void operator=(const TypeName&) = delete

#define TDD_DISABLE_MOVE(TypeName) \
   TypeName(TypeName&&) = delete;  \
   void operator=(TypeName&&) = delete

#define TDD_DISABLE_COPY_MOVE(TypeName) \
   TDD_DISABLE_COPY(TypeName); \
   TDD_DISABLE_MOVE(TypeName)

#define TDD_DEFAULT_CTOR_DTOR(TypeName) \
   TypeName() = default; \
   ~TypeName() = default

#define TDD_DEFAULT_COPY(TypeName) \
   TypeName(const TypeName&) = default;    \
   TypeName& operator=(const TypeName&) = default

#define TDD_DEFAULT_MOVE(TypeName) \
   TypeName(TypeName&&) = default;  \
   TypeName& operator=(TypeName&&) = default

#define TDD_DEFAULT_COPY_MOVE(TypeName) \
   TDD_DEFAULT_COPY(TypeName); \
   TDD_DEFAULT_MOVE(TypeName)

#define TDD_DEFAULT_ALL_SPECIAL_MEMBERS(TypeName) \
   TDD_DEFAULT_CTOR_DTOR(TypeName); \
   TDD_DEFAULT_COPY_MOVE(TypeName)
