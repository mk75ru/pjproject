#include "VsDbTest.h"
#include <QtSql>

// -----------------------------------------------------------------------------
// тест функций для sql
// -----------------------------------------------------------------------------
VsDbTest::VsDbTest (QWidget *parent)
  : QDialog(parent), m_DbAddr("127.0.0.1")
{
  QVBoxLayout *vlt;
  QHBoxLayout *hlt;
  QLabel *lbl;
  QPushButton *btn;
  QFont fnt;

  m_BinDir = qApp->applicationDirPath ();
  m_BinDir.replace ('\\', '/');
  if (!m_BinDir.endsWith ('/'))
    m_BinDir.push_back ('/');
  m_WrkDir = QDir::currentPath ();
  m_WrkDir.replace ('\\', '/');
  if (!m_WrkDir.endsWith ('/'))
    m_WrkDir.push_back ('/');
  m_LogLib.setFileName (m_BinDir + "Logger");
  fnt.setPointSize (10);
  fnt.setWeight (50);
  setFont (fnt);
  fnt.setPointSize (8);

  vlt = new QVBoxLayout (this);
  vlt->setMargin (4);
  vlt->setSpacing (4);
  hlt = new QHBoxLayout;
  hlt->setMargin (0);
  hlt->setSpacing (4);
  lbl = new QLabel ("Имя БД");
  hlt->addWidget (lbl);
  m_DbNameEd = new QLineEdit;
  m_DbNameEd->setMinimumWidth (140);
  m_DbNameEd->setText ("ts001");
  hlt->addWidget (m_DbNameEd);
  lbl = new QLabel ("Пароль");
  hlt->addWidget (lbl);
  m_PassWrdEd = new QLineEdit;
  m_PassWrdEd->setMinimumWidth (80);
  hlt->addWidget (m_PassWrdEd);
  hlt->addStretch (1);
  vlt->addLayout (hlt);

  hlt = new QHBoxLayout;
  hlt->setMargin (0);
  hlt->setSpacing (4);
  m_CreateBtn = new QPushButton ("Create");
  m_CreateBtn->setFont (fnt);
  m_CreateBtn->setEnabled (false);
  connect (m_CreateBtn, SIGNAL(clicked(bool)), SLOT(slotDbCreate()));
  hlt->addWidget (m_CreateBtn);

  m_OpenBtn = new QPushButton ("Open");
  m_OpenBtn->setFont (fnt);
  m_OpenBtn->setEnabled (false);
  connect (m_OpenBtn, SIGNAL(clicked(bool)), SLOT(slotDbOpen()));
  hlt->addWidget (m_OpenBtn);

  m_CloseBtn = new QPushButton ("Close");
  m_CloseBtn->setFont (fnt);
  m_CloseBtn->setEnabled (false);
  connect (m_CloseBtn, SIGNAL(clicked(bool)), SLOT(slotDbClose()));
  hlt->addWidget (m_CloseBtn);

  m_DeleteBtn = new QPushButton ("Delete");
  m_DeleteBtn->setFont (fnt);
  m_DeleteBtn->setEnabled (false);
  connect (m_DeleteBtn, SIGNAL(clicked(bool)), SLOT(slotDbDelete()));
  hlt->addWidget (m_DeleteBtn);
  hlt->addStretch (1);
  vlt->addLayout (hlt);

  vlt->addStretch (1);
  hlt = new QHBoxLayout;
  hlt->setMargin (0);
  hlt->setSpacing (4);
  hlt->addStretch (1);
  btn = new QPushButton ("Выход");
  btn->setFont (fnt);
  connect (btn, SIGNAL(clicked(bool)), SLOT(reject()));
  hlt->addWidget (btn);
  vlt->addLayout (hlt);

  setWindowFlags (Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
  setWindowTitle ("Postgres DB test");
  if (m_Journal)
    loadLogger ();
  if (m_Log){
    m_Log->writeLog ("------------------------- старт -------------------------", 0x11);
    m_Log->writeLog ("VsDbTest | VsDbTest | Приложение VsDbTest стартовало.", 0x11);
  }
  init ();
}

// -----------------------------------------------------------------------------
VsDbTest::~VsDbTest ()
{
  QSqlDatabase db = QSqlDatabase::database ("sv");
  if (db.isValid () && db.isOpen ()){
    db.close ();
    if (m_Log)
      m_Log->writeLog ("VsDbTest | ~VsDbTest | База данных '" +
          db.databaseName () + "' закрыта.", 0x11);
  }
  if (m_Log){
    m_Log->writeLog ("VsDbTest | ~VsDbTest | Приложение VsDbTest завершено.", 0x11);
    m_Log->writeLog ("------------------------- стоп -------------------------", 0x11);
    delete m_Log;
    if (m_LogLib.isLoaded ())
      m_LogLib.unload ();
  }
}

// -----------------------------------------------------------------------------
void VsDbTest::slotDbCreate ()
{
  QSqlDatabase db = QSqlDatabase::database ("sv");

  try{
    m_DbName = m_DbNameEd->text ();
    if (m_DbName.isEmpty ())
      throw 1;
    m_PassWrd = m_PassWrdEd->text ().trimmed ();
    db.setDatabaseName ("postgres");
    db.setHostName (m_DbAddr);
    db.setPort (m_DbPort);
    db.setUserName ("postgres");
    if (!m_PassWrd.isEmpty ())
      db.setPassword (m_PassWrd);
    if (!db.open ()){
      if (m_Log)
        m_Log->writeLog ("VsDbTest | slotDbCreate | Не могу открыть БД '" +
            db.databaseName () + "'!", 0x15, 6);
      throw 1;
    }
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbCreate | База данных '" +
          db.databaseName () + "' открыта.", 0x15);

    QSqlQuery q (db);
    if (!q.exec ("CREATE DATABASE \"" + m_DbName + "\" OWNER postgres "
        "ENCODING UTF8 TEMPLATE postgres;")){
      if (m_Log)
        m_Log->writeLog ("VsDbTest | slotDbCreate | Не могу создать БД '" +
            m_DbName + "'!", 0x11, 6);
      throw 1;
    }
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbCreate | База данных '" +
          m_DbName + "' создана.", 0x11);
  }
  catch (...){
  }
  if (db.isOpen ()){
    db.close ();
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbCreate | База данных '" +
          db.databaseName () + "' закрыта.", 0x11);
  }
}

// -----------------------------------------------------------------------------
void VsDbTest::slotDbOpen ()
{
  QSqlDatabase db = QSqlDatabase::database ("sv");

  try{
    if (!db.isValid ()){
      if (m_Log)
        m_Log->writeLog ("VsDbTest | slotDbOpen | Отсутствует драйвер БД!", 0x11, 6);
      throw 1;
    }
    m_DbName = m_DbNameEd->text ();
    if (m_DbName.isEmpty ())
      throw 1;
    m_PassWrd = m_PassWrdEd->text ().trimmed ();
    db.setDatabaseName (m_DbName);
    db.setHostName (m_DbAddr);
    db.setPort (m_DbPort);
    db.setUserName ("postgres");
    if (!m_PassWrd.isEmpty ())
      db.setPassword (m_PassWrd);
    if (!db.open ()){
      if (m_Log)
        m_Log->writeLog ("VsDbTest | slotDbOpen | Не могу открыть БД '" +
            db.databaseName () + "'!", 0x15, 6);
      throw 1;
    }
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbOpen | База данных '" +
          db.databaseName () + "' открыта.", 0x15);
    m_OpenBtn->setEnabled (false);
    m_CloseBtn->setEnabled (true);
    m_CreateBtn->setEnabled (false);
    m_DeleteBtn->setEnabled (false);
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void VsDbTest::slotDbClose ()
{
  QSqlDatabase db = QSqlDatabase::database ("sv");

  if (!db.isValid ())
    return;
  if (db.isOpen ()){
    db.close ();
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbClose | База данных '" +
          db.databaseName () + "' закрыта.", 0x11);
    m_OpenBtn->setEnabled (true);
    m_CloseBtn->setEnabled (false);
    m_CreateBtn->setEnabled (true);
    m_DeleteBtn->setEnabled (true);
  }
}

// -----------------------------------------------------------------------------
void VsDbTest::slotDbDelete ()
{
  QSqlDatabase db = QSqlDatabase::database ("sv");

  try{
    if (!db.isValid ())
      throw 1;
    m_DbName = m_DbNameEd->text ();
    if (m_DbName.isEmpty () || m_DbName == "postgres")
      throw 1;
    db.setDatabaseName ("postgres");
    db.setHostName (m_DbAddr);
    db.setPort (m_DbPort);
    db.setUserName ("postgres");
    if (!db.open())
      throw 1;
    QSqlQuery q (db);
    if (!q.exec ("DROP DATABASE IF EXISTS \"" + m_DbName + "\";")){
      if (m_Log)
        m_Log->writeLog ("VsDbTest | slotDbDelete | Не могу удалить БД '" +
            m_DbName + "'!", 0x11, 6);
      throw 1;
    }
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbDelete | База данных '" + m_DbName +
          "' удалена.", 0x11);
  }
  catch (...){
  }
  if (db.isOpen ()){
    db.close ();
    if (m_Log)
      m_Log->writeLog ("VsDbTest | slotDbDelete | База данных '" +
          db.databaseName () + "' закрыта.", 0x11);
  }
}

// -----------------------------------------------------------------------------
void VsDbTest::init ()
{
  QSqlDatabase db = QSqlDatabase::addDatabase ("QPSQL", "sv");

  try{
    if (!db.isValid ()){
      if (m_Log)
        m_Log->writeLog ("VsDbTest | init | Отсутствует драйвер БД!", 0x11, 6);
      throw 1;
    }
    if (m_Log)
      m_Log->writeLog ("VsDbTest | init | Postgresql доступен.", 0x11);
    m_OpenBtn->setEnabled (true);
    m_CreateBtn->setEnabled (true);
    m_DeleteBtn->setEnabled (true);
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void VsDbTest::loadLogger ()
{
  CreateLoggerFn fn;
  QString ldir = m_WrkDir + "Logs/";
  QDir dr (ldir);
  bool fl = false;

  if (m_Log)
    return;
  try{
    if (!m_LogLib.load ())
      throw 1;
    fl = true;
    fn = (CreateLoggerFn)m_LogLib.resolve ("CreateLogger");
    if (!fn)
      throw 1;
    if (!dr.exists ())
      dr.mkdir (ldir);
    if (!dr.exists ())
      ldir = m_WrkDir;
    m_Log = fn (ldir + "VsDbTest");
    if (!m_Log)
      throw 1;
    m_Log->setCaption ("VsDbTest версия 1.0.0.1 от 10.02.2019");
  }
  catch (...){
    if (fl && m_LogLib.isLoaded ())
      m_LogLib.unload ();
  }
}

// -----------------------------------------------------------------------------
