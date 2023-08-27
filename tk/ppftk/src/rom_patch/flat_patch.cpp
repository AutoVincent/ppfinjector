#include <ppftk/rom_patch/flat_patch.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch {

namespace {
   // Counts the number 'uint64_t's required.
   static constexpr size_t kFlattenedHeader = 3;


   // The FlattenedPatches is in 'uint64_t'. This returns the count in terms
   // 'uint64_t'.
   [[nodiscard]] ptrdiff_t GetBlockSize(FlatPatch::PatchBlock const* block)
   {
      static constexpr size_t kHeaderSize = 3;
      const auto patchSize =
         (block->patchLength + sizeof(uint64_t) - 1) / sizeof(uint64_t);
      return kHeaderSize + patchSize;
   }

   [[nodiscard]] size_t GetFlattenedSize(const PatchItem& patch)
   {
      const size_t dataLength =
         (patch.data.size() + sizeof(uint64_t) - 1) / sizeof(uint64_t);
      return kFlattenedHeader + dataLength;
   }
}

FlatPatch::Iterator::reference FlatPatch::Iterator::operator*() const noexcept
{
   return *operator->();
}

FlatPatch::Iterator::pointer FlatPatch::Iterator::operator->() const noexcept
{
   TDD_ASSERT(nullptr != m_parent);
   TDD_ASSERT(m_pos != m_parent->m_patch.end());
   return reinterpret_cast<pointer>(&(*m_pos));
}

FlatPatch::Iterator& FlatPatch::Iterator::operator++() noexcept
{
   TDD_ASSERT(nullptr != m_parent);

   const auto offset = GetBlockSize(this->operator->());
   TDD_ASSERT(offset <= std::distance(m_pos, m_parent->m_patch.end()));

   std::advance(m_pos, offset);
   return *this;
}

FlatPatch::Iterator FlatPatch::Iterator::operator++(int) noexcept
{
   Iterator tmp = *this;
   this->operator++();
   return tmp;
}

FlatPatch::Iterator& FlatPatch::Iterator::operator--() noexcept
{
   TDD_ASSERT(nullptr != m_parent);

   if (m_pos == m_parent->m_patch.end()) {
      m_pos = m_parent->m_back.m_pos;
      return *this;
   }

   const difference_type offset = (*this)->previousBlockOffset;
   TDD_ASSERT(std::distance(m_pos, m_parent->m_patch.begin()) <= offset);

   std::advance(m_pos, -offset);
   return *this;
}

FlatPatch::Iterator FlatPatch::Iterator::operator--(int) noexcept
{
   Iterator tmp = *this;
   this->operator--();
   return tmp;
}

FlatPatch::Iterator::Iterator(
   FlatPatch const* parent,
   FlattenedPatches::const_iterator pos)
   : m_parent(parent)
   , m_pos(pos)
{}


FlatPatch::FlatPatch(const PatchDescriptor descriptor)
   : m_validationData(descriptor.GetValidationData())
   , m_patch()
   , m_back()
   , m_lastRead()
{
   static constexpr size_t kInitialAllowancePerPatch = 8;
   
   TDD_ASSERT(!descriptor.GetFullPatch().empty());

   m_patch.reserve(
      kInitialAllowancePerPatch * descriptor.GetFullPatch().size());

   std::vector<uint64_t> blockBuffer;
   size_t previousBlockOffset = 0;

   const auto& patches = descriptor.GetFullPatch();

   for (const auto& p : patches) {
      const auto requiredSize = GetFlattenedSize(p);
      blockBuffer.clear();
      blockBuffer.resize(requiredSize);

      auto* block = reinterpret_cast<PatchBlock*>(blockBuffer.data());
      block->address = p.address;
      block->patchLength = p.data.size();
      block->previousBlockOffset = previousBlockOffset;
      memcpy_s(
         block->patch,
         (requiredSize - kFlattenedHeader) * sizeof(uint64_t),
         p.data.data(),
         p.data.size());

      previousBlockOffset = requiredSize;

      m_patch.insert(m_patch.end(), blockBuffer.begin(), blockBuffer.end());
   }

   m_lastRead = begin();
   m_back = Iterator(this, m_patch.end() - previousBlockOffset);
}

FlatPatch::FlatPatch(const FlatPatch& other)
   : FlatPatch()
{
   *this = other;
}

FlatPatch& FlatPatch::operator=(const FlatPatch& other) noexcept
{
   if (this == &other) {
      return *this;
   }

   m_validationData = other.m_validationData;
   m_patch = other.m_patch;
   const auto lastBlockSize = GetBlockSize(other.m_back.operator->());
   m_back = Iterator(this, m_patch.end() - lastBlockSize);
   m_lastRead = begin();
   std::advance(m_lastRead, std::distance(other.begin(), other.m_lastRead));

   return *this;
}

FlatPatch::FlatPatch(FlatPatch&& other)
   : FlatPatch()
{
   *this = std::move(other);
}

FlatPatch& FlatPatch::operator=(FlatPatch&& other) noexcept
{
   if (this == &other) {
      return *this;
   }

   m_validationData = std::move(other.m_validationData);
   m_patch = std::move(other.m_patch);
   m_back = other.m_back;
   m_back.m_parent = this;
   m_lastRead = other.m_lastRead;
   m_lastRead.m_parent = this;
   return *this;
}

FlatPatch::iterator FlatPatch::begin() const noexcept
{
   return Iterator(this, m_patch.begin());
}

FlatPatch::iterator FlatPatch::end() const noexcept
{
   return Iterator(this, m_patch.end());
}

bool FlatPatch::empty() const noexcept
{
   return m_patch.empty();
}

std::optional<IPatcher::AdditionalReads> FlatPatch::Patch(
   const uint64_t addr,
   std::span<uint8_t> buffer)
{
   const auto endAddr = addr + buffer.size_bytes();

   if (!IsInRange(addr, endAddr)) {
      return std::nullopt;
   }

   if (addr < m_lastRead->address) {
      if (!SeekBackward(addr, endAddr)) {
         return std::nullopt;
      }
   }
   else if (m_lastRead->address < addr) {
      if (!SeekForward(addr, endAddr)) {
         return std::nullopt;
      }
   }

   // patch data
   while (m_lastRead != end() && m_lastRead->address < endAddr) {
      const auto& patch = *m_lastRead;

      TDD_ASSERT(patch.address + patch.patchLength >= addr);

      if (patch.address <= addr) {
         // Target start in the middle of patch
         const size_t skip = addr - patch.address;
         const auto copySize = std::min(buffer.size_bytes(), patch.patchLength - skip);
         memcpy_s(
            buffer.data(),
            copySize,
            reinterpret_cast<const char*>(patch.patch) + skip,
            copySize);
      }
      else {
         const auto offset = patch.address - addr;
         const auto availableBufferSize = buffer.size_bytes() - offset;
         const auto copySize = std::min(availableBufferSize, patch.patchLength);
         memcpy_s(
            &buffer[offset],
            copySize,
            patch.patch,
            copySize);
      }

      ++m_lastRead;
   }

   --m_lastRead;
   return std::nullopt;
}

bool FlatPatch::IsInRange(
   const uint64_t targetAddrBegin,
   const uint64_t targetAddrEnd) const
{
   if (targetAddrEnd <= begin()->address) {
      return false;
   }

   if (targetAddrBegin >= m_back->address + m_back->patchLength) {
      return false;
   }

   return true;
}

bool FlatPatch::SeekForward(
   const uint64_t targetAddrBegin,
   const uint64_t targetAddrEnd) const
{
   // targetAddrBegin is later than m_lastRead->address

   while (m_lastRead != end()) {
      const auto patchEnd = m_lastRead->address + m_lastRead->patchLength;

      if (m_lastRead->address <= targetAddrBegin) {
         if (targetAddrBegin < patchEnd) {
            // overlaps the beginning region of the target range.
            return true;
         }
      }
      else if (targetAddrBegin < m_lastRead->address) {
         if (m_lastRead->address < targetAddrEnd) {
            // This block starts in the middle of the target region.
            return true;
         }
         else {
            // Start of this block is after the target region.
            return false;
         }
      }

      ++m_lastRead;
   }

   if (m_lastRead == end()) {
      m_lastRead = m_back;
   }
   return false;
}

bool FlatPatch::SeekBackward(
   const uint64_t targetAddr,
   const uint64_t targetAddrEnd) const
{
   do {
      --m_lastRead;
      if (targetAddr > m_lastRead->address) {
         break;
      }

   }
   while (m_lastRead != begin());
   return SeekForward(targetAddr, targetAddrEnd);
}

}