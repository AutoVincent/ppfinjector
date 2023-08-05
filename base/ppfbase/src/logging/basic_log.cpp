#include "basic_log.h"

#include <ppfbase/chrono/timestamp.h>
#include <ppfbase/diagnostics/assert.h>

#include <set>

namespace tdd::base::logging::details {

namespace fs = std::filesystem;

namespace {
   // TODO: Move to an archivist thing
   std::set<fs::path> EnumerateArchive(const fs::path& logFile)
   {
      const auto logStem = logFile.stem();
      std::set<fs::path> archive;

      for (auto& it : fs::directory_iterator(logFile.parent_path())) {
         if (it.is_directory()) {
            continue;
         }

         if (it.path().extension() != ILog::kExt) {
            continue;
         }

         // log archive file has the name of <logStem>.<timestamp>.log
         const auto archiveStem = it.path().stem();
         const auto baseStem = archiveStem.stem();
         const bool isOldLog = baseStem == logStem && baseStem != archiveStem;
         if (isOldLog) {
            archive.insert(it.path());
         }
      }
      return archive;
   }
}

BasicLog::BasicLog(
   const std::filesystem::path& filepath,
   size_t maxBytesPerFile,
   size_t numberOfFilesToKeep)
: m_file(filepath)
, m_maxBytesPerFile(maxBytesPerFile)
, m_archiveSize(numberOfFilesToKeep - 1)
, m_os()
{
   TDD_ASSERT(filepath.is_absolute());

   const auto exceptMask = m_os.exceptions();
   m_os.exceptions(std::ios::failbit | std::ios::badbit);
   m_os.open(m_file, std::fstream::app);
   m_os.exceptions(exceptMask);
}

BasicLog::~BasicLog()
{
   m_os.close();
}

void BasicLog::Write(const char* msg)
{
   m_os << msg << std::flush;
   RotateAsRequired();
}

void BasicLog::RotateAsRequired()
{
   if (m_file.file_size() < m_maxBytesPerFile) {
      return;
   }

   m_os.close();
   auto archiveName = m_file.path().parent_path();
   archiveName /= m_file.path().stem();
   archiveName += chrono::TimeStamp::FilenameSuffix();
   archiveName += ILog::kExt;
   fs::rename(m_file, archiveName);
   m_os.open(m_file, std::fstream::app);

   // TODO: move to log archivist
   // Prune old logs
   auto archive = EnumerateArchive(m_file);
   while (archive.size() > m_archiveSize) {
      fs::remove(*archive.begin());
      archive.erase(archive.begin());
   }
}

}