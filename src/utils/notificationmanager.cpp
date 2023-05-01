
#include "notificationmanager.h"

NotificationManager::NotificationManager(QObject* parent, DBManager* dbPtr)
  : QObject{ parent }
  , m_dbMgr{ dbPtr }
  , m_dtNow{ QDateTime::currentDateTime() }
  , m_sysTray{ new QSystemTrayIcon(this) }
  , m_trayMenu{ new QMenu() }
{
  QAction* togglePlay = new QAction(tr("Play/Pause recitation"), m_trayMenu);
  QAction* show = new QAction(tr("Show window"), m_trayMenu);
  QAction* hide = new QAction(tr("Hide window"), m_trayMenu);
  QAction* update = new QAction(tr("Check for updates"), m_trayMenu);
  QAction* website = new QAction(tr("Website"), m_trayMenu);
  QAction* exit = new QAction(tr("Exit"), m_trayMenu);
  m_trayMenu->addAction(togglePlay);
  m_trayMenu->addSeparator();
  m_trayMenu->addAction(show);
  m_trayMenu->addAction(hide);
  m_trayMenu->addSeparator();
  m_trayMenu->addAction(update);
  m_trayMenu->addAction(website);
  m_trayMenu->addSeparator();
  m_trayMenu->addAction(exit);

  connect(togglePlay,
          &QAction::triggered,
          this,
          &NotificationManager::togglePlayback,
          Qt::UniqueConnection);
  connect(show,
          &QAction::triggered,
          this,
          &NotificationManager::showWindow,
          Qt::UniqueConnection);
  connect(hide,
          &QAction::triggered,
          this,
          &NotificationManager::hideWindow,
          Qt::UniqueConnection);
  connect(exit,
          &QAction::triggered,
          this,
          &NotificationManager::exit,
          Qt::UniqueConnection);
  connect(update,
          &QAction::triggered,
          this,
          &NotificationManager::checkForUpdates,
          Qt::UniqueConnection);
  connect(website,
          &QAction::triggered,
          this,
          &NotificationManager::openWebsite,
          Qt::UniqueConnection);

  m_sysTray->setContextMenu(m_trayMenu);
  m_sysTray->setIcon(QIcon(":/images/tray.png"));
  m_sysTray->show();
}

void
NotificationManager::notify(QString title, QString msg)
{
  m_sysTray->showMessage(title, msg);
}

void
NotificationManager::checkDailyVerse()
{

  QDateTime lastTimestamp;
  QFile timestamp = QDir::currentPath() + QDir::separator() + "ts";
  if (!timestamp.exists()) {
    if (!timestamp.open(QIODevice::WriteOnly)) {
      qWarning() << "Couldn't open timestamp file for daily notification check";
      return;
    }
    timestamp.write(m_dtNow.toString(Qt::DateFormat::ISODate).toLatin1());
    timestamp.close();
    showVerseOfTheDay();

  } else {
    if (!timestamp.open(QIODevice::ReadWrite)) {
      qWarning() << "Couldn't open timestamp file for daily notification check";
      return;
    }
    lastTimestamp =
      QDateTime::fromString(timestamp.readAll(), Qt::DateFormat::ISODate);
    if (lastTimestamp.daysTo(m_dtNow) > 0) {
      showVerseOfTheDay();
      timestamp.seek(0);
      timestamp.write(m_dtNow.toString(Qt::ISODate).toLatin1());
    }
    timestamp.close();
  }
}
void
NotificationManager::showVerseOfTheDay()
{
  QString msg;
  QPair<Verse, QString> votd = m_dbMgr->randomVerse();

  votd.second.truncate(votd.second.size() - 2);
  msg.append("«" + votd.second + "»");
  msg.append("<br>");
  msg.append(m_dbMgr->getTranslation(votd.first.surah, votd.first.number));
  msg.append("<br>");
  msg.append(tr("Surah: ") + m_dbMgr->getSurahName(votd.first.surah) + " - " +
             tr("Verse: ") + QString::number(votd.first.number));

  emit showVOTDmessagebox(votd.first, msg);
}
