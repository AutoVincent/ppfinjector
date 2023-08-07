#include <ppftk/rom_patch/patch_descriptor.h>

#include <ppftk/rom_patch/flat_patch.h>

#include <ppfbase/logging/logging.h>

namespace tdd::tk::rompatch {

FlatPatch PatchDescriptor::Flatten() const
{
   // TODO: implement
   TDD_ASSERT(false);
   return {};
}

void PatchDescriptor::AddValidationData(
   const size_t address,
   const DataBuffer& data)
{
   AddValidationData(address, DataBuffer(data));
}

void PatchDescriptor::AddValidationData(
   const size_t address,
   DataBuffer&& data) noexcept
{
   m_validationData.address = address;
   m_validationData.data = std::move(data);
}

bool PatchDescriptor::AddPatchData(
   const size_t address,
   const DataBuffer& data)
{
   return AddPatchData(address, DataBuffer(data));
}

bool PatchDescriptor::AddPatchData(
   const size_t address,
   DataBuffer&& data)
{
   const auto [it, inserted] = m_fullPatch.emplace(address, std::move(data));
   if (!inserted) {
      TDD_LOG_ERROR() << "Patch data for address [" << address
         << "] already exists.";
   }
   return inserted;
}

const PatchDescriptor::ValidationData&
PatchDescriptor::GetValidationData() const noexcept
{
   return m_validationData;
}

const PatchDescriptor::FullPatch&
PatchDescriptor::GetFullPatch() const noexcept
{
   return m_fullPatch;
}

}