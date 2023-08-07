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

   [[nodiscard]] ptrdiff_t GetPreviousBlockOffset(
      FlatPatch::PatchBlock const* block)
   {
      TDD_ASSERT(0 == block->previousBlockOffset % sizeof(uint64_t));

      return -static_cast<ptrdiff_t>(block->previousBlockOffset / sizeof(uint64_t));
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

   const auto offset = GetPreviousBlockOffset(this->operator->());
   TDD_ASSERT(std::distance(m_pos, m_parent->m_patch.begin()) <= offset);

   std::advance(m_pos, offset);
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

FlatPatch::iterator FlatPatch::begin() const noexcept
{
   return Iterator(this, m_patch.begin());
}

FlatPatch::iterator FlatPatch::end() const noexcept
{
   return Iterator(this, m_patch.end());
}

void FlatPatch::Patch(
   const uint64_t addr,
   const uint64_t size,
   char* buffer) const noexcept
{
   const auto endAddr = addr + size;

   if (!IsInRange(addr, endAddr)) {
      return;
   }

   if (addr < m_lastRead->address) {
      if (!SeekBackward(addr, endAddr)) {
         return;
      }
   }
   else if (addr > m_lastRead->address) {
      if (!SeekForward(addr, endAddr)) {
         return;
      }
   }

   // patch data
   if (m_lastRead->address < addr) {
      // Target buffer starts in the middle of a patch.
      const auto offset = m_lastRead->address - addr;
      memcpy_s(
         buffer,
         size,
         reinterpret_cast<const uint8_t*>(m_lastRead->patch) + offset,
         m_lastRead->patchLength - offset);
      ++m_lastRead;
   }

   while (m_lastRead != end() && m_lastRead->address < endAddr) {
      const auto offset = m_lastRead->address - addr;
      const auto availableBufferSize = size - offset;
      memcpy_s(
         buffer + offset,
         availableBufferSize,
         m_lastRead->patch,
         m_lastRead->patchLength);

      ++m_lastRead;
   }

   --m_lastRead;
}

bool FlatPatch::IsInRange(
   const uint64_t targetAddrBegin,
   const uint64_t targetAddrEnd) const
{
   if (targetAddrEnd < begin()->address) {
      return false;
   }

   if (targetAddrBegin > m_back->address + m_back->patchLength) {
      return false;
   }

   return true;
}

bool FlatPatch::SeekForward(
   const uint64_t targetAddrBegin,
   const uint64_t targetAddrEnd) const
{
   while (m_lastRead != end()) {
      if (m_lastRead->address >= targetAddrEnd) {
         return false;
      }

      if (m_lastRead->address + m_lastRead->patchLength > targetAddrBegin) {
         return true;
      }

      if (targetAddrEnd > m_lastRead->address) {
         return true;
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