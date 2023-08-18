#include <ppfbase/stdext/mutex.h>

#include <ppfbase/diagnostics/assert.h>

#include <Windows.h>

namespace tdd::stdext {

namespace {
   [[nodiscard]] HANDLE MakeMutex(const wchar_t* name) noexcept
   {
      static constexpr LPSECURITY_ATTRIBUTES kDefaultSecurity = nullptr;
      static constexpr BOOL kNotTakingOwnership = FALSE;

      return ::CreateMutexW(kDefaultSecurity, kNotTakingOwnership, name);
   }
}

mp_mutex::mp_mutex(std::wstring_view name)
   : mp_mutex(std::wstring(name))
{}

mp_mutex::mp_mutex(std::wstring&& name)
   : m_name(std::move(name))
   , m_hMutex(MakeMutex(m_name.c_str()))
{
   if (NULL == m_hMutex) {
      throw std::runtime_error(make_last_error().message());
   }
}

mp_mutex::~mp_mutex() noexcept
{
   CloseHandle(m_hMutex);
}

void mp_mutex::lock()
{
   ::WaitForSingleObject(m_hMutex, INFINITE);
}

bool mp_mutex::try_lock()
{
   return WAIT_OBJECT_0 == ::WaitForSingleObject(m_hMutex, 0);
}

void mp_mutex::unlock()
{
   TDD_ASSERT(::ReleaseMutex(m_hMutex));
}

mp_mutex::native_handle_type mp_mutex::native_handle() const noexcept
{
   return m_hMutex;
}

std::wstring_view mp_mutex::name() const noexcept
{
   return m_name;
}

}