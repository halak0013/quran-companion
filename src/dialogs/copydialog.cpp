#include "copydialog.h"
#include "ui_copydialog.h"

CopyDialog::CopyDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::CopyDialog)
  , m_verseValidator(new QIntValidator(this))
  , m_currVerse(Verse::getCurrent())
  , m_quranDb(QuranDb::getInstance())
  , m_notifier(this)
{
  ui->setupUi(this);
  ui->cmbCopyFrom->setValidator(m_verseValidator);
  ui->cmbCopyTo->setValidator(m_verseValidator);
  connect(
    ui->buttonBox, &QDialogButtonBox::accepted, this, &CopyDialog::copyRange);
}

void
CopyDialog::copyVerseText(const Verse v)
{
  QClipboard* clip = QApplication::clipboard();
  QString text = m_quranDb.verseText(v.surah(), v.number());
  QString vNum = QString::number(v.number());
  text.remove(text.size() - 1, 1);
  text = text.trimmed();
  text = "{" + text + "}";
  text += ' ';
  text += "[" + m_quranDb.surahNames().at(v.surah() - 1) + ":" + vNum + "]";
  clip->setText(text);
  m_notifier.copied();
}

void
CopyDialog::copyRange()
{
  int from = ui->cmbCopyFrom->currentText().toInt();
  int to = ui->cmbCopyTo->currentText().toInt();
  if (to < from || from <= 0 || from > m_currVerse.surahCount() || to <= 0 ||
      to > m_currVerse.surahCount()) {
    QMessageBox::warning(
      this, tr("Invalid range"), tr("The entered verse range is invalid"));
    return;
  }

  QString final = "{ ";
  QClipboard* clip = QApplication::clipboard();
  for (int i = from; i <= to; i++) {
    QString text = m_quranDb.verseText(m_currVerse.surah(), i);
    text.remove(text.size() - 1, 1);
    text += "(" + QString::number(i) + ") ";
    final.append(text);
  }

  final += "} [" + m_quranDb.surahNames().at(m_currVerse.surah() - 1) + "]";
  clip->setText(final);
  m_notifier.copied();
}

void
CopyDialog::show()
{
  ui->lbCopySurahName->setText(m_quranDb.surahName(m_currVerse.surah()));

  ui->cmbCopyFrom->clear();
  ui->cmbCopyTo->clear();
  for (int i = 1; i <= m_currVerse.surahCount(); i++) {
    ui->cmbCopyFrom->addItem(QString::number(i));
    ui->cmbCopyTo->addItem(QString::number(i));
  }

  int idx = m_currVerse.number() ? m_currVerse.number() - 1 : 0;
  ui->cmbCopyFrom->setCurrentIndex(idx);
  ui->cmbCopyTo->setCurrentIndex(idx);
  m_verseValidator->setBottom(1);
  m_verseValidator->setTop(m_currVerse.surahCount());

  QDialog::show();
}

const CopyNotifier*
CopyDialog::notifier() const
{
  return &m_notifier;
}

void
CopyDialog::closeEvent(QCloseEvent* event)
{
  this->hide();
}

CopyDialog::~CopyDialog()
{
  delete ui;
}
