#pragma once

#include <ppftk/rom_patch/cd/address.h>
#include <ppfbase/preprocessor_utils.h>

#include <span>

namespace tdd::tk::rompatch::cd {

namespace spec {
   struct [[nodiscard]] Sector;
}

class [[nodiscard]] SectorView
{
public:
   SectorView() noexcept;
   SectorView(const ByteAddress addr, std::span<uint8_t> data) noexcept;

   ~SectorView() = default;
   TDD_DEFAULT_COPY_MOVE(SectorView);

   [[nodiscard]] SectorNumber SectorNumber() const noexcept;
   [[nodiscard]] ByteAddress SectorAddress() const noexcept;

   // Idx of the first valid data byte.
   [[nodiscard]] SectorOffset DataOffset() const noexcept;
   [[nodiscard]] size_t Size() const noexcept;
   [[nodiscard]] bool Empty() const noexcept;
   [[nodiscard]] std::span<uint8_t> Data() const noexcept;

   [[nodiscard]] bool IsComplete() const noexcept;
   spec::Sector* AsSector() const noexcept;

   [[nodiscard]] uint8_t& operator[](const SectorOffset idx) noexcept;
   [[nodiscard]] uint8_t operator[](const SectorOffset idx) const noexcept;

private:
   cd::SectorNumber m_number;
   SectorOffset m_offset;
   std::span<uint8_t> m_data;

};

}