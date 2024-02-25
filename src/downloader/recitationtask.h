#ifndef RECITATIONTASK_H
#define RECITATIONTASK_H

#include <database/qurandb.h>
#include <interfaces/downloadtask.h>
#include <types/reciter.h>
#include <utils/dirmanager.h>

class RecitationTask : public DownloadTask
{
public:
  RecitationTask();
  RecitationTask(int reciter, int surah, int verse);
  RecitationTask(const RecitationTask& other);
  RecitationTask(const RecitationTask&& other);
  ~RecitationTask();

  RecitationTask& operator=(RecitationTask other);

  QUrl url() const override;
  QFileInfo destination() const override;

private:
  QSharedPointer<QuranDb> m_quranDb = QuranDb::current();
  QSharedPointer<const QDir> m_downloadsDir = DirManager::downloadsDir;
  const QList<QSharedPointer<Reciter>>& m_reciters = Reciter::reciters;
  int m_reciter;
  int m_surah;
  int m_verse;
};

#endif // RECITATIONTASK_H
