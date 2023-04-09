#include "mainwindow.h"
#include "ui_mainwindow.h"

/*!
 * \brief MainWindow::MainWindow initalizes the main application window and sets up UI connections
 * \param parent is a pointer to the parent widget
 * \param settingsPtr is a pointer to the QSettings object to acess app settings
 */
MainWindow::MainWindow(QWidget *parent, QSettings *settingsPtr)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settingsPtr{settingsPtr}
{
    ui->setupUi(this);
    ui->cmbPage->setValidator(new QIntValidator(1, 604, this));

    init();

    if (m_settingsPtr->value("WindowState").isNull())
        m_settingsPtr->setValue("WindowState", saveState());
    else
        restoreState(m_settingsPtr->value("WindowState").toByteArray());

    setupConnections();
}

/*!
 * \brief MainWindow::init initalizes different parts used by the app, such as the quran page widget, db manager, and the verse player objects
 */
void MainWindow::init()
{
    m_darkMode = m_settingsPtr->value("Theme").toInt() == 1;

    if (m_settingsPtr->value("Language").toString() == "العربية") {
        ui->frmCenteralCont->setLayoutDirection(Qt::LeftToRight);
        ui->retranslateUi(this);
    }

    setWindowTitle(QApplication::applicationName());

    m_settingsPtr->beginGroup("Reader");

    m_currVerse = {m_settingsPtr->value("Page").toInt(),
                   m_settingsPtr->value("Surah").toInt(),
                   m_settingsPtr->value("Verse").toInt()};

    if (m_settingsPtr->value("SideContent").isNull()) {
        m_settingsPtr->setValue("SideContent", (int) SideContent::translation);
        m_settingsPtr->setValue("Tafsir", (int) DBManager::Tafsir::muyassar);
        m_settingsPtr->setValue("Translation", (int) DBManager::Translation::en_sahih);
    }
    m_settingsPtr->endGroup();

    // initalization
    m_dbManPtr = new DBManager(this, m_settingsPtr->value("Reader/QCF").toInt());
    m_player = new VersePlayer(this,
                               m_dbManPtr,
                               m_currVerse,
                               m_settingsPtr->value("Reciter", 0).toInt());
    m_quranBrowser = new QuranPageBrowser(ui->frmPageContent,
                                          m_settingsPtr->value("Reader/QCF").toInt(),
                                          m_currVerse.page,
                                          m_dbManPtr,
                                          m_settingsPtr);
    ui->frmPageContent->layout()->addWidget(m_quranBrowser);

    updateSideContentType();
    updateLoadedTafsir();
    updateLoadedTranslation();
    updateSideFont();

    redrawQuranPage();
    updateVerseDropDown();

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setDirection(QBoxLayout::BottomToTop);
    ui->scrlVerseCont->setLayout(vbl);
    addSideContent();

    ui->menuView->addAction(ui->dockControls->toggleViewAction());

    for (int i = 1; i < 605; i++) {
        ui->cmbPage->addItem(QString::number(i));
    }

    foreach (Reciter r, m_player->recitersList()) {
        ui->cmbReciter->addItem(r.displayName);
    }

    m_internalSurahChange = true;
    m_internalVerseChange = true;
    ui->cmbSurah->setCurrentIndex(m_currVerse.surah - 1);
    ui->cmbVerse->setCurrentIndex(m_currVerse.number - 1);
    m_internalSurahChange = false;
    m_internalVerseChange = false;

    ui->cmbPage->setCurrentIndex(m_currVerse.page - 1);
    ui->cmbReciter->setCurrentIndex(m_settingsPtr->value("Reciter", 0).toInt());
}

/*!
 * \brief MainWindow::setupConnections connects different UI components with signals and slots
 */
void MainWindow::setupConnections()
{
    QShortcut *spaceKey = new QShortcut(Qt::Key_Space, this);

    /* ------------------ UI connectors ------------------ */

    // Menubar
    connect(ui->actionExit, &QAction::triggered, this, &QApplication::exit);
    connect(ui->actionPereferences, &QAction::triggered, this, &MainWindow::actionPrefTriggered);
    connect(ui->actionDownload_manager, &QAction::triggered, this, &MainWindow::actionDMTriggered);
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::openSearchDialog);

    // Quran page
    connect(m_quranBrowser, &QTextBrowser::anchorClicked, this, &MainWindow::verseAnchorClicked);
    connect(m_quranBrowser, &QuranPageBrowser::copyVerse, this, &MainWindow::copyVerseText);

    // page controls
    connect(ui->btnNext, &QPushButton::clicked, this, &MainWindow::nextPage);
    connect(ui->btnPrev, &QPushButton::clicked, this, &MainWindow::prevPage);
    connect(ui->cmbSurah, &QComboBox::currentIndexChanged, this, &MainWindow::cmbSurahChanged);
    connect(ui->cmbPage, &QComboBox::currentIndexChanged, this, &MainWindow::cmbPageChanged);
    connect(ui->cmbVerse, &QComboBox::currentIndexChanged, this, &MainWindow::cmbVerseChanged);
    connect(m_player, &VersePlayer::surahChanged, this, &MainWindow::updateSurah);
    connect(m_player, &VersePlayer::verseNoChanged, this, &MainWindow::activeVerseChanged);
    connect(m_player, &VersePlayer::missingVerseFile, this, &MainWindow::missingRecitationFileWarn);

    // audio slider
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::mediaPosChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::mediaStateChanged);
    connect(ui->sldrAudioPlayer, &QSlider::sliderMoved, m_player, &QMediaPlayer::setPosition);

    // player control
    connect(ui->btnPlay, &QPushButton::clicked, this, &MainWindow::btnPlayClicked);
    connect(ui->btnPause, &QPushButton::clicked, this, &MainWindow::btnPauseClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::btnStopClicked);
    connect(ui->cmbReciter, &QComboBox::currentIndexChanged, m_player, &VersePlayer::changeReciter);
    connect(spaceKey, &QShortcut::activated, this, &MainWindow::spaceKeyPressed);
    connect(ui->btnSearch, &QPushButton::clicked, this, &MainWindow::openSearchDialog);
    connect(ui->btnPreferences, &QPushButton::clicked, this, &MainWindow::actionPrefTriggered);
}

/* ------------------------ UI updating ------------------------ */

/*!
 * \brief MainWindow::updateSurah slot is called on surah change by the vcrse player to update the reader interface by navigating to that surah
 * works by firing off the cmbSurahChanged slot
 */
void MainWindow::updateSurah()
{
    ui->cmbSurah->setCurrentIndex(m_player->activeVerse().surah - 1);
}

/*!
 * \brief MainWindow::updatePageVerseInfoList updates the list that contains all verses in the current page each as a QMap of keys "surah" & "ayah" 
 */
void MainWindow::updatePageVerseInfoList()
{
    m_vInfoList = m_dbManPtr->getVerseInfoList(m_currVerse.page);
}

/*!
 * \brief MainWindow::updateVerseDropDown sets the verse combobox values according to the current surah verse count, sets the current verse visible
 */
void MainWindow::updateVerseDropDown()
{
    m_internalVerseChange = true;

    m_player->updateSurahVerseCount();

    if (verseValidator != nullptr)
        delete verseValidator;

    verseValidator = new QIntValidator(1, m_player->surahCount(), ui->cmbVerse);
    // updates values in the combobox with the current surah verses
    ui->cmbVerse->clear();
    for (int i = 1; i <= m_player->surahCount(); i++)
        ui->cmbVerse->addItem(QString::number(i), i);

    ui->cmbVerse->setValidator(verseValidator);
    ui->cmbVerse->setCurrentIndex(m_currVerse.number - 1);

    m_internalVerseChange = false;
}

/* ------------------------ Page navigation ------------------------ */

/*!
 * \brief MainWindow::gotoPage displays the given page and sets the current verse to the 1st verse in the page
 * \param page
 */
void MainWindow::gotoPage(int page)
{
    m_currVerse.page = page;
    redrawQuranPage();

    btnStopClicked(); // stop playback, set verse & surah in player to the page selected
    addSideContent();
}

/*!
 * \brief MainWindow::nextPage navigates to the next page relative to the current page
 */
void MainWindow::nextPage()
{
    bool keepPlaying = m_player->playbackState() == QMediaPlayer::PlayingState;
    if (m_currVerse.page < 604) {
        ui->cmbPage->setCurrentIndex(m_currVerse.page);

        // if the page is flipped automatically, resume playback
        if (keepPlaying)
            btnPlayClicked();
    }
}

/*!
 * \brief MainWindow::prevPage navigates to the previous page relative to the current page
 */
void MainWindow::prevPage()
{
    bool keepPlaying = m_player->playbackState() == QMediaPlayer::PlayingState;
    if (m_currVerse.page > 1) {
        ui->cmbPage->setCurrentIndex(m_currVerse.page - 2);

        if (keepPlaying)
            btnPlayClicked();
    }
}

/*!
 * \brief MainWindow::gotoSurah gets the page of the 1st verse in this surah, moves to that page, and starts playback of the surah
 * \param surahIdx surah number in the mushaf (1-114)
 */
void MainWindow::gotoSurah(int surahIdx)
{
    // getting surah index
    m_currVerse.page = m_dbManPtr->getSurahStartPage(surahIdx);
    m_currVerse.surah = ui->cmbSurah->currentIndex() + 1;
    m_currVerse.number = 1;

    // setting up the page of verse 1
    redrawQuranPage();
    addSideContent();

    // syncing the player & playing basmalah
    m_player->setVerse(m_currVerse);
    m_player->playBasmalah();

    m_internalPageChange = true;
    ui->cmbPage->setCurrentIndex(m_currVerse.page - 1);
    m_internalPageChange = false;

    m_player->updateSurahVerseCount();
    updateVerseDropDown();

    m_endOfPage = false;
}

/*!
 * \brief MainWindow::cmbPageChanged slot for updating the reader page as the user selects a different page from the combobox
 * \param newIdx the selected idx in the combobox (0-603)
 */
void MainWindow::cmbPageChanged(int newIdx)
{
    if (m_internalPageChange) {
        qDebug() << "Internal page change";
        return;
    }

    gotoPage(newIdx + 1);

}

/*!
 * \brief MainWindow::cmbSurahChanged slot for updating the reader page as the user selects a different surah
 * \param newSurahIdx surah idx in the combobox (0-113)
 */
void MainWindow::cmbSurahChanged(int newSurahIdx)
{
    if (m_internalSurahChange) {
        qDebug() << "Internal surah change";
        return;
    }

    m_internalPageChange = true;

    gotoSurah(newSurahIdx + 1);

    m_internalPageChange = false;
}

/*!
 * \brief MainWindow::cmbVerseChanged slot for updating the reader page as the user selects a different verse in the same surah
 * \param newVerseIdx verse idx in the combobox (0 -> (surahVerseCount-1))
 */
void MainWindow::cmbVerseChanged(int newVerseIdx)
{
    if (newVerseIdx < 0)
        return;

    if (m_internalVerseChange) {
        qDebug() << "internal verse change";
        return;
    }

    int verse = newVerseIdx + 1;
    m_currVerse.page = m_dbManPtr->getVersePage(m_currVerse.surah, verse);
    m_currVerse.number = verse;

    redrawQuranPage();

    // update the player surah & verse
    m_player->setVerse(m_currVerse);
    // open newly set verse recitation file
    m_player->setVerseFile(m_player->constructVerseFilename());

    m_internalPageChange = true;
    ui->cmbPage->setCurrentIndex(m_currVerse.page - 1);
    m_internalPageChange = false;

    addSideContent();
    m_endOfPage = false;
}

/* ------------------------ Player controls / highlighting ------------------------ */

void MainWindow::spaceKeyPressed()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        btnPauseClicked();
    } else {
        btnPlayClicked();
    }
}

/*!
 * \brief MainWindow::btnPlayClicked continues playback of the current verse
 */
void MainWindow::btnPlayClicked()
{
    // If now playing the last verse in the page, set the flag to flip the page
    if (m_currVerse.number == m_vInfoList.last().number
        && m_currVerse.number != m_player->surahCount()) {
        m_endOfPage = true;
    }

    highlightCurrentVerse();
    m_player->play();
}

void MainWindow::btnPauseClicked()
{
  m_player->pause();
}

/*!
 * \brief MainWindow::btnStopClicked stops playback, sets the current vers to the 1st in the page, updates comboboxes as it might be a different surah
 */
void MainWindow::btnStopClicked()
{
  m_player->stop();

  // set the current verse to the verse at the top of the page
  m_currVerse = m_vInfoList.at(0);
  // update the player surah & verse
  m_player->setVerse(m_currVerse);
  // open newly set verse recitation file
  m_player->setVerseFile(m_player->constructVerseFilename());

  m_internalSurahChange = true;
  updateSurah();
  updateVerseDropDown();
  m_internalSurahChange = false;

  m_endOfPage = false;
}

/*!
 * \brief MainWindow::mediaStateChanged disables/enables control buttons according to the media player state
 * \param state
 */
void MainWindow::mediaStateChanged(QMediaPlayer::PlaybackState state)
{
  if (state == QMediaPlayer::PlayingState) {
        ui->btnPlay->setEnabled(false);
        ui->btnPause->setEnabled(true);
        ui->btnStop->setEnabled(true);

  } else if (state == QMediaPlayer::PausedState) {
        ui->btnPlay->setEnabled(true);
        ui->btnPause->setEnabled(false);
        ui->btnStop->setEnabled(true);

  } else if (state == QMediaPlayer::StoppedState) {
        ui->btnPlay->setEnabled(true);
        ui->btnPause->setEnabled(false);
        ui->btnStop->setEnabled(false);
  }
}

/*!
 * \brief MainWindow::mediaPosChanged sets the current position in the audio file as the position of the slider
 * \param position 
 */
void MainWindow::mediaPosChanged(qint64 position)
{
  if (ui->sldrAudioPlayer->maximum() != m_player->duration())
        ui->sldrAudioPlayer->setMaximum(m_player->duration());

  if (!ui->sldrAudioPlayer->isSliderDown())
        ui->sldrAudioPlayer->setValue(position);
}

/*!
 * \brief MainWindow::missingRecitationFileWarn display warning message box in case that recitation files are missing
 */
void MainWindow::missingRecitationFileWarn()
{
  QMessageBox::StandardButton btn = QMessageBox::question(
      this,
      tr("Recitation not found"),
      tr("The recitation files for the current surah is missing, would you like to download it?"));

  if (btn == QMessageBox::Yes) {
        actionDMTriggered();
  }
}

/*!
 * \brief MainWindow::activeVerseChanged sync the main window with the verse player as active verse changes, set the endOfPage flag or flip page if the flag is set
 */
void MainWindow::activeVerseChanged()
{
  m_currVerse = {m_currVerse.page, m_player->activeVerse().surah, m_player->activeVerse().number};

  if (m_currVerse.number == 0)
        m_currVerse.number = 1;

  m_internalVerseChange = true;
  ui->cmbVerse->setCurrentIndex(m_currVerse.number - 1);
  m_internalVerseChange = false;

  if (m_endOfPage) {
        m_endOfPage = false;
        nextPage();
  }

  // If now playing the last verse in the page, set the flag to flip the page
  if (m_currVerse.number == m_vInfoList.last().number
      && m_currVerse.number != m_player->surahCount()) {
        m_endOfPage = true;
  }

  highlightCurrentVerse();
}

/*!
 * \brief MainWindow::verseClicked slot to navigate to the clicked verse in the side panel, sync player, and copy aya text to clipboard
 */
void MainWindow::verseClicked()
{
  // object = clickable label, parent = verse frame, verse frame name scheme = 'surah_verse'
  QStringList data = sender()->parent()->objectName().split('_');
  int surah = data.at(0).toInt();
  int verse = data.at(1).toInt();

  m_currVerse.number = verse;
  m_player->setVerse(m_currVerse);

  if (m_currVerse.surah != surah) {
        m_currVerse.surah = surah;

        m_player->setVerse(m_currVerse);
        m_player->updateSurahVerseCount();
        updateVerseDropDown();

        m_internalSurahChange = true;
        ui->cmbSurah->setCurrentIndex(surah - 1);
        m_internalSurahChange = false;
  }

  m_internalVerseChange = true;
  ui->cmbVerse->setCurrentIndex(verse - 1);
  m_internalVerseChange = false;

  if (m_settingsPtr->value("Reader/CopyVerseOnClick").toBool()) {
        QClipboard *clip = QApplication::clipboard();
        clip->setText(m_dbManPtr->getVerseText(surah, verse));
  }

  m_endOfPage = false;
  m_player->setVerseFile(m_player->constructVerseFilename());
  btnPlayClicked();
}

void MainWindow::verseAnchorClicked(const QUrl &hrefUrl)
{
  QString idx = hrefUrl.toString();
  idx.remove('#');
  Verse v = m_vInfoList.at(idx.toInt());

  m_currVerse.number = v.number;
  m_player->setVerse(m_currVerse);

  if (m_currVerse.surah != v.surah) {
        m_currVerse.surah = v.surah;

        m_player->setVerse(m_currVerse);
        m_player->updateSurahVerseCount();
        updateVerseDropDown();

        m_internalSurahChange = true;
        ui->cmbSurah->setCurrentIndex(v.surah - 1);
        m_internalSurahChange = false;
  }

  m_internalVerseChange = true;
  ui->cmbVerse->setCurrentIndex(v.number - 1);
  m_internalVerseChange = false;

  if (m_settingsPtr->value("Reader/CopyVerseOnClick").toBool()) {
        QClipboard *clip = QApplication::clipboard();
        clip->setText(m_dbManPtr->getVerseText(v.surah, v.number));
  }

  m_endOfPage = false;
  m_player->setVerseFile(m_player->constructVerseFilename());
  btnPlayClicked();
}

/*!
 * \brief MainWindow::highlightCurrentVerse highlights the currently selected/recited verse in the quran page & side panel
 */
void MainWindow::highlightCurrentVerse()
{
  int idx;
  for (idx = 0; idx < m_vInfoList.size(); idx++) {
        Verse v = m_vInfoList.at(idx);
        if (m_currVerse == v)
            break;
  }
  m_quranBrowser->highlightVerse(idx);

  if (m_highlightedFrm != nullptr)
        m_highlightedFrm->setStyleSheet("");

  VerseFrame *verseFrame = ui->scrlVerseCont->findChild<VerseFrame *>(
      QString("%0_%1").arg(QString::number(m_currVerse.surah), QString::number(m_currVerse.number)));

  verseFrame->highlightFrame();

  if (m_highlightedFrm != nullptr) {
        ui->scrlVerseByVerse->verticalScrollBar()->setValue(m_highlightedFrm->pos().y());
  }

  m_highlightedFrm = verseFrame;
}

/* ------------------------ Settings update methods ------------------------ */

/*!
 * \brief MainWindow::actionPrefTriggered open the settings dialog and connect settings change slots
 */
void MainWindow::actionPrefTriggered()
{
  if (m_settingsDlg != nullptr)
        delete m_settingsDlg;

  m_settingsDlg = new SettingsDialog(this, m_settingsPtr, m_player);

  // Restart signal
  connect(m_settingsDlg, &SettingsDialog::restartApp, this, &MainWindow::restartApp);

  // Quran page signals
  connect(m_settingsDlg, &SettingsDialog::redrawQuranPage, this, &MainWindow::redrawQuranPage);
  connect(m_settingsDlg,
          &SettingsDialog::quranFontChanged,
          m_quranBrowser,
          &QuranPageBrowser::updateFontSize);

  // Side panel signals
  connect(m_settingsDlg, &SettingsDialog::redrawSideContent, this, &MainWindow::addSideContent);
  connect(m_settingsDlg,
          &SettingsDialog::sideContentTypeChanged,
          this,
          &MainWindow::updateSideContentType);
  connect(m_settingsDlg, &SettingsDialog::tafsirChanged, this, &MainWindow::updateLoadedTafsir);
  connect(m_settingsDlg,
          &SettingsDialog::translationChanged,
          this,
          &MainWindow::updateLoadedTranslation);
  connect(m_settingsDlg, &SettingsDialog::sideFontChanged, this, &MainWindow::updateSideFont);

  // audio device signals
  connect(m_settingsDlg,
          &SettingsDialog::usedAudioDeviceChanged,
          m_player,
          &VersePlayer::changeUsedAudioDevice);

  m_settingsDlg->show();
}

/*!
 * \brief MainWindow::actionDMTriggered open the download manager dialog, create downloadmanager instance if it's not set
 */
void MainWindow::actionDMTriggered()
{
  if (m_downloaderDlg == nullptr) {
        if (m_downManPtr == nullptr)
            m_downManPtr = new DownloadManager(this, m_dbManPtr, m_player->recitersList());

        m_downloaderDlg = new DownloaderDialog(this, m_settingsPtr, m_downManPtr, m_dbManPtr);
  }

  m_downloaderDlg->show();
}

/*!
 * \brief MainWindow::redrawQuranPage redraw the current quran page
 */
void MainWindow::redrawQuranPage()
{
  m_quranBrowser->constructPage(m_currVerse.page);
  updatePageVerseInfoList();
}

/*!
 * \brief MainWindow::updateSideContentType set side content type to the one in the settings 
 */
void MainWindow::updateSideContentType()
{
  m_sideContent = static_cast<SideContent>(m_settingsPtr->value("Reader/SideContent").toInt());
}

/*!
 * \brief MainWindow::updateLoadedTafsir set tafsir to the one in the settings, update the selected db 
 */
void MainWindow::updateLoadedTafsir()
{
  DBManager::Tafsir currTafsir = static_cast<DBManager::Tafsir>(
      m_settingsPtr->value("Reader/Tafsir").toInt());

  m_dbManPtr->setCurrentTafsir(currTafsir);
}

/*!
 * \brief MainWindow::updateLoadedTranslation set translation to the one in the settings, update the selected db 
 */
void MainWindow::updateLoadedTranslation()
{
  DBManager::Translation currTrans = static_cast<DBManager::Translation>(
      m_settingsPtr->value("Reader/Translation").toInt());

  m_dbManPtr->setCurrentTranslation(currTrans);
}

/*!
 * \brief MainWindow::updateSideFont set side content font to the one in the settings 
 */
void MainWindow::updateSideFont()
{
  m_sideFont = qvariant_cast<QFont>(m_settingsPtr->value("Reader/SideContentFont"));
}

/* ------------------------ Side content generation ------------------------ */

/*!
 * \brief MainWindow::addSideContent updates the side panel with the chosen side content type
 */
void MainWindow::addSideContent()
{
  if (!m_verseFrameList.isEmpty()) {
        qDeleteAll(m_verseFrameList);
        m_verseFrameList.clear();
        m_highlightedFrm = nullptr;
  }

  bool showTafsir = m_sideContent == SideContent::tafsir;
  bool showFullTafsir = m_settingsPtr->value("Reader/Tafsir").toInt()
                        == DBManager::Tafsir::muyassar;

  ClickableLabel *verselb;
  ClickableLabel *contentLb;
  VerseFrame *verseContFrame;
  QString prevLbContent, currLbContent;
  for (int i = m_vInfoList.size() - 1; i >= 0; i--) {
        Verse vInfo = m_vInfoList.at(i);

        verseContFrame = new VerseFrame(ui->scrlVerseCont);
        verselb = new ClickableLabel(verseContFrame);
        contentLb = new ClickableLabel(verseContFrame);

        verseContFrame->setObjectName(QString("%0_%1").arg(vInfo.surah).arg(vInfo.number));

        verselb->setFont(QFont(m_quranBrowser->pageFont(), m_quranBrowser->fontSize() - 2));
        verselb->setText(m_dbManPtr->getVerseGlyphs(vInfo.surah, vInfo.number));
        verselb->setAlignment(Qt::AlignCenter);
        verselb->setWordWrap(true);

        if (showTafsir) {
            QString txt = m_dbManPtr->getTafsir(vInfo.surah, vInfo.number);
            if (showFullTafsir)
                currLbContent = txt;
            else {
                currLbContent = tr("Expand...");
                connect(contentLb,
                        &ClickableLabel::clicked,
                        this,
                        &MainWindow::showExpandedVerseTafsir);
            }

        } else {
            currLbContent = m_dbManPtr->getTranslation(vInfo.surah, vInfo.number);
        }

        if (currLbContent == prevLbContent && (!showTafsir || showFullTafsir)) {
            currLbContent.clear();
        } else {
            prevLbContent = currLbContent;
        }

        contentLb->setText(currLbContent);
        contentLb->setTextFormat(Qt::RichText);
        contentLb->setTextInteractionFlags(Qt::TextSelectableByMouse);
        contentLb->setAlignment(Qt::AlignCenter);
        contentLb->setWordWrap(true);
        contentLb->setFont(m_sideFont);

        verseContFrame->layout()->addWidget(verselb);
        verseContFrame->layout()->addWidget(contentLb);
        ui->scrlVerseCont->layout()->addWidget(verseContFrame);
        m_verseFrameList.insert(0, verseContFrame);

        // connect clicked signal for each label
        connect(verselb,
                &ClickableLabel::clicked,
                this,
                &MainWindow::verseClicked,
                Qt::UniqueConnection);
  }

  if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        highlightCurrentVerse();
  }
}

void MainWindow::copyVerseText(int IdxInPage)
{
  const Verse &v = m_vInfoList.at(IdxInPage);
  QClipboard *clip = QApplication::clipboard();
  clip->setText(m_dbManPtr->getVerseText(v.surah, v.number));
}

void MainWindow::saveReaderState()
{
  m_settingsPtr->setValue("WindowState", saveState());
  m_settingsPtr->setValue("Reciter", ui->cmbReciter->currentIndex());

  m_settingsPtr->beginGroup("Reader");
  m_settingsPtr->setValue("Page", m_currVerse.page);
  m_settingsPtr->setValue("Surah", m_currVerse.surah);
  m_settingsPtr->setValue("Verse", m_currVerse.number);
  m_settingsPtr->endGroup();

  m_settingsPtr->sync();
}

void MainWindow::restartApp()
{
  saveReaderState();
  emit QApplication::exit();
  QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

/*!
 * \brief MainWindow::showExpandedVerseTafsir toggle a collapsed verse tafsir
 */
void MainWindow::showExpandedVerseTafsir()
{
  ClickableLabel *showLb = qobject_cast<ClickableLabel *>(sender());

  if (showLb->text() == tr("Expand...")) {
        QStringList data = sender()->parent()->objectName().split('_');
        QString text = m_dbManPtr->getTafsir(data.at(0).toInt(), data.at(1).toInt());
        showLb->setText(text);
  } else
        showLb->setText(tr("Expand..."));
}

/*!
 * \brief MainWindow::openSearchDialog open the verse search dialog
 */
void MainWindow::openSearchDialog()
{
  if (m_srchDlg == nullptr) {
        m_srchDlg = new SearchDialog(this, m_settingsPtr->value("Reader/QCF").toInt(), m_dbManPtr);
        connect(m_srchDlg, &SearchDialog::navigateToVerse, this, &MainWindow::navigateToVerse);
  }

  m_srchDlg->show();
}

/*!
 * \brief MainWindow::navigateToVerse navigate to a selected verse from the search results
 * \param v Verse to navigate to
 */
void MainWindow::navigateToVerse(Verse v)
{
  m_currVerse = v;

  redrawQuranPage();
  addSideContent();

  updateVerseDropDown();

  m_internalPageChange = true;
  ui->cmbPage->setCurrentIndex(m_currVerse.page - 1);
  m_internalPageChange = false;

  m_internalSurahChange = true;
  ui->cmbSurah->setCurrentIndex(m_currVerse.surah - 1);
  m_internalSurahChange = false;

  m_internalVerseChange = true;
  ui->cmbVerse->setCurrentIndex(m_currVerse.number - 1);
  m_internalVerseChange = false;

  highlightCurrentVerse();

  m_player->setVerse(m_currVerse);
  m_player->updateSurahVerseCount();
  m_player->setVerseFile(m_player->constructVerseFilename());
  m_endOfPage = false;
}

MainWindow::~MainWindow()
{
  saveReaderState();
  delete ui;
}
