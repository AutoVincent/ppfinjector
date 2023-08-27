#pragma once

#include <ppftk/rom_patch/ipatcher.h>
#include <ppftk/rom_patch/patch_descriptor.h>

#include <vector>

namespace tdd::tk::rompatch {

   class [[nodiscard]] FlatPatch : public IPatcher
   {
   private:
      using FlattenedPatches = std::vector<uint64_t>;
   public:
      using ValidationData = PatchItem;

      // sizeof(PatchBlock) = 3 * sizeof(uint64_t) + (patchLength + 7) & ~0b111
      struct PatchBlock
      {
         uint64_t address;
         // byte count of the size of the patch
         uint64_t patchLength;
         // previous = (char*)this - previousBlockOffset
         uint64_t previousBlockOffset;
         // patch is always a multiple of 8 to keep PatchBlock 8-byte aligned.
         uint64_t patch[1];
      };

      using value_type = PatchBlock;
      using size_type = size_t;
      using difference_type = std::ptrdiff_t;
      using pointer = PatchBlock const *;
      using const_pointer = pointer;
      using reference = PatchBlock const &;
      using const_reference = reference;


      class [[nodiscard]] Iterator
      {
      public:
         using iterator_concept = std::bidirectional_iterator_tag;
         using iterator_category = std::bidirectional_iterator_tag;

         using value_type = FlatPatch::value_type;
         using difference_type = FlatPatch::difference_type;
         using pointer = FlatPatch::const_pointer;
         using reference = FlatPatch::reference;

         TDD_DEFAULT_ALL_SPECIAL_MEMBERS(Iterator);

         reference operator*() const noexcept;
         pointer operator->() const noexcept;

         Iterator& operator++() noexcept;
         Iterator operator++(int) noexcept;

         Iterator& operator--() noexcept;
         Iterator operator--(int) noexcept;

         [[nodiscard]] bool operator==(const Iterator& other) const = default;

      private:
         friend class FlatPatch;

         Iterator(
            FlatPatch const* parent,
            FlattenedPatches::const_iterator pos);

         FlatPatch const* m_parent;
         FlattenedPatches::const_iterator m_pos;
      };

      TDD_DEFAULT_CTOR_DTOR(FlatPatch);

      FlatPatch(const PatchDescriptor descriptor);

      FlatPatch(const FlatPatch& other);
      FlatPatch& operator=(const FlatPatch& other) noexcept;

      FlatPatch(FlatPatch&& other);
      FlatPatch& operator=(FlatPatch&& other) noexcept;

      using iterator = Iterator;

      iterator begin() const noexcept;
      iterator end() const noexcept;
      [[nodiscard]] bool empty() const noexcept;

      // IPatcher
      std::optional<AdditionalReads> Patch(
         const uint64_t addr,
         std::span<uint8_t> buffer) override;

   private:
      [[nodiscard]] bool IsInRange(
         const uint64_t targetAddrBegin,
         const uint64_t targetAddrEnd) const;

      // Returns true if a usable block is found
      [[nodiscard]] bool SeekForward(
         const uint64_t targetAddrBegin,
         const uint64_t targetAddrEnd) const;

      [[nodiscard]] bool SeekBackward(
         const uint64_t targetAddrBegin,
         const uint64_t targetAddrEnd) const;

      ValidationData m_validationData;
      FlattenedPatches m_patch;

      iterator m_back;
      mutable iterator m_lastRead;
      
   };

}