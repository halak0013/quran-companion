#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../utils/dbmanager.h"
#include "../utils/notificationmanager.h"
#include "../utils/verseplayer.h"
#include "../widgets/clickablelabel.h"
#include "../widgets/quranpagebrowser.h"
#include "../widgets/verseframe.h"
#include "bookmarksdialog.h"
#include "downloaderdialog.h"
#include "searchdialog.h"
#include "settingsdialog.h"
#include <QBoxLayout>
#include <QClipboard>
#include <QDesktopServices>
#include <QIntValidator>
#include <QMainWindow>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>

typedef DBManager::Verse Verse;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/*!
 * \class MainWindow
 *
 * \brief The MainWindow class is responsible for the reader interface, audio
 * controls, navigation, and opening other dialogs
 */
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr, QSettings* settingsPtr = nullptr);
  void setupConnections();
  void highlightCurrentVerse();
  enum SideContent
  {
    tafsir,
    translation
  };
  ~MainWindow();

public slots:
  void showVOTDmessage(Verse v, QString msg);
  void visitWebsite();
  void checkForUpdates();
  void updateProcessCallback();
  void saveReaderState();
  void restartApp();

private slots:
  void nextPage();
  void prevPage();

  void btnPlayClicked();
  void btnPauseClicked();
  void btnStopClicked();

  void cmbPageChanged(int newPageIdx);
  void cmbSurahChanged(int newSurahIdx);
  void cmbVerseChanged(int newVerseIdx);
  void updateVerseDropDown();
  void gotoPage(int page, bool automaticFlip = false);
  void gotoSurah(int surahIdx);

  void activeVerseChanged();
  void missingRecitationFileWarn(int reciterIdx, int surah);
  void mediaPosChanged(qint64 position);
  void mediaStateChanged(QMediaPlayer::PlaybackState state);

  void spaceKeyPressed();
  void actionPrefTriggered();
  void actionDMTriggered();
  void actionBookmarksTriggered();
  void openSearchDialog();

  void verseClicked();
  void showExpandedVerseTafsir();
  void navigateToVerse(Verse v);
  void verseAnchorClicked(const QUrl& hrefUrl);
  void copyVerseText(int IdxInPage);

  void redrawQuranPage(bool manualSz = false);
  void addSideContent();

  void updateSideContentType();
  void updateLoadedTafsir();
  void updateLoadedTranslation();
  void updateSideFont();

private:
  void loadIcons();
  void init();
  void updateSurah();
  void updatePageVerseInfoList();
  void setVerseToStartOfPage();
  void setCmbPageIdx(int idx);
  void setCmbSurahIdx(int idx);
  void setCmbVerseIdx(int idx);
  bool m_endOfPage = false;
  bool m_internalPageChange = false;
  bool m_internalSurahChange = false;
  bool m_internalVerseChange = false;
  bool m_darkMode = false;
  Ui::MainWindow* ui;
  QString m_iconsPath;
  QProcess* m_process;
  QString m_updateToolPath;
  QIntValidator* m_verseValidator = nullptr;
  QuranPageBrowser* m_quranBrowser;
  NotificationManager* m_notifyMgr;
  DBManager* m_dbMgr;
  VersePlayer* m_player;
  SearchDialog* m_searchDlg = nullptr;
  SettingsDialog* m_settingsDlg = nullptr;
  BookmarksDialog* m_bookmarksDlg = nullptr;
  DownloaderDialog* m_downloaderDlg = nullptr;
  DownloadManager* m_downManPtr = nullptr;
  VerseFrame* m_highlightedFrm = nullptr;
  QSettings* m_settingsPtr;
  Verse m_currVerse{ 1, 1, 1 };
  SideContent m_sideContent;
  QDir m_assetsDir = QDir::currentPath() + QDir::separator() + "assets";
  QFont m_sideFont;
  QList<Verse> m_vInfoList;
  QList<QFrame*> m_verseFrameList;
};
#endif // MAINWINDOW_H
