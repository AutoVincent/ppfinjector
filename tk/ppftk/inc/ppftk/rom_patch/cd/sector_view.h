#pragma once

#include <ppfbase/preprocessor_utils.h>

#include <span>

namespace tdd::tk::rompatch::cd {

namespace spec {
   struct [[nodiscard]] Sector;
}

class [[nodiscard]] SectorView
{
public:
   SectorView();
   SectorView(const uint64_t addr, std::span<uint8_t> data);

   ~SectorView() = default;
   TDD_DEFAULT_COPY_MOVE(SectorView);

   [[nodiscard]] uint64_t SectorNumber() const noexcept;
   [[nodiscard]] uint64_t SectorAddress() const noexcept;

   // Idx of the first valid data byte.
   [[nodiscard]] size_t DataOffset() const noexcept;
   [[nodiscard]] size_t Size() const noexcept;
   [[nodiscard]] bool Empty() const noexcept;
   [[nodiscard]] std::span<uint8_t> Data() const noexcept;

   [[nodiscard]] bool IsComplete() const noexcept;
   spec::Sector* AsSector() const noexcept;

   [[nodiscard]] uint8_t& operator[](const size_t idx) noexcept;
   [[nodiscard]] uint8_t operator[](const size_t idx) const noexcept;

private:
   uint64_t m_number;
   size_t m_offset;
   std::span<uint8_t> m_data;

};

}