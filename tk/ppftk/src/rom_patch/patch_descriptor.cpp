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

void PatchDescriptor::SetFullPatch(FullPatch&& patch) noexcept
{
   m_fullPatch = std::move(patch);
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

void PatchDescriptor::AddFileId(std::string_view info)
{
   AddFileId(std::string(info));
}

void PatchDescriptor::AddFileId(std::string&& info)
{
   m_fileId = std::move(info);
}

void PatchDescriptor::AddDescription(std::string_view description)
{
   AddDescription(std::string(description));
}

void PatchDescriptor::AddDescription(std::string&& description)
{
   m_description = std::move(description);
}

const PatchDescriptor::FullPatch&
PatchDescriptor::GetFullPatch() const noexcept
{
   return m_fullPatch;
}

const PatchDescriptor::ValidationData&
PatchDescriptor::GetValidationData() const noexcept
{
   return m_validationData;
}

const std::string& PatchDescriptor::GetFileId() const noexcept
{
   return m_fileId;
}

const std::string& PatchDescriptor::GetDescription() const noexcept
{
   return m_description;
}

}