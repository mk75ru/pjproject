#pragma once
#include <QtWidgets>
#include "ILogger.h"
#include "JournalObject.h"

// ----------------------
// тест функций для sql
// ----------------------
class VsDbTest : public QDialog
{
  Q_OBJECT

public:
  VsDbTest (QWidget *parent = nullptr);
  ~VsDbTest ();
  QSize sizeHint () const { return QSize (420, 120); }

private slots:
  void slotDbCreate ();
  void slotDbOpen ();
  void slotDbClose ();
  void slotDbDelete ();

private:
  void init ();
  void loadLogger ();

private:
  QString       m_DbName;
  QString       m_DbAddr;
  QString       m_PassWrd;
  ILogger*      m_Log {};
  quint16       m_DbPort {5432};
  quint16       m_LogMaxSize {512};
  bool          m_Journal {true};
  bool          m_DbInit {};

  QString       m_BinDir;
  QString       m_WrkDir;
  QLibrary      m_LogLib;

  QLineEdit*    m_DbNameEd;
  QLineEdit*    m_PassWrdEd;
  QPushButton*  m_OpenBtn;
  QPushButton*  m_CloseBtn;
  QPushButton*  m_CreateBtn;
  QPushButton*  m_DeleteBtn;
};

// -----------------------------------------------------------------------------
