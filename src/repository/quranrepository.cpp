#include "quranrepository.h"
#include <QRandomGenerator>
#include <QSqlError>

QuranRepository&
QuranRepository::getInstance()
{
  static QuranRepository qdb;
  return qdb;
}

QuranRepository::QuranRepository()
  : QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "QuranCon"))
  , m_assetsDir(DirManager::getInstance().assetsDir())
  , m_config(Configuration::getInstance())
{
  QuranRepository::open();
  for (int i = 1; i <= 114; i++)
    m_surahNames.append(surahName(i));
}

void
QuranRepository::open()
{
  setDatabaseName(m_assetsDir.absoluteFilePath("quran.db"));
  if (!QSqlDatabase::open())
    qFatal("Error opening quran db");
}

DbConnection::Type
QuranRepository::type()
{
  return DbConnection::Quran;
}

QPair<int, int>
QuranRepository::pageMetadata(const int page) const
{
  QSqlQuery dbQuery(*this);
  dbQuery.prepare(
    "SELECT sura_no,jozz FROM verses_v1 WHERE page=:p ORDER BY id");
  dbQuery.bindValue(0, page);

  if (!dbQuery.exec())
    qCritical() << "Error occurred during getPageMetadata SQL statment exec";

  dbQuery.next();
  // { surahIdx, jozz }
  return { dbQuery.value(0).toInt(), dbQuery.value(1).toInt() };
}

int
QuranRepository::getVersePage(const int& surahIdx, const int& verse) const
{
  QSqlQuery dbQuery(*this);

  QString query = "SELECT page FROM verses_v%0 WHERE sura_no=%1 AND aya_no=%2";
  dbQuery.prepare(query.arg(QString::number(m_config.qcfVersion()),
                            QString::number(surahIdx),
                            QString::number(verse)));

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getVersePage SQL statment exec";
  }
  dbQuery.next();

  return dbQuery.value(0).toInt();
}

int
QuranRepository::getJuzStartPage(const int juz) const
{
  QSqlQuery dbQuery(*this);

  QString query =
    "SELECT page FROM verses_v1 WHERE jozz=" + QString::number(juz) +
    " ORDER BY id";
  dbQuery.prepare(query);

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getJuzStartPage SQL statment exec";
  }
  dbQuery.next();

  return dbQuery.value(0).toInt();
}

int
QuranRepository::getJuzOfPage(const int page) const
{
  QSqlQuery dbQuery(*this);

  QString query =
    "SELECT jozz FROM verses_v1 WHERE page=" + QString::number(page);
  dbQuery.prepare(query);

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getJuzOfPage SQL statment exec";
  }
  dbQuery.next();

  return dbQuery.value(0).toInt();
}

QList<Verse>
QuranRepository::verseInfoList(const int page) const
{
  QList<Verse> viList;
  QSqlQuery dbQuery(*this);

  QString query =
    "SELECT sura_no,aya_no FROM verses_v%0 WHERE page=%1 ORDER BY id";
  dbQuery.prepare(
    query.arg(QString::number(m_config.qcfVersion()), QString::number(page)));

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getVerseInfoList SQL statment exec";
  }

  while (dbQuery.next()) {
    viList.append(
      Verse(page, dbQuery.value(0).toInt(), dbQuery.value(1).toInt()));
  }

  return viList;
}

QString
QuranRepository::verseText(const int sIdx, const int vIdx) const
{
  QSqlQuery dbQuery(*this);
  if (m_config.verseType() == Configuration::Annotated)
    dbQuery.prepare("SELECT aya_text_annotated FROM verses_v1 WHERE sura_no=:s "
                    "AND aya_no=:v");
  else
    dbQuery.prepare(
      "SELECT aya_text FROM verses_v1 WHERE sura_no=:s AND aya_no=:v");

  dbQuery.bindValue(0, sIdx);
  dbQuery.bindValue(1, vIdx);

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getVerseText SQL statment exec";
  }
  dbQuery.next();

  return dbQuery.value(0).toString();
}

int
QuranRepository::surahStartPage(int surahIdx) const
{
  QSqlQuery dbQuery(*this);

  dbQuery.prepare("SELECT page FROM verses_v1 WHERE sura_no=:sn AND aya_no=1");
  dbQuery.bindValue(0, surahIdx);

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getSurahStartPage SQL statment exec";
  }
  dbQuery.next();

  return dbQuery.value(0).toInt();
}

QString
QuranRepository::surahName(const int sIdx, bool ar) const
{
  QSqlQuery dbQuery(*this);

  if (m_config.language() == QLocale::Arabic || ar)
    dbQuery.prepare("SELECT sura_name_ar FROM verses_v1 WHERE sura_no=:i");
  else
    dbQuery.prepare("SELECT sura_name_en FROM verses_v1 WHERE sura_no=:i");

  dbQuery.bindValue(0, sIdx);
  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getSurahName SQL statment exec";
  }

  dbQuery.next();
  return dbQuery.value(0).toString();
}

Verse
QuranRepository::verseById(const int id) const
{
  QSqlQuery dbQuery(*this);
  dbQuery.prepare("SELECT page,sura_no,aya_no FROM verses_v1 WHERE id=:i");
  dbQuery.bindValue(0, id);

  if (!dbQuery.exec())
    qCritical() << "Error occurred during getVerseById SQL statement exec";

  dbQuery.next();

  return Verse(dbQuery.value(0).toInt(),
               dbQuery.value(1).toInt(),
               dbQuery.value(2).toInt());
}

int
QuranRepository::versePage(const int& surahIdx, const int& verse) const
{
  QSqlQuery dbQuery(*this);

  QString query = "SELECT page FROM verses_v%0 WHERE sura_no=%1 AND aya_no=%2";
  dbQuery.prepare(query.arg(QString::number(m_config.qcfVersion()),
                            QString::number(surahIdx),
                            QString::number(verse)));

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during getVersePage SQL statment exec";
  }
  dbQuery.next();

  return dbQuery.value(0).toInt();
}

QList<int>
QuranRepository::searchSurahNames(QString text) const
{
  QList<int> results;
  QSqlQuery dbQuery(*this);
  QString q =
    "SELECT DISTINCT sura_no FROM verses_v1 WHERE (sura_name_ar like '%" +
    text +
    "%' OR "
    "sura_name_en like '%" +
    text + "%')";

  dbQuery.prepare(q);
  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during searchSurahNames SQL statment exec";
  }

  while (dbQuery.next()) {
    results.append(dbQuery.value(0).toInt());
  }

  return results;
}

QList<Verse>
QuranRepository::searchSurahs(QString searchText,
                              const QList<int> surahs,
                              const bool whole) const
{
  QList<Verse> results;
  QSqlQuery dbQuery(*this);

  QString q = "SELECT page,sura_no,aya_no FROM verses_v" +
              QString::number(m_config.qcfVersion()) + " WHERE (";
  for (int i = 0; i < surahs.size(); i++) {
    q.append("sura_no=" + QString::number(surahs.at(i)) + ' ');
    if (i != surahs.size() - 1)
      q.append("OR ");
  }

  if (whole)
    q.append(") AND (aya_text_emlaey like '" + searchText +
             " %' OR aya_text_emlaey like '% " + searchText +
             " %') ORDER BY id");
  else
    q.append(") AND (aya_text_emlaey like '%" + searchText + "%') ORDER BY id");

  dbQuery.prepare(q);
  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during searchSurahs SQL statment exec";
  }

  while (dbQuery.next()) {
    results.append(Verse(dbQuery.value(0).toInt(),
                         dbQuery.value(1).toInt(),
                         dbQuery.value(2).toInt()));
  }

  return results;
}

QList<Verse>
QuranRepository::searchVerses(QString searchText,
                              const int range[],
                              const bool whole) const
{
  QList<Verse> results;
  QSqlQuery dbQuery(*this);

  QString q = "SELECT page,sura_no,aya_no FROM verses_v" +
              QString::number(m_config.qcfVersion()) +
              " WHERE (page >= " + QString::number(range[0]) +
              " AND page <= " + QString::number(range[1]) + ")";

  if (whole)
    q.append(" AND (aya_text_emlaey like '" + searchText +
             " %' OR aya_text_emlaey like '% " + searchText +
             " %') ORDER BY id");
  else
    q.append(" AND (aya_text_emlaey like '%" + searchText + "%') ORDER BY id");

  dbQuery.prepare(q);
  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during searchVerses SQL statment exec";
  }

  while (dbQuery.next()) {
    results.append(Verse(dbQuery.value(0).toInt(),
                         dbQuery.value(1).toInt(),
                         dbQuery.value(2).toInt()));
  }

  return results;
}

Verse
QuranRepository::randomVerse() const
{
  QSqlQuery dbQuery(*this);

  int id = QRandomGenerator::global()->bounded(1, 6237);
  dbQuery.prepare("SELECT page,sura_no,aya_no FROM verses_v" +
                  QString::number(m_config.qcfVersion()) +
                  " WHERE id=" + QString::number(id));

  if (!dbQuery.exec()) {
    qCritical() << "Error occurred during randomVerse SQL statment exec";
  }
  dbQuery.next();
  return Verse(dbQuery.value(0).toInt(),
               dbQuery.value(1).toInt(),
               dbQuery.value(2).toInt());
}

QStringList
QuranRepository::surahNames() const
{
  return m_surahNames;
}

Verse
QuranRepository::next(const Verse& verse, bool withBasmallah) const
{
  Verse nxt(verse);
  if (!nxt.number()) {
    nxt.setNumber(1);
    return nxt;
  }

  nxt = verseById(nxt.id(nxt.surah(), nxt.number()) + 1);

  if (withBasmallah && nxt.number() == 1 && nxt.surah() != 9 &&
      nxt.surah() != 1)
    nxt.setNumber(0);

  return nxt;
}

Verse
QuranRepository::previous(const Verse& verse, bool withBasmallah) const
{
  Verse nxt(verse);
  if (withBasmallah && nxt.number() == 1 && nxt.surah() != 9 &&
      nxt.surah() != 1) {
    nxt.setNumber(0);
    return nxt;
  }

  if (!nxt.number())
    nxt.setNumber(1);

  nxt = verseById(nxt.id(nxt.surah(), nxt.number()) - 1);
  return nxt;
}
