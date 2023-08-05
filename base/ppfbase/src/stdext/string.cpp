#include <ppfbase/stdext/string.h>

#include <Windows.h>

namespace tdd::stdext {
   
std::string WideToUtf8(const std::wstring& src)
{
   if (src.empty()) {
      return "";
   }

   const auto srcLen = static_cast<int>(src.size());
   auto charCount = ::WideCharToMultiByte(
      CP_UTF8,
      0,
      src.c_str(),
      srcLen,
      nullptr,
      0,
      nullptr,
      nullptr);
   if (0 == charCount) {
      return "";
   }

   std::string utf8(charCount, 0);
   charCount = WideCharToMultiByte(
      CP_UTF8,
      0,
      src.c_str(),
      srcLen,
      utf8.data(),
      charCount,
      nullptr,
      nullptr);
   if (charCount > 0) {
      return utf8;
   }
   return "";
}

std::wstring Utf8ToWide(const std::string& src)
{
   if (src.empty()) {
      return L"";
   }

   const auto srcLen = static_cast<int>(src.size());
   auto charCount = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), srcLen, nullptr, 0);
   if (0 == charCount) {
      return L"";
   }

   std::wstring wide(charCount, 0);
   charCount = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), srcLen, wide.data(), charCount);
   if (charCount > 0) {
      return wide;
   }
   return L"";
}

}