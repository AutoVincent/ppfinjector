#pragma once

#include <array>

namespace tdd::tk::rompatch::cd::spec {

// ECMA-130, 14
inline constexpr size_t kSectorSize = 2352;

// 14.1
inline constexpr std::array<uint8_t, 12> kSync =
   {00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 00};

#pragma pack(push, 1)
// 14
union [[nodiscard]] SectorHeader
{
   struct
   {
      // Sector address
      uint8_t minutes;
      uint8_t seconds;
      uint8_t frame;
      // Sector mode
      uint8_t mode;
   } parts;
   uint32_t full;
};

inline constexpr uint8_t kMode0 = 0;
inline constexpr uint8_t kMode1 = 1;
inline constexpr uint8_t kMode2 = 2;

inline constexpr size_t kSectorHeaderSize = 4;
static_assert(sizeof(SectorHeader) == kSectorHeaderSize);

union [[nodiscard]] Edc
{
   uint32_t full;
   uint8_t parts[4];
};

// 14
struct [[nodiscard]] Mode0Data
{
   uint8_t zeros[2336];
};

// 14
struct [[nodiscard]] Mode1Data
{
   uint8_t userData[2048];
   Edc edc;
   
   // 8 bytes of '0'
   uint8_t intermediate[8];
   uint8_t pParity[172];
   uint8_t qParity[104];
};

// 14
struct [[nodiscard]] Mode2Data
{
   uint8_t userData[2336];
};

// CD-ROM XA: 4.3.2.3
struct [[nodiscard]] XaSubmode
{
   uint8_t endOfFile : 1;
   uint8_t realTimeSector : 1;
   uint8_t form : 1;
   uint8_t trigger : 1;
   uint8_t data : 1;
   uint8_t audio : 1;
   uint8_t video : 1;
   uint8_t endOfRecord : 1;
};

inline constexpr uint8_t kXaForm1 = 0;
inline constexpr uint8_t kXaForm2 = 1;

// CD-ROM XA: 4.3.1
union [[nodiscard]] XaSubHeader
{
   struct
   {
      uint8_t fileNumber;
      uint8_t channel;
      XaSubmode submode;
      uint8_t codingInfo;
   } parts;
   uint32_t full;
};

// CD-ROM XA: 4.5.1
struct [[nodiscard]] Mode2Xa1Data
{
   uint8_t data[2048];
   Edc edc;
   uint8_t pParity[172];
   uint8_t qParity[104];
};

// CD-ROM XA: 4.6.1
struct [[nodiscard]] Mode2Xa2Data
{
   uint8_t data[2324];
   // CRC-32 or 0
   Edc edc;
};

struct [[nodiscard]] Mode2XaData
{
   XaSubHeader subheader[2];
   union
   {
      Mode2Xa1Data form1;
      Mode2Xa2Data form2;
   };
};

struct [[nodiscard]] Sector
{
   uint8_t sync[12];
   SectorHeader header;
   // data portion
   union
   {
      Mode0Data mode0;
      Mode1Data mode1;
      Mode2Data mode2;
      Mode2XaData xa;
   };
};

static_assert(sizeof(Sector) == kSectorSize);
#pragma pack(pop)

}