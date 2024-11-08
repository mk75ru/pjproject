#pragma once
#include <QtCore>
#include <QtSql>
#include "ILogger.h"
#include "JournalObject.h"
#include "ThSrvObjects.h"
#include "ThSrvDataObj.h"
#include "IPgDataKeeper.h"

namespace ItkSrv {

class TechPgDataKeeper;
// -------------------------------
// объект информационной системы
// -------------------------------
class ItkPgObject : public ItkObject
{
public:
  ItkPgObject (TechPgDataKeeper *dkeeper, QObject *pobj = nullptr);
  ~ItkPgObject ();

  void read ();
  bool save ();
  void setDefaultFeatures ();
  ItemList& content (bool rd = false);
  bool saveContent ();
  ServChannels& channels (bool rd = false);
  bool saveChannels ();
  ObjPropList& properties (bool rd = false);
  bool saveProperties ();
  Ui8Set& abilities (bool rd = false);
  void readAbilities (Abilities &alst);
  bool saveAbilities (Abilities &alst);

protected:
  void readContent (Item *item, QSqlDatabase &db);
  void saveContent (Item *item, QSqlDatabase &db);
  bool getLogger ();

protected:
  TechPgDataKeeper* m_Keeper;
  ILoggerHost*      m_LHost {};
  ILogger*          m_Log {};
  quint16           m_LogFlags {};
  quint16           m_ObjInit {};
};

// ----------------------------
// хранилище данных ИТК-ОС ЦД
// ----------------------------
class TechPgDataKeeper : public ThSrvDataKeeper, public IPgDataKeeper
{
  Q_OBJECT

public:
  TechPgDataKeeper (const KeeperDataInit &di);
  ~TechPgDataKeeper ();

  // implementation ThSrvDataKeeper
  void init ();
  bool createNewBase (const QString &name);
  bool reloadBase (const QString &name);

  Features& features (bool rd = false);
  bool saveFeatures ();
  Abilities& abilities (bool rd = false);
  bool saveAbilities ();
  Channels& channels (bool rd = false);
  bool saveChannels ();
  Properties& properties (bool rd = false);
  bool saveProperties ();
  ObjTypes& types (bool rd = false);
  bool saveTypes ();
  SrvEvents& srvEvents (bool rd = false);
  bool saveSrvEvents ();
  ItkSystems& itkSystems (bool rd = false);
  bool saveItkSystems ();

  void readTypeAbilities (Abilities &alst, quint16 tp);
  bool saveTypeAbilities (Abilities &alst, quint16 tp);
  void readTypeFeatures (Features &ftlst, quint16 tp);
  bool saveTypeFeatures (Features &ftlst, quint16 tp);
  void readTypeChannels (Channels &clst, quint16 tp);
  bool saveTypeChannels (Channels &clst, quint16 tp);
  void readTypeProperties (Properties &plst, quint16 tp);
  bool saveTypeProperties (Properties &plst, quint16 tp);
  void propValueSet (PropValList &vs, quint16 pr);
  bool savePropValueSet (PropValList &vs, quint16 pr);

  void readObjects (ObjList &objlst, quint8 sys = 0, bool act = false);
  bool saveObjects (ObjList &objlst);
  ItkObject* readCurrentObject (quint32 id, quint32 num = 0, quint8 sys = 0);
  ItkObject* readObject (quint32 num, quint8 sys);
  ItkObject* createNewObject ();
  void readContacts (ContObjects &cont, quint32 oid);
  bool saveContacts (ContObjects &cont, quint32 oid);
  void removeObject (quint32 num, quint8 sys);
  void readContactNumbers (ContNumbers &cont, quint32 onum, quint8 sys);
  void readContChannels (ObjChannels &chns, quint64 &contid);
  bool saveContChannels (ObjChannels &chns, quint64 &contid);
  void readServChannels (ServChannels &chnls, quint32 oid);
  void contObjects (ObjList &objlst, Ui32List &clst, quint8 syst = 0);
  void updateObjCenter (quint32 num, quint32 cntnum, quint8 sys);
  quint32 getObjCenter (quint32 num, quint8 sys);
  void getObjCenters (ObjCenters &cntrs, quint8 sys);
  void readShells (ShellClients &shells, quint32 oid);
  bool saveShells (ShellClients &shells, quint32 oid);
  void readRouteTable (AbnRoutTable &table, quint32 oid);
  bool saveRouteTable (AbnRoutTable &table, quint32 oid);

  void readObjFtChanges (ObjFeatures &ftrs, quint32 oid);
  void saveObjFtChange (ObjFtChangeEvent *fcev);
  ContConnEvent* abnConnectEv (quint32 cid);
  void saveAbnConnEv (ContConnEvent *cce);
  void readEvents (ObjectEvents &events, EvFilter *flt);
  bool saveEvent (ItkSrvEvent *ev);
  bool saveEvents (ObjectEvents &events);
  // implementation IPgDataKeeper
  void dbClear (QDateTime &dtm);
  void dbVacuum ();
  bool saveAlertSeance (PgAlertSeance *als, quint8 sys);
  bool saveAlertObjects (Ui32List &objlst, quint32 sid);
  PgAlertSeance* readAlertSeance (quint64 &acode, quint8 sys);
  void readAlertObjects (Ui32List &objlst, quint32 sid);
  bool saveAlertEvent (PgAlertEvent *ae);
  void readSeances (TsAlertSeances &alerts, EvFilter *flt, quint8 sys);
  void readAlObjSounds (AlObjEvents &evts, quint32 sid, quint32 anum, quint8 sys);
  bool joinAlertSeances (quint64 &acode, quint64 &oldcode, quint8 sys);
  // end implementation

  QString valTableName (quint8 tp);
  void variantToValue (QVariant &qv, Value &val, quint8 tp);
  void valueToVariant (Value &val, QVariant &qv, quint8 tp);
  ILoggerHost* loggerHost () { return m_LHost; }

private:
  bool checkData (QSqlDatabase &db);
  void addContObject (QSqlDatabase &db, quint32 obj, quint32 cont, quint8 lev);
  void updateContObject (QSqlDatabase &db, quint32 obj, quint32 cont, quint8 lev);
  void deleteContObject (QSqlDatabase &db, quint32 obj, quint32 cont);
  bool getLogger ();

private slots:
  void slotTimer ();

private:
  ILoggerHost*  m_LHost {};
  ILogger*      m_Log {};
  QTimer*       m_InitTimer {};
  QString       m_BsName;
  QString       m_ExtDir;
  QString       m_BsAddr;
  QString       m_PassWrd;
  QString       m_WrkDir;
  QString       m_IniFileName;
  quint16       m_BsPort {};
  quint16       m_LogFlags {};
  quint16       m_ObjInit {};
  quint16       m_TryOpenCounter {};

public:
  static const char*  m_ObjVersion;
  static const char*  m_ObjVersionDate;
};

// -----------------------------------------------------------------------------
} // namespace ItkSrv
