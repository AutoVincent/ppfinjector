#pragma once

#include <ppftk/rom_patch/cd/sector_view.h>

#include <ppfbase/preprocessor_utils.h>

#include <compare>
#include <iterator>
#include <span>

namespace tdd::tk::rompatch::cd {

class [[nodiscard]] SectorRange
{
public:
   using value_type = SectorView;
   using pointer = value_type*;
   using const_pointer = const value_type*;
   using reference = value_type&;
   using const_reference = const value_type&;
   using size_type = size_t;
   using difference_type = SectorDiff;

   class ConstIterator
   {
   public:
      using iterator_concept = std::random_access_iterator_tag;
      using iterator_category = std::random_access_iterator_tag;
      using value_type = SectorRange::value_type;
      using difference_type = SectorRange::difference_type;
      using pointer = SectorRange::const_pointer;
      using reference = SectorRange::const_reference;

      ConstIterator() noexcept;
      ~ConstIterator() = default;
      TDD_DEFAULT_COPY_MOVE(ConstIterator);

      [[nodiscard]] reference operator*() const noexcept;
      [[nodiscard]] pointer operator->() const noexcept;

      ConstIterator& operator++() noexcept;
      ConstIterator operator++(int) noexcept;
      ConstIterator& operator--() noexcept;
      ConstIterator operator--(int) noexcept;

      ConstIterator& operator+=(const difference_type offset) noexcept;
      [[nodiscard]] ConstIterator operator+(
         const difference_type offset) const noexcept;

      ConstIterator& operator-=(const difference_type offset) noexcept;
      [[nodiscard]] ConstIterator operator-(
         const difference_type offset) const noexcept;

      [[nodiscard]] difference_type operator-(
         const ConstIterator& rhs) const noexcept;

      [[nodiscard]] reference operator[](
         const difference_type offset) const noexcept;

      [[nodiscard]] bool operator==(const ConstIterator& other) const noexcept;

      [[nodiscard]] std::strong_ordering operator<=>(
         const ConstIterator& rhs) const noexcept;

   private:
      friend class SectorRange;
      ConstIterator(
         const SectorNumber sectorNumber,
         const ByteAddress rangeAddr,
         std::span<uint8_t> range) noexcept;

      void CheckCompatible(const ConstIterator& other) const noexcept;
      void LoadSector(ByteAddress sectorAddr) noexcept;

      SectorView m_sector;
      ByteAddress m_rangeStart;
      ByteAddress m_rangeEnd;
      std::span<uint8_t> m_range;
   };

   class Iterator : public ConstIterator
   {
   public:
      using Base = ConstIterator;

      using iterator_concept = std::random_access_iterator_tag;
      using iterator_category = std::random_access_iterator_tag;
      using value_type = SectorRange::value_type;
      using difference_type = SectorRange::difference_type;
      using pointer = SectorRange::pointer;
      using reference = SectorRange::reference;

      TDD_DEFAULT_ALL_SPECIAL_MEMBERS(Iterator);

      [[nodiscard]] reference operator*() const noexcept;
      [[nodiscard]] pointer operator->() const noexcept;

      Iterator& operator++() noexcept;
      Iterator operator++(int) noexcept;
      Iterator& operator--() noexcept;
      Iterator operator--(int) noexcept;

      Iterator& operator+=(const difference_type offset) noexcept;
      [[nodiscard]] Iterator operator+(const difference_type offset) noexcept;

      Iterator& operator-=(const difference_type offset) noexcept;

      // iterator diff.
      using Base::operator-;

      [[nodiscard]] Iterator operator-(const difference_type offset) noexcept;
      [[nodiscard]] reference operator[](
         const difference_type offset) const noexcept;

   private:
      friend class SectorRange;
      Iterator(
         const SectorNumber sectorAddr,
         const ByteAddress rangeAddr,
         std::span<uint8_t> range) noexcept;

      Iterator(const ConstIterator& other);
   };

   using iterator = Iterator;
   using const_iterator = ConstIterator;

   SectorRange() noexcept;
   SectorRange(const ByteAddress addr, std::span<uint8_t> data) noexcept;

   ~SectorRange() = default;
   TDD_DEFAULT_COPY_MOVE(SectorRange);

   iterator begin() noexcept;
   const_iterator begin() const noexcept;
   const_iterator cbegin() const noexcept;

   iterator end() noexcept;
   const_iterator end() const noexcept;
   const_iterator cend() const noexcept;

   [[nodiscard]] bool empty() const noexcept;
   [[nodiscard]] size_type size() const noexcept;

private:
   ByteAddress m_addr;
   std::span<uint8_t> m_data;
};

}