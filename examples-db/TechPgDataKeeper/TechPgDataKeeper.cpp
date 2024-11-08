#include "TechPgDataKeeper.h"
#include "Value.h"
#include "AlertSeances.h"

using namespace UnVal;

namespace ItkSrv {

// -----------------------------------------------------------------------------
// объект информационной системы
// -----------------------------------------------------------------------------
ItkPgObject::ItkPgObject (TechPgDataKeeper *dkeeper, QObject *pobj)
  : ItkObject(pobj), m_Keeper(dkeeper), m_LHost(dkeeper->loggerHost ())
{
}

// -----------------------------------------------------------------------------
ItkPgObject::~ItkPgObject ()
{
}

// -----------------------------------------------------------------------------
void ItkPgObject::read ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);

  if (!m_Id || !db.isValid () || !db.isOpen ())
    return;
  m_Abil.clear ();
  qDeleteAll (m_Channels);
  m_Channels.clear ();
  qDeleteAll (m_Content);
  m_Content.clear ();
  m_Features.clear ();
  m_ObjInit = 0;
  try{
    q.prepare ("SELECT i.Name, i.Num, i.Tp, o.Syst, o.Act, t.Name FROM Items i,"
        " Objects o, Types t WHERE i.Id=:id AND o.Id=i.Id AND t.Id=i.Tp");
    q.bindValue (":id", m_Id);
    if (!q.exec () || !q.next ())
      throw 1;
    m_Name = q.value (0).toString ();
    m_Num = q.value (1).toUInt ();
    m_Type = quint16(q.value (2).toUInt ());
    m_Syst = quint8(q.value (3).toUInt ());
    m_Act = q.value (4).toBool ();
    m_TypeName = q.value (5).toString ();
    m_st = 0;
    readContent (this, db);
    m_ObjInit |= 1;
    channels ();
    abilities ();
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool ItkPgObject::save ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  FeatureSet::iterator it;
  quint8 ft, fv;
  bool res = false;

  if ((m_st & 4) != 0 || !db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    if ((m_st & 3) == 1){
      q.prepare ("UPDATE Items SET Name=:name, Num=:num, Tp=:tp WHERE Id=:id;");
      q.bindValue (":name", m_Name);
      q.bindValue (":num", m_Num);
      q.bindValue (":tp", m_Type);
      q.bindValue (":id", m_Id);
      if (!q.exec ())
        throw 1;
      q.clear ();
      q.prepare ("UPDATE Objects SET Syst=:sys, Act=:act WHERE Id=:id;");
      q.bindValue (":sys", m_Syst);
      q.bindValue (":act", m_Act);
      q.bindValue (":id", m_Id);
      if (!q.exec ())
        throw 1;
      m_st &= 0xf8;
    }
    else if (!m_Id){
      q.prepare ("INSERT INTO Items (Id, Parent, Name, Num, Tp) VALUES (DEFAULT, 0,"
        " ?, ?, ?) RETURNING Id;");
      q.bindValue (0, m_Name);
      q.bindValue (1, m_Num);
      q.bindValue (2, m_Type);
      if (!q.exec () || !q.next ())
        throw 1;
      m_Id = q.value (0).toUInt ();
      q.clear ();
      q.prepare ("INSERT INTO Objects (Id, Syst, Act) VALUES (?, ?, ?);");
      q.bindValue (0, m_Id);
      q.bindValue (1, m_Syst);
      q.bindValue (2, m_Act);
      if (!q.exec ())
        throw 1;
      m_st &= 0xf8;
    }
    if ((m_st & 8) != 0){
      q.clear ();
      q.prepare ("DELETE FROM ItemFeatures WHERE Itm=:id;");
      q.bindValue (":id", m_Id);
      if (!q.exec ())
        throw 1;
      q.clear ();
      q.prepare ("INSERT INTO ItemFeatures (Itm, Ftr, FtVal) VALUES (?, ?, ?);");
      q.bindValue (0, m_Id);
      for (it = m_Features.begin (); it != m_Features.end (); ++ it){
        ft = it.key ();
        fv = it.value ();
        q.bindValue (1, ft);
        q.bindValue (2, fv);
        if (!q.exec ())
          throw 1;
      }
      m_st &= 0xf0;
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void ItkPgObject::setDefaultFeatures ()
{
  if ((m_st & 2) != 0 && m_Type){
    m_Features.clear ();
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    quint8 ui8;

    try{
      q.prepare ("SELECT Name FROM Types WHERE Id=:tp");
      q.bindValue (":tp", m_Type);
      if (!q.exec () || !q.next ())
        throw 1;
      m_TypeName = q.value (0).toString ();
      q.clear ();
      q.prepare ("SELECT Ftr FROM TypeFeatures WHERE Tp=:tp");
      q.bindValue (":tp", m_Type);
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        ui8 = quint8(q.value (0).toUInt ());
        m_Features.insert (ui8, 0);
      }
      m_st |= 8;
      m_ObjInit |= 1;
    }
    catch (...){
    }
  }
}

// -----------------------------------------------------------------------------
ItemList& ItkPgObject::content (bool rd)
{
  if (rd || (m_ObjInit & 0x0001) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");

    qDeleteAll (m_Content);
    m_Content.clear ();
    m_ObjInit &= 0xfffe;
    readContent (this, db);
    m_ObjInit |= 1;
  }
  return m_Content;
}

// -----------------------------------------------------------------------------
bool ItkPgObject::saveContent ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");

  saveContent (this, db);
  return true;
}

// -----------------------------------------------------------------------------
ServChannels& ItkPgObject::channels (bool rd)
{
  if (rd || (m_ObjInit & 0x0004) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    SrvChannel *chn;

    try{
      if (!m_Id || !db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Channels);
      m_Channels.clear ();
      m_ObjInit &= 0xfffb;
      q.prepare ("SELECT s.Id, s.Chan, s.Num, c.Name, s.Mode, s.Valid FROM "
          "ServChannels s, Channels c WHERE s.ObjId=:id AND c.Id=s.Chan ORDER BY "
          "s.Chan, s.Num");
      q.bindValue (":id", m_Id);
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        chn = new SrvChannel;
        chn->id = q.value (0).toUInt ();
        chn->ch = char(q.value (1).toUInt ());
        chn->num = quint8(q.value (2).toUInt ());
        chn->name = q.value (3).toString ();
        chn->mode = quint8(q.value (4).toUInt ());
        chn->valid = quint8(q.value (5).toBool ());
        m_Channels.push_back (chn);
      }
      m_ObjInit |= 0x0004;
    }
    catch (...){
    }
  }
  return m_Channels;
}

// -----------------------------------------------------------------------------
bool ItkPgObject::saveChannels ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ServChannels::iterator it;
  SrvChannel *chn;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ServChannels WHERE Id=:id;");
    it = m_Channels.begin ();
    while (it != m_Channels.end ()){
      chn = *it;
      if ((chn->st & 4) != 0){
        q.bindValue (":id", chn->id);
        if (!q.exec ())
          throw 1;
        it = m_Channels.erase (it);
        delete chn;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE ServChannels SET Mode=:mode, Valid=:val WHERE Id=:id;");
    for (SrvChannel *chn : m_Channels){
      if ((chn->st & 3) == 1){
        q.bindValue (":mode", chn->mode);
        q.bindValue (":val", chn->valid);
        q.bindValue (":id", chn->id);
        if (!q.exec ())
          throw 1;
        chn->st &= 0xf0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO ServChannels (Id, ObjId, Chan, Num, Mode, Valid) "
        "VALUES (DEFAULT, ?, ?, ?, ?, ?) RETURNING Id;");
    for (SrvChannel *chn : m_Channels){
      if ((chn->st & 2) != 0){
        q.bindValue (0, m_Id);
        q.bindValue (1, chn->ch);
        q.bindValue (2, chn->num);
        q.bindValue (3, chn->mode);
        q.bindValue (4, chn->valid);
        if (!q.exec () || !q.next ())
          throw 1;
        chn->id = q.value (0).toUInt ();
        chn->st &= 0xf0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
ObjPropList& ItkPgObject::properties (bool rd)
{
  if (rd || (m_ObjInit & 0x0002) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    ObjPropVal *pv;
    QVariant qv;
    quint8 tp = 0;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Prop);
      m_Prop.clear ();
      if (m_Id){
        m_ObjInit &= 0xfffd;
        q.prepare ("SELECT c.Id, c.Prop, p.Name, p.VTp FROM ObjPropVals c, "
            "Properties p WHERE c.Obj=:id AND p.Id=c.Prop");
        q.bindValue (":id", m_Id);
        if (!q.exec ())
          throw 1;
        while (q.next ()){
          pv = new ObjPropVal;
          pv->id = q.value (0).toUInt ();
          pv->pr = q.value (1).toUInt ();
          pv->name = q.value (2).toString ();
          pv->tp = ValType(q.value (3).toUInt ());
          m_Prop.push_back (pv);
        }
        for (ObjPropVal *pv : m_Prop){
          if (tp != quint8(pv->tp)){
            tp = quint8(pv->tp);
            q.clear ();
            q.prepare ("SELECT Val FROM " + m_Keeper->valTableName (tp) +
                " WHERE Id=:id");
          }
          q.bindValue (":id", pv->id);
          if (!q.exec ())
            throw 1;
          if (q.next ()){
            qv = q.value (0);
            if (qv.isValid () && !qv.isNull ())
              m_Keeper->variantToValue (qv, pv->val, tp);
          }
        }
        m_ObjInit |= 0x0002;
      }
    }
    catch (...){
      if (getLogger ())
        m_Log->writeLog (QString ("ItkPgObject | properties | Объект %1. Ошибка "
            "при считывании свойств!").arg (m_Num), 0x15, 6);
    }
  }
  return m_Prop;
}

// -----------------------------------------------------------------------------
bool ItkPgObject::saveProperties ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ObjPropList::iterator it;
  QVariant qv;
  ObjPropVal *pv;
  quint8 tp = 0;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ObjPropVals WHERE Id=:id;");
    it = m_Prop.begin ();
    while (it != m_Prop.end ()){
      pv = *it;
      if ((pv->st & 4) != 0){
        q.bindValue (":id", pv->id);
        if (!q.exec ())
          throw 1;
        it = m_Prop.erase (it);
        delete pv;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO ObjPropVals (Id, Obj, Prop) VALUES (DEFAULT, ?, ?)"
        " RETURNING Id;");
    q.bindValue (0, m_Id);
    for (ObjPropVal *pv : m_Prop){
      if ((pv->st & 2) != 0){
        q.bindValue (1, pv->pr);
        if (!q.exec () || !q.next ())
          throw 1;
        pv->id = q.value (0).toUInt ();
        pv->st &= 0x38;
      }
    }
    for (ObjPropVal *pv : m_Prop){
      if ((pv->st & 0x38) != 0){
        tp = quint8(pv->tp);
        if (!pv->val.isValid () || pv->val.isNull ())
          pv->st |= 0x20;
        q.clear ();
        if ((pv->st & 0x20) != 0){
          q.prepare ("DELETE FROM " + m_Keeper->valTableName (tp) +
              " WHERE Id=:id;");
          q.bindValue (":id", pv->id);
          if (!q.exec ())
            throw 1;
        }
        else if ((pv->st & 0x10) != 0){
          m_Keeper->valueToVariant (pv->val, qv, tp);
          q.prepare ("INSERT INTO " + m_Keeper->valTableName (tp) +
              " (Id, Val) VALUES (?, ?);");
          q.bindValue (0, pv->id);
          q.bindValue (1, qv);
          if (!q.exec ())
            throw 1;
        }
        else if ((pv->st & 0x08) != 0){
          m_Keeper->valueToVariant (pv->val, qv, tp);
          q.prepare ("UPDATE " + m_Keeper->valTableName (tp) +
              " SET Val=:val WHERE Id=:id;");
          q.bindValue (":val", qv);
          q.bindValue (":id", pv->id);
          if (!q.exec ())
            throw 1;
        }
        pv->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog (QString ("ItkPgObject | saveProperties | Объект %1. Ошибка "
          "при сохранении свойств!").arg (m_Num), 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
Ui8Set& ItkPgObject::abilities (bool rd)
{
  if (rd || (m_ObjInit & 0x0008) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    quint8 ab;

    try{
      if (!m_Id || !db.isValid () || !db.isOpen ())
        throw 1;
      m_Abil.clear ();
      m_ObjInit &= 0xfff7;
      q.prepare ("SELECT Abil FROM ObjAbilities WHERE Obj=:id ORDER BY Abil");
      q.bindValue (":id", m_Id);
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        ab = quint8(q.value (0).toUInt ());
        if (!m_Abil.contains (ab))
          m_Abil.insert (ab);
      }
      m_ObjInit |= 0x0008;
    }
    catch (...){
    }
  }
  return m_Abil;
}

// -----------------------------------------------------------------------------
void ItkPgObject::readAbilities (Abilities &alst)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Ability *ab;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    if (m_Id){
      q.prepare ("SELECT a.Id, a.Name FROM ObjAbilities o, Abilities a WHERE o.Obj=:id"
          " AND a.Id=o.Abil ORDER BY a.Id");
      q.bindValue (":id", m_Id);
    }
    else{
      q.prepare ("SELECT a.Id, a.Name FROM TypeAbilities t, Abilities a WHERE t.Tp=:tp"
          " AND a.Id=t.Abil");
      q.bindValue (":tp", m_Type);
    }
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ab = new Ability;
      ab->id = quint8(q.value (0).toUInt ());
      ab->name = q.value (1).toString ();
      if ((m_st & 2) != 0)
        ab->st = 2;
      alst.push_back (ab);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool ItkPgObject::saveAbilities (Abilities &alst)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Abilities::iterator it;
  Ability *ab;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ObjAbilities WHERE Obj=:obj AND Abil=:ab;");
    q.bindValue (":obj", m_Id);
    it = alst.begin ();
    while (it != alst.end ()){
      ab = *it;
      if ((ab->st & 4) != 0){
        q.bindValue (":ab", ab->id);
        if (!q.exec ())
          throw 1;
        it = alst.erase (it);
        delete ab;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO ObjAbilities (Obj, Abil) VALUES (?, ?);");
    q.bindValue (0, m_Id);
    for (Ability *ab :alst){
      if ((ab->st & 2) != 0){
        q.bindValue (1, ab->id);
        if (!q.exec ())
          throw 1;
        ab->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void ItkPgObject::readContent (Item *item, QSqlDatabase &db)
{
  QSqlQuery q (db);
  Item *itm;
  quint8 ft, fv;

  try{
    q.prepare ("SELECT Ftr, FtVal FROM ItemFeatures WHERE Itm=:id");
    q.bindValue (":id", item->m_Id);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ft = quint8(q.value (0).toUInt ());
      fv = quint8(q.value (1).toUInt ());
      if (!item->m_Features.contains (ft))
        item->m_Features.insert (ft, fv);
    }
    q.clear ();
    q.prepare ("SELECT i.Id, i.Name, i.Num, i.Tp, t.Name FROM Items i, Types t"
        " WHERE i.Parent=:par AND t.Id=i.Tp ORDER BY i.Num");
    q.bindValue (":par", item->m_Id);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      itm = new Item (m_Keeper);
      itm->m_Parent = item->m_Id;
      itm->m_Id = q.value (0).toUInt ();
      itm->m_Name = q.value (1).toString ();
      itm->m_Num = q.value (2).toUInt ();
      itm->m_Type = quint16(q.value (3).toUInt ());
      itm->m_TypeName = q.value (4).toString ();
      item->m_Content.push_back (itm);
    }
    for (Item *itm : item->m_Content)
      readContent (itm, db);
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void ItkPgObject::saveContent (Item *item, QSqlDatabase &db)
{
  QSqlQuery q (db);
  ItemList::iterator it;
  FeatureSet::iterator it2;
  Item *itm;
  quint8 ft, fv;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Items WHERE Id=:id;");
    it = item->m_Content.begin ();
    while (it != item->m_Content.end ()){
      itm = *it;
      if ((itm->m_st & 4) != 0){
        q.bindValue (":id", itm->m_Id);
        if (!q.exec ())
          throw 1;
        it = item->m_Content.erase (it);
        delete itm;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Items SET Parent=:par, Name=:name, Num=:num, Tp=:tp WHERE Id=:id;");
    q.bindValue (":par", item->m_Id);
    for (Item *itm : item->m_Content){
      if ((itm->m_st & 3) == 1){
        q.bindValue (":name", itm->m_Name);
        q.bindValue (":num", itm->m_Num);
        q.bindValue (":tp", itm->m_Type);
        q.bindValue (":id", itm->m_Id);
        if (!q.exec ())
          throw 1;
        itm->m_Parent = item->m_Id;
        itm->m_st &= 0xf8;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Items (Id, Parent, Name, Num, Tp) VALUES (DEFAULT,"
        " ?, ?, ?, ?) RETURNING Id;");
    q.bindValue (0, item->m_Id);
    for (Item *itm : item->m_Content){
      if ((itm->m_st & 2) != 0){
        q.bindValue (1, itm->m_Name);
        q.bindValue (2, itm->m_Num);
        q.bindValue (3, itm->m_Type);
        if (!q.exec () || !q.next ())
          throw 1;
        itm->m_Id = q.value (0).toUInt ();
        itm->m_Parent = item->m_Id;
        itm->m_st &= 0xf8;
      }
    }
    for (Item *itm : item->m_Content){
      if ((itm->m_st & 8) != 0){
        q.clear ();
        q.prepare ("DELETE FROM ItemFeatures WHERE Itm=:id;");
        q.bindValue (":id", itm->m_Id);
        if (!q.exec ())
          throw 1;
        q.clear ();
        q.prepare ("INSERT INTO ItemFeatures (Itm, Ftr, FtVal) VALUES (?, ?, ?);");
        q.bindValue (0, itm->m_Id);
        it2 = itm->m_Features.begin ();
        while (it2 != itm->m_Features.end ()){
          ft = it2.key ();
          fv = it2.value ();
          q.bindValue (1, ft);
          q.bindValue (2, fv);
          if (!q.exec ())
            throw 1;
        }
        itm->m_st &= 0xf7;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  if (res){
    for (Item *itm : item->m_Content)
      saveContent (itm, db);
  }
}

// -----------------------------------------------------------------------------
bool ItkPgObject::getLogger ()
{
  return (m_LHost != nullptr && (m_Log = m_LHost->GetLogger (&m_LogFlags)) != nullptr);
}

// -----------------------------------------------------------------------------
const char* TechPgDataKeeper::m_ObjVersion = "1.0.0.4";
const char* TechPgDataKeeper::m_ObjVersionDate = "16.04.2024";

// -----------------------------------------------------------------------------
// хранилище данных ИТК-ОС ЦД
// -----------------------------------------------------------------------------
TechPgDataKeeper::TechPgDataKeeper (const KeeperDataInit &di)
  : ThSrvDataKeeper(di.host), m_BsName(di.bsname), m_ExtDir(di.extfld), m_BsAddr(di.addr),
    m_BsPort(di.port)
{
  m_WrkDir = QDir::currentPath ();
  m_WrkDir.replace ('\\', '/');
  if (!m_WrkDir.endsWith ('/'))
    m_WrkDir.push_back ('/');
  m_IniFileName = m_WrkDir + "TechCtrlServ.ini";
  m_LHost = dynamic_cast<ILoggerHost*>(di.host);
  if (getLogger ()){
    m_Log->writeLog ("TechPgDataKeeper | TechPgDataKeeper | Обработчик данных версии " +
        QString (m_ObjVersion) + " от " + QString (m_ObjVersionDate) + " создан.", 0x11);
    m_Log->writeLog ("TechPgDataKeeper | TechPgDataKeeper | Задано имя базы данных '" +
        m_BsName + "'.", 0x11);
  }
}

// -----------------------------------------------------------------------------
TechPgDataKeeper::~TechPgDataKeeper ()
{
  if (m_InitTimer)
    delete m_InitTimer;
  QSqlDatabase db = QSqlDatabase::database ("itk");
  getLogger ();
  if (db.isValid () && db.isOpen ()){
    db.close ();
    if (m_Log)
      m_Log->writeLog ("TechPgDataKeeper | ~TechPgDataKeeper | База данных '" +
          db.databaseName () + "' закрыта.", 0x15);
  }
  if (m_Log)
    m_Log->writeLog ("TechPgDataKeeper | ~TechPgDataKeeper | Обработчик данных "
        "удален.", 0x15);
}

static const char *buf1 {"jNCefdoMaigBLh"}, *buf2 {"93847214241305"};
// -----------------------------------------------------------------------------
void TechPgDataKeeper::init ()
{
  QSqlDatabase db = QSqlDatabase::addDatabase ("QPSQL", "itk");
  QString str;
  QByteArray ba0, ba;
  const char *buf;
  bool fl = true;

  m_ObjInit = 0;
  m_TryOpenCounter = 0;
  m_PassWrd.clear ();
  {
    QSettings st (m_IniFileName, QSettings::IniFormat);
    st.beginGroup ("Common");
    str = st.value ("User").toString ();
    st.endGroup ();
  }
  ba0 = str.toUtf8 ();
  try{
    if (ba0.size () != 14)
      throw 1;
    buf = ba0.data ();
    for (int i = 0; i < 14; ++ i){
      if (buf[i] < 0x41)
        fl = false;
      if (fl){
        if (buf[i] < buf1[i])
          throw 1;
        ba += buf[i] - buf1[i] + 0x30;
      }
      else if (buf[i] != buf2[i])
        throw 1;
    }
    m_PassWrd = ba;
  }
  catch (...){
  }
  clearData ();
  getLogger ();
  try{
    if (!m_InitTimer){
      m_InitTimer = new QTimer;
      if (!m_InitTimer)
        throw 1;
      m_InitTimer->setSingleShot (true);
      m_InitTimer->setInterval (3000);
      connect (m_InitTimer, &QTimer::timeout, this, &TechPgDataKeeper::slotTimer);
    }
    if (!db.isValid ()){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | init | Отсутствует драйвер БД!", 0x15, 6);
      throw 1;
    }
    if (m_BsName.isEmpty ()){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | init | Отсутствует имя БД!", 0x15, 6);
      throw 1;
    }
    db.setDatabaseName (m_BsName);
    db.setHostName (m_BsAddr);
    db.setPort (m_BsPort);
    db.setUserName ("postgres");
    if (!m_PassWrd.isEmpty ())
      db.setPassword (m_PassWrd);
    slotTimer ();
  }
  catch (...){
    if (db.isOpen ()){
      db.close ();
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | init | База данных '" + db.databaseName () +
            "' закрыта.", 0x15);
    }
    emit onInitFinished (-2);
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::createNewBase (const QString &name)
{
  bool res = false;

  getLogger ();
  try{
    QSqlDatabase db = QSqlDatabase::addDatabase ("QPSQL", "itk");
    db.setDatabaseName ("postgres");
    db.setHostName (m_BsAddr);
    db.setPort (m_BsPort);
    db.setUserName ("postgres");
    if (!m_PassWrd.isEmpty ())
      db.setPassword (m_PassWrd);
    if (!db.open ())
      throw 1;
    QSqlQuery q (db);
    //if (!q.exec ("CREATE DATABASE \"" + name + "\" OWNER postgres TEMPLATE postgres;")){
    if (!q.exec ("CREATE DATABASE \"" + name + "\" OWNER postgres ENCODING = 'UTF8';")){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | createNewBase | Не могу создать БД '" +
            name + "'!", 0x15, 6);
      throw 1;
    }
    if (m_Log)
      m_Log->writeLog ("TechPgDataKeeper | createNewBase | База данных '" + name +
          "' создана.", 0x15);
    res = true;
  }
  catch (...){
  }
  if (res)
    QSqlDatabase::removeDatabase ("itk");
  return res;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::reloadBase (const QString &name)
{
  m_ObjInit = 0;
  {
    QSqlDatabase::removeDatabase ("itk");
  }
  m_BsName = name;
  init ();
  return true;
}

// -----------------------------------------------------------------------------
Features& TechPgDataKeeper::features (bool rd)
{
  if (rd || (m_ObjInit & 0x0004) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    Feature *ft;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Features);
      m_Features.clear ();
      m_ObjInit &= 0xfffb;
      q.prepare ("SELECT Id, Name FROM Features ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        ft = new Feature;
        ft->id = quint8(q.value (0).toUInt ());
        ft->name = q.value (1).toString ();
        m_Features.push_back (ft);
      }
      m_ObjInit |= 0x0004;
    }
    catch (...){
    }
  }
  return m_Features;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveFeatures ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Features::iterator it;
  Feature *ft;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Features WHERE Id=:id;");
    it = m_Features.begin ();
    while (it != m_Features.end ()){
      ft = *it;
      if ((ft->st & 4) != 0){
        q.bindValue (":id", ft->id);
        if (!q.exec ())
          throw 1;
        it = m_Features.erase (it);
        delete ft;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Features SET Name=:name WHERE Id=:id;");
    for (Feature *ft : m_Features){
      if ((ft->st & 3) == 1){
        q.bindValue (":name", ft->name);
        q.bindValue (":id", ft->id);
        if (!q.exec ())
          throw 1;
        ft->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Features (Id, Name) VALUES (?, ?);");
    for (Feature *ft : m_Features){
      if ((ft->st & 2) != 0){
        q.bindValue (0, ft->id);
        q.bindValue (1, ft->name);
        if (!q.exec ())
          throw 1;
        ft->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
Abilities& TechPgDataKeeper::abilities (bool rd)
{
  if (rd || (m_ObjInit & 0x0008) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    Ability *ab;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Abilities);
      m_Abilities.clear ();
      m_ObjInit &= 0xfff7;
      q.prepare ("SELECT Id, Name FROM Abilities ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        ab = new Ability;
        ab->id = quint8(q.value (0).toUInt ());
        ab->name = q.value (1).toString ();
        m_Abilities.push_back (ab);
      }
      m_ObjInit |= 0x0008;
    }
    catch (...){
    }
  }
  return m_Abilities;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveAbilities ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Abilities::iterator it;
  Ability *ab;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Abilities WHERE Id=:id;");
    it = m_Abilities.begin ();
    while (it != m_Abilities.end ()){
      ab = *it;
      if ((ab->st & 4) != 0){
        q.bindValue (":id", ab->id);
        if (!q.exec ())
          throw 1;
        it = m_Abilities.erase (it);
        delete ab;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Abilities SET Name=:name WHERE Id=:id;");
    for (Ability *ab : m_Abilities){
      if ((ab->st & 3) == 1){
        q.bindValue (":name", ab->name);
        q.bindValue (":id", ab->id);
        if (!q.exec ())
          throw 1;
        ab->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Abilities (Id, Name) VALUES (?, ?);");
    for (Ability *ab : m_Abilities){
      if ((ab->st & 2) != 0){
        q.bindValue (0, ab->id);
        q.bindValue (1, ab->name);
        if (!q.exec ())
          throw 1;
        ab->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
Channels& TechPgDataKeeper::channels (bool rd)
{
  if (rd || (m_ObjInit & 0x0020) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    Channel *ch;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Channels);
      m_Channels.clear ();
      m_ObjInit &= 0xffdf;
      q.prepare ("SELECT Id, Name, DNum, Tp FROM Channels ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        ch = new Channel;
        ch->cid = char(q.value (0).toInt ());
        ch->name = q.value (1).toString ();
        ch->defnum = quint8(q.value (2).toUInt ());
        ch->tp = quint8(q.value (3).toUInt ());
        m_Channels.push_back (ch);
      }
      m_ObjInit |= 0x0020;
    }
    catch (...){
    }
  }
  return m_Channels;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveChannels ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Channels::iterator it;
  Channel *ch;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Channels WHERE Id=:id;");
    it = m_Channels.begin ();
    while (it != m_Channels.end ()){
      ch = *it;
      if ((ch->st & 4) != 0){
        q.bindValue (":id", ch->cid);
        if (!q.exec ())
          throw 1;
        it = m_Channels.erase (it);
        delete ch;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Channels SET Name=:nm, DNum=:dn, Tp=:tp WHERE Id=:id;");
    for (Channel *ch : m_Channels){
      if ((ch->st & 3) == 1){
        q.bindValue (":name", ch->name);
        q.bindValue (":dn", ch->defnum);
        q.bindValue (":tp", ch->tp);
        q.bindValue (":id", ch->cid);
        if (!q.exec ())
          throw 1;
        ch->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (?, ?, ?, ?);");
    for (Channel *ch : m_Channels){
      if ((ch->st & 2) != 0){
        q.bindValue (0, ch->cid);
        q.bindValue (1, ch->name);
        q.bindValue (2, ch->defnum);
        q.bindValue (4, ch->tp);
        if (!q.exec ())
          throw 1;
        ch->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
Properties& TechPgDataKeeper::properties (bool rd)
{
  if (rd || (m_ObjInit & 0x0001) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    Property *pr;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Properties);
      m_Properties.clear ();
      m_ObjInit &= 0xfffe;
      q.prepare ("SELECT Id, Name, VTp, Vs FROM Properties ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        pr = new Property;
        pr->id = q.value (0).toUInt ();
        pr->name = q.value (1).toString ();
        pr->type = ValType(q.value (2).toUInt ());
        pr->vs = q.value (3).toBool ();
        m_Properties.push_back (pr);
      }
      m_ObjInit |= 0x0001;
    }
    catch (...){
      if (getLogger ())
        m_Log->writeLog ("TechPgDataKeeper | properties | Ошибка при считывании "
            "свойств!", 0x15, 6);
    }
  }
  return m_Properties;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveProperties ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Properties::iterator it;
  Property *pr;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Properties WHERE Id=:id;");
    it = m_Properties.begin ();
    while (it != m_Properties.end ()){
      pr = *it;
      if ((pr->st & 4) != 0){
        q.bindValue (":id", pr->id);
        if (!q.exec ())
          throw 1;
        it = m_Properties.erase (it);
        delete pr;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Properties SET Name=:name, VTp=:tp, Vs=:vs WHERE Id=:id;");
    for (Property *pr : m_Properties){
      if ((pr->st & 3) == 1){
        q.bindValue (":name", pr->name);
        q.bindValue (":tp", pr->type);
        q.bindValue (":vs", pr->vs);
        q.bindValue (":id", pr->id);
        if (!q.exec ())
          throw 1;
        pr->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Properties (Id, Name, VTp, Vs) VALUES (?, ?, ?, ?);");
    for (Property *pr : m_Properties){
      if ((pr->st & 2) != 0){
        q.bindValue (0, pr->id);
        q.bindValue (1, pr->name);
        q.bindValue (2, pr->type);
        q.bindValue (3, pr->vs);
        if (!q.exec ())
          throw 1;
        pr->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveProperties | Ошибка при сохранении"
          " свойств!", 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
ObjTypes& TechPgDataKeeper::types (bool rd)
{
  if (rd || (m_ObjInit & 0x0400) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    ObjType *tp;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Types);
      m_Types.clear ();
      m_ObjInit &= 0xfbff;
      q.prepare ("SELECT Id, Name FROM Types ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        tp = new ObjType;
        tp->id = quint16(q.value (0).toUInt ());
        tp->name = q.value (1).toString ();
        m_Types.push_back (tp);
      }
      m_ObjInit |= 0x0400;
    }
    catch (...){
    }
  }
  return m_Types;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveTypes ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ObjTypes::iterator it;
  ObjType *tp;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Types WHERE Id=:id;");
    it = m_Types.begin ();
    while (it != m_Types.end ()){
      tp = *it;
      if ((tp->st & 4) != 0){
        q.bindValue (":id", tp->id);
        if (!q.exec ())
          throw 1;
        it = m_Types.erase (it);
        delete tp;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Types SET Name=:name WHERE Id=:id;");
    for (ObjType *tp : m_Types){
      if ((tp->st & 3) == 1){
        q.bindValue (":name", tp->name);
        q.bindValue (":id", tp->id);
        if (!q.exec ())
          throw 1;
        tp->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Types (Id, Name) VALUES (?, ?);");
    for (ObjType *tp : m_Types){
      if ((tp->st & 2) != 0){
        q.bindValue (0, tp->id);
        q.bindValue (1, tp->name);
        if (!q.exec ())
          throw 1;
        tp->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
SrvEvents& TechPgDataKeeper::srvEvents (bool rd)
{
  if (rd || (m_ObjInit & 0x0040) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    EventType *evtp;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Events);
      m_Events.clear ();
      m_ObjInit &= 0xffbf;
      q.prepare ("SELECT Id, Name, Rec FROM EventTypes ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        evtp = new EventType;
        evtp->id = quint16(q.value (0).toUInt ());
        evtp->name = q.value (1).toString ();
        evtp->rec = q.value (2).toBool ();
        m_Events.push_back (evtp);
      }
      m_ObjInit |= 0x0040;
    }
    catch (...){
    }
  }
  return m_Events;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveSrvEvents ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  SrvEvents::iterator it;
  EventType *evtp;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM EventTypes WHERE Id=:id;");
    it = m_Events.begin ();
    while (it != m_Events.end ()){
      evtp = *it;
      if ((evtp->st & 4) != 0){
        q.bindValue (":id", evtp->id);
        if (!q.exec ())
          throw 1;
        it = m_Events.erase (it);
        delete evtp;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE EventTypes SET Name=:name, Rec=:rec WHERE Id=:id;");
    for (EventType *et : m_Events){
      if ((et->st & 3) == 1){
        q.bindValue (":name", et->name);
        q.bindValue (":rec", et->rec);
        q.bindValue (":id", et->id);
        if (!q.exec ())
          throw 1;
        et->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO EventTypes (Id, Name, Rec) VALUES (?, ?, ?);");
    for (EventType *et : m_Events){
      if ((et->st & 2) != 0){
        q.bindValue (0, et->id);
        q.bindValue (1, et->name);
        q.bindValue (2, et->rec);
        if (!q.exec ())
          throw 1;
        et->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveSrvEvents | Ошибка при сохранении"
          " списка типов событий!", 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
ItkSystems& TechPgDataKeeper::itkSystems (bool rd)
{
  if (rd || (m_ObjInit & 0x0800) == 0){
    QSqlDatabase db = QSqlDatabase::database ("itk");
    QSqlQuery q (db);
    ItkSystem *sys;

    try{
      if (!db.isValid () || !db.isOpen ())
        throw 1;
      qDeleteAll (m_Systems);
      m_Systems.clear ();
      m_ObjInit &= 0xf7ff;
      q.prepare ("SELECT Id, Name FROM Systems ORDER BY Id");
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        sys = new ItkSystem;
        sys->id = quint8(q.value (0).toUInt ());
        sys->name = q.value (1).toString ();
        m_Systems.push_back (sys);
      }
      m_ObjInit |= 0x0800;
    }
    catch (...){
    }
  }
  return m_Systems;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveItkSystems ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ItkSystems::iterator it;
  ItkSystem *sys;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Systems WHERE Id=:id;");
    it = m_Systems.begin ();
    while (it != m_Systems.end ()){
      sys = *it;
      if ((sys->st & 4) != 0){
        q.bindValue (":id", sys->id);
        if (!q.exec ())
          throw 1;
        it = m_Systems.erase (it);
        delete sys;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Systems SET Name=:name WHERE Id=:id;");
    for (ItkSystem *sys : m_Systems){
      if ((sys->st & 3) == 1){
        q.bindValue (":name", sys->name);
        q.bindValue (":id", sys->id);
        if (!q.exec ())
          throw 1;
        sys->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO Systems (Id, Name) VALUES (?, ?);");
    for (ItkSystem *sys : m_Systems){
      if ((sys->st & 2) != 0){
        q.bindValue (0, sys->id);
        q.bindValue (1, sys->name);
        if (!q.exec ())
          throw 1;
        sys->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readTypeAbilities (Abilities &alst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Ability *ab;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (alst);
    alst.clear ();
    q.prepare ("SELECT a.Id, a.Name FROM TypeAbilities t, Abilities a "
        "WHERE t.Tp=:tp AND a.Id=t.Abil ORDER BY a.Id");
    q.bindValue (":tp", tp);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ab = new Ability;
      ab->id = quint8(q.value (0).toUInt ());
      ab->name = q.value (1).toString ();
      alst.push_back (ab);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveTypeAbilities (Abilities &alst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Abilities::iterator it;
  Ability *ab;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM TypeAbilities WHERE Tp=:tp AND Abil=:abil;");
    q.bindValue (":tp", tp);
    it = alst.begin ();
    while (it != alst.end ()){
      ab = *it;
      if ((ab->st & 4) != 0){
        q.bindValue (":abil", ab->id);
        if (!q.exec ())
          throw 1;
        it = alst.erase (it);
        delete ab;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO TypeAbilities (Tp, Abil) VALUES (?, ?);");
    q.bindValue (0, tp);
    for (Ability *ab : alst){
      if ((ab->st & 2) != 0){
        q.bindValue (1, ab->id);
        if (!q.exec ())
          throw 1;
        ab->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readTypeFeatures (Features &ftlst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Feature *ft;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (ftlst);
    ftlst.clear ();
    q.prepare ("SELECT f.Id, f.Name FROM TypeFeatures t, Features f "
        "WHERE t.Tp=:tp AND f.Id=t.Ftr ORDER BY f.Id");
    q.bindValue (":tp", tp);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ft = new Feature;
      ft->id = quint8(q.value (0).toUInt ());
      ft->name = q.value (1).toString ();
      ftlst.push_back (ft);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveTypeFeatures (Features &ftlst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Features::iterator it;
  Feature *ft;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM TypeFeatures WHERE Tp=:tp AND Ftr=:ft;");
    q.bindValue (":tp", tp);
    it = ftlst.begin ();
    while (it != ftlst.end ()){
      ft = *it;
      if ((ft->st & 4) != 0){
        q.bindValue (":ft", ft->id);
        if (!q.exec ())
          throw 1;
        it = ftlst.erase (it);
        delete ft;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO TypeFeatures (Tp, Ftr) VALUES (?, ?);");
    q.bindValue (0, tp);
    for (Feature *ft : ftlst){
      if ((ft->st & 2) != 0){
        q.bindValue (1, ft->id);
        if (!q.exec ())
          throw 1;
        ft->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readTypeChannels (Channels &clst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Channel *ch;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (clst);
    clst.clear ();
    q.prepare ("SELECT c.Id, c.Name, c.DNum, c.Tp FROM TypeChannels t, Channels c"
        " WHERE t.Tp=:tp AND c.Id=t.Chan ORDER BY c.Id");
    q.bindValue (":tp", tp);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ch = new Channel;
      ch->cid = char(q.value (0).toInt ());
      ch->name = q.value (1).toString ();
      ch->defnum = quint8(q.value (2).toUInt ());
      ch->tp = quint8(q.value (3).toUInt ());
      clst.push_back (ch);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveTypeChannels (Channels &clst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Channels::iterator it;
  Channel *ch;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM TypeChannels WHERE Tp=:tp AND Chan=:chan;");
    q.bindValue (":tp", tp);
    it = clst.begin ();
    while (it != clst.end ()){
      ch = *it;
      if ((ch->st & 4) != 0){
        q.bindValue (":chan", int(ch->cid));
        if (!q.exec ())
          throw 1;
        it = clst.erase (it);
        delete ch;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO TypeChannels (Tp, Chan) VALUES (?, ?);");
    q.bindValue (0, tp);
    for (Channel *ch : clst){
      if ((ch->st & 2) != 0){
        q.bindValue (1, int(ch->cid));
        if (!q.exec ())
          throw 1;
        ch->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readTypeProperties (Properties &plst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Property *pr;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (plst);
    plst.clear ();
    q.prepare ("SELECT p.Id, p.Name, p.VTp, p.Vs FROM TypeProperties t, Properties p"
        " WHERE t.Tp=:tp AND p.Id=t.Prop ORDER BY p.Id");
    q.bindValue (":tp", tp);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      pr = new Property;
      pr->id = q.value (0).toUInt ();
      pr->name = q.value (1).toString ();
      pr->type = ValType(q.value (2).toUInt ());
      pr->vs = q.value (3).toBool ();
      plst.push_back (pr);
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readTypeProperties | Ошибка при считывании"
          " свойств типа объекта!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveTypeProperties (Properties &plst, quint16 tp)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Properties::iterator it;
  Property *pr;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM TypeProperties WHERE Tp=:tp AND Prop=:pr;");
    q.bindValue (":tp", tp);
    it = plst.begin ();
    while (it != plst.end ()){
      pr = *it;
      if ((pr->st & 4) != 0){
        q.bindValue (":pr", pr->id);
        if (!q.exec ())
          throw 1;
        it = plst.erase (it);
        delete pr;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO TypeProperties (Tp, Prop) VALUES (?, ?);");
    q.bindValue (0, tp);
    for (Property *pr : plst){
      if ((pr->st & 2) != 0){
        q.bindValue (1, pr->id);
        if (!q.exec ())
          throw 1;
        pr->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveTypeProperties | Ошибка при сохранении"
          " свойств типа объекта!", 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::propValueSet (PropValList &vs, quint16 pr)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  PropVal *pv;
  QVariant qv;
  quint8 tp = 0, i;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    qDeleteAll (vs);
    vs.clear ();
    q.prepare ("SELECT VTp, Vs FROM Properties WHERE Id=:id");
    q.bindValue (":id", pr);
    if (!q.exec () || !q.next ())
      throw 1;
    tp = q.value (0).toUInt ();
    if (q.value (1).toBool () && tp != 0){
      q.clear ();
      q.prepare ("SELECT s.Id, s.Num, v.Val FROM PropValSets s, " + valTableName (tp) +
          " v WHERE s.Prop=:pr AND v.Id=s.Id ORDER BY s.Num, s.Id");
      q.bindValue (":pr", pr);
      if (!q.exec ())
        throw 1;
      i = 1;
      while (q.next ()){
        pv = new PropVal;
        pv->tp = ValType(tp);
        pv->id = q.value (0).toUInt ();
        pv->num = q.value (1).toUInt ();
        qv = q.value (2);
        if (qv.isValid () && !qv.isNull ())
          variantToValue (qv, pv->val, tp);
        if (pv->num != i){
          pv->num = i;
          pv->st |= 1;
        }
        vs.push_back (pv);
        i ++;
      }
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | propValueSet | Ошибка при считывании "
          "набора значений!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::savePropValueSet (PropValList &vs, quint16 pr)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  PropValList::iterator it;
  QVariant qv;
  PropVal *pv;
  quint8 tp = 0;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("SELECT VTp, Vs FROM Properties WHERE Id=:id;");
    q.bindValue (":id", pr);
    if (!q.exec () || !q.next ())
      throw 1;
    tp = q.value (0).toUInt ();
    if (!q.value (1).toBool () || !tp)
      throw 1;
    q.clear ();
    q.prepare ("DELETE FROM PropValSets WHERE Id=:id;");
    it = vs.begin ();
    while (it != vs.end ()){
      pv = *it;
      if ((pv->st & 4) != 0){
        q.bindValue (":id", pv->id);
        if (!q.exec ())
          throw 1;
        it = vs.erase (it);
        delete pv;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("INSERT INTO PropValSets (Id, Num, Prop) VALUES (DEFAULT, ?, ?) RETURNING Id;");
    q.bindValue (1, pr);
    for (PropVal *pv : vs){
      if ((pv->st & 2) != 0){
        q.bindValue (0, pv->num);
        if (!q.exec () || !q.next ())
          throw 1;
        pv->id = q.value (0).toUInt ();
        pv->st &= 0x38;
      }
    }
    q.clear ();
    q.prepare ("UPDATE PropValSets SET Num=:num WHERE Id=:id;");
    for (PropVal *pv : vs){
      if ((pv->st & 3) == 1){
        q.bindValue (":num", pv->num);
        q.bindValue (":id", pv->id);
        if (!q.exec ())
          throw 1;
        pv->st &= 0x38;
      }
    }
    for (PropVal *pv : vs){
      if ((pv->st & 0x38) != 0){
        if (!pv->val.isValid () || pv->val.isNull ())
          pv->st &= 0x20;
        q.clear ();
        if ((pv->st & 0x20) != 0){
          q.prepare ("DELETE FROM " + valTableName (tp) + " WHERE Id=:id;");
          q.bindValue (":id", pv->id);
          if (!q.exec ())
            throw 1;
        }
        else if ((pv->st & 0x10) != 0){
          valueToVariant (pv->val, qv, tp);
          q.prepare ("INSERT INTO " + valTableName (tp) + " (Id, Val) VALUES (?, ?);");
          q.bindValue (0, pv->id);
          q.bindValue (1, qv);
          if (!q.exec ())
            throw 1;
        }
        else if ((pv->st & 0x08) != 0){
          valueToVariant (pv->val, qv, tp);
          q.prepare ("UPDATE " + valTableName (tp) + " SET Val=:val WHERE Id=:id;");
          q.bindValue (":val", qv);
          q.bindValue (":id", pv->id);
          if (!q.exec ())
            throw 1;
        }
        pv->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | savePropValueSet | Ошибка при созранении"
          " набора значений!", 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readObjects (ObjList &objlst, quint8 sys, bool act)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ItkPgObject *obj;
  QString str;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    qDeleteAll (objlst);
    objlst.clear ();
    str = "SELECT i.Id, i.Name, i.Num, i.Tp, t.Name, o.Syst, o.Act FROM Items i,"
        " Objects o, Types t WHERE o.Id=i.Id AND t.Id=i.Tp";
    if (sys)
      str += QString (" AND o.Syst=%1").arg (sys);
    if (act)
      str += " AND o.Act=TRUE";
    str += " ORDER BY Act DESC, i.Num";
    if (!q.exec (str))
      throw 1;
    while (q.next ()){
      obj = new ItkPgObject (this, this);
      obj->m_Id = q.value (0).toUInt ();
      obj->m_Name = q.value (1).toString ();
      obj->m_Num = q.value (2).toUInt ();
      obj->m_Type = quint16(q.value (3).toUInt ());
      obj->m_TypeName = q.value (4).toString ();
      obj->m_Syst = quint8(q.value (5).toUInt ());
      obj->m_Act = q.value (6).toBool ();
      objlst.push_back (obj);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveObjects (ObjList &objlst)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db), q1 (db);
  ObjList::iterator it;
  ItkObject *obj;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM Objects WHERE Id=:id;");
    it = objlst.begin ();
    while (it != objlst.end ()){
      obj = *it;
      if ((obj->m_st & 4) != 0){
        q.bindValue (":id", obj->m_Id);
        if (!q.exec ())
          throw 1;
        it = objlst.erase (it);
        delete obj;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE Items SET Name=:name, Num=:num, Tp=:tp WHERE Id=:id;");
    q1.prepare ("UPDATE Objects SET Syst=:sys, Act=:act WHERE Id=:id;");
    for (ItkObject *obj : objlst){
      if ((obj->m_st & 3) == 1){
        q.bindValue (":name", obj->m_Name);
        q.bindValue (":num", obj->m_Num);
        q.bindValue (":tp", obj->m_Type);
        q.bindValue (":id", obj->m_Id);
        if (!q.exec ())
          throw 1;
        q1.bindValue (":sys", obj->m_Syst);
        q1.bindValue (":act", obj->m_Act);
        q1.bindValue (":id", obj->m_Id);
        if (!q1.exec ())
          throw 1;
        obj->m_st &= 0xf8;
      }
    }
    q.clear ();
    q1.clear ();
    q.prepare ("INSERT INTO Items (Id, Name, Num, Tp) VALUES (DEFAULT, ?, ?, ?)"
        " RETURNING Id;");
    q1.prepare ("INSERT INTO Objects (Id, Syst, Act) VALUES (?, ?, ?);");
    for (ItkObject *obj : objlst){
      if ((obj->m_st & 2) != 0){
        q.bindValue (0, obj->m_Name);
        q.bindValue (1, obj->m_Num);
        q.bindValue (2, obj->m_Type);
        if (!q.exec () || !q.next ())
          throw 1;
        obj->m_Id = q.value (0).toUInt ();
        q1.bindValue (0, obj->m_Id);
        q1.bindValue (1, obj->m_Syst);
        q1.bindValue (2, obj->m_Act);
        if (!q1.exec ())
          throw 1;
        obj->m_st &= 0xf8;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
ItkObject* TechPgDataKeeper::readCurrentObject (quint32 id, quint32 num, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);

  if (m_CurObject){
    delete m_CurObject;
    m_CurObject = nullptr;
  }
  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    if (id){
      q.prepare ("SELECT i.Name, i.Num, i.Tp, o.Syst FROM Items i, Objects o"
          " WHERE i.Id=:id AND o.Id=i.Id");
      q.bindValue (":id", id);
      if (!q.exec () || !q.next ())
        throw 1;
      m_CurObject = new ItkPgObject (this, this);
      m_CurObject->m_Id = id;
      m_CurObject->m_Name = q.value (0).toString ();
      m_CurObject->m_Num = q.value (1).toUInt ();
      m_CurObject->m_Type = quint16(q.value (2).toUInt ());
      m_CurObject->m_Syst = quint8(q.value (3).toUInt ());
    }
    else{
      if (!num || !sys)
        return nullptr;
      q.prepare ("SELECT i.Id, i.Name, i.Tp FROM Objects o, Items i"
          " WHERE i.Num=:num AND o.Syst=:sys AND i.Id=o.Id");
      q.bindValue (":num", num);
      q.bindValue (":sys", sys);
      if (!q.exec () || !q.next ())
        throw 1;
      m_CurObject = new ItkPgObject (this, this);
      m_CurObject->m_Num = num;
      m_CurObject->m_Syst  = sys;
      m_CurObject->m_Id = q.value (0).toUInt ();
      m_CurObject->m_Name = q.value (1).toString ();
      m_CurObject->m_Type = quint16(q.value (2).toUInt ());
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readCurrentObject | Ошибка!", 0x15, 6);
  }
  return m_CurObject;
}

// -----------------------------------------------------------------------------
ItkObject* TechPgDataKeeper::readObject (quint32 num, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ItkObject *obj = nullptr;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    if (!num || !sys)
      throw 1;
    q.prepare ("SELECT i.Id, i.Name, i.Tp FROM Objects o, Items i"
        " WHERE i.Num=:num AND o.Syst=:sys AND i.Id=o.Id");
    q.bindValue (":num", num);
    q.bindValue (":sys", sys);
    if (!q.exec () || !q.next ())
      throw 1;
    obj = new ItkPgObject (this, this);
    obj->m_Num = num;
    obj->m_Syst  = sys;
    obj->m_Id = q.value (0).toUInt ();
    obj->m_Name = q.value (1).toString ();
    obj->m_Type = quint16(q.value (2).toUInt ());
    obj->m_Act = true;
  }
  catch (...){
  }
  return obj;
}

// -----------------------------------------------------------------------------
ItkObject* TechPgDataKeeper::createNewObject ()
{
  return new ItkPgObject (this, this);
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readContacts (ContObjects &cont, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ContObj *co;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    qDeleteAll (cont);
    cont.clear ();
    q.prepare ("SELECT c.Id, i.Id, i.Name, i.Num, i.Tp, t.Name, c.Lev FROM ContObjects c,"
        " Items i, Objects o, Types t WHERE c.Obj=:id AND i.Id=c.Cont AND o.Id=c.Cont"
        " AND t.Id=i.Tp ORDER BY c.Lev, i.Tp, i.Num");
    q.bindValue (":id", oid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      co = new ContObj;
      co->id = q.value (0).toUInt ();
      co->objid = q.value (1).toUInt ();
      co->name = q.value (2).toString ();
      co->num = q.value (3).toUInt ();
      co->tp = quint16(q.value (4).toUInt ());
      co->tpname = q.value (5).toString ();
      co->lev = quint8(q.value (6).toUInt ());
      cont.push_back (co);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveContacts (ContObjects &cont, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ContObjects::iterator it;
  ContObj *co;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ContObjects WHERE Id=:id;");
    it = cont.begin ();
    while (it != cont.end ()){
      co = *it;
      if ((co->st & 4) != 0){
        q.bindValue (":id", co->id);
        if (!q.exec ())
          throw 1;
        deleteContObject (db, oid, co->objid);
        it = cont.erase (it);
        delete co;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE ContObjects SET Lev=:lev WHERE Id=:id;");
    for (ContObj *co : cont){
      if ((co->st & 3) == 1){
        q.bindValue (":lev", co->lev);
        q.bindValue (":id", co->id);
        if (!q.exec ())
          throw 1;
        co->st &= 0xf8;
        updateContObject (db, oid, co->objid, co->lev);
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO ContObjects (Id, Obj, Cont, Lev) VALUES (DEFAULT,"
        " ?, ?, ?) RETURNING Id;");
    q.bindValue (0, oid);
    for (ContObj *co : cont){
      if ((co->st & 2) != 0){
        q.bindValue (1, co->objid);
        q.bindValue (2, co->lev);
        if (!q.exec () || !q.next ())
          throw 1;
        co->id = q.value (0).toUInt ();
        co->st &= 0xf8;
        addContObject (db, oid, co->objid, co->lev);
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveContacts | Ошибка при сохранении "
          "списка контактов!", 0x15, 6);
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::removeObject (quint32 num, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  quint32 id;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    q.exec ("BEGIN");
    q.prepare ("SELECT i.Id FROM Items i, Objects o WHERE i.Num=:num AND "
        "o.Syst=:sys AND i.Id=o.Id;");
    q.bindValue (":num", num);
    q.bindValue (":sys", sys);
    if (!q.exec () || !q.next ())
      throw 1;
    id = q.value (0).toUInt ();
    if (!id)
      throw 1;
    if (m_CurObject && m_CurObject->m_Num != num){
      q.clear ();
      q.prepare ("DELETE FROM Objects WHERE Id=:id;");
      q.bindValue (":id", id);
      if (!q.exec ())
        throw 1;
    }
    q.exec ("COMMIT");
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | removeObject | Ошибка при Удалении "
          "объекта!", 0x15, 6);
    q.exec ("ROLLBACK");
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readContactNumbers (ContNumbers &cont, quint32 onum, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db), q1 (db);
  ObjContNumber *cn;
  quint32 oid, cid;
  int cnt;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    qDeleteAll (cont);
    cont.clear ();
    q.prepare ("SELECT i.Id FROM Items i, Objects o WHERE i.Num=:num AND "
        "o.Id=i.Id AND o.Syst=:sys");
    q.bindValue (":num", onum);
    q.bindValue (":sys", sys);
    if (!q.exec () || !q.next ())
      throw 1;
    oid = q.value (0).toUInt ();
    q.clear ();
    q.prepare ("SELECT i.Id, i.Num, c.Lev FROM ContObjects c, Items i, Objects o "
        "WHERE c.Obj=:id AND i.Id=c.Cont AND o.Id=c.Cont ORDER BY c.Lev, i.Num");
    q.bindValue (":id", oid);
    q1.prepare ("SELECT COUNT(*) FROM ObjAbilities WHERE Obj=:id AND Abil=3");
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      cid = q.value (0).toUInt ();
      q1.bindValue (":id", cid);
      cn = new ObjContNumber;
      cn->num = q.value (1).toUInt ();
      cn->lev = quint8(q.value (2).toUInt ());
      if (q1.exec () && q1.next ()){
        cnt = q1.value (0).toInt ();
        if (cnt)
          cn->tp = 1;
      }
      cont.push_back (cn);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readContChannels (ObjChannels &chns, quint64 &contid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ObjChannel *chn;

  if (!db.isValid () || !db.isOpen ())
    return;
  getLogger ();
  try{
    qDeleteAll (chns);
    chns.clear ();
    q.prepare ("SELECT o.Id, o.Chan, o.Num, c.Name, o.Addr, o.Lev, o.Con FROM "
        "ContChannels o, Channels c WHERE o.Cont=:id AND c.Id=o.Chan ORDER BY "
        "o.Chan, o.Lev, o.Num");
    q.bindValue (":id", quint32(contid));
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      chn = new ObjChannel;
      chn->id = q.value (0).toUInt ();
      chn->ch = char(q.value (1).toUInt ());
      chn->num = quint8(q.value (2).toUInt ());
      chn->name = q.value (3).toString ();
      chn->addr = q.value (4).toString ();
      chn->lev = quint8(q.value (5).toUInt ());
      chn->con = q.value (6).toBool ();
      if (m_Log && (m_LogFlags & 4) != 0)
        m_Log->writeLog (QString ("TechPgDataKeeper | readContChannels | Считан"
            " канал id=%1 ch=%2 num=%3 addr=%4.").arg (chn->id).arg (chn->ch).
            arg (chn->num).arg (chn->addr), 0x15);
      chns.push_back (chn);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveContChannels (ObjChannels &chns, quint64 &contid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ObjChannels::iterator it;
  ObjChannel *chn;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ContChannels WHERE Id=:id;");
    it = chns.begin ();
    while (it != chns.end ()){
      chn = *it;
      if ((chn->st & 4) != 0){
        q.bindValue (":id", chn->id);
        if (!q.exec ())
          throw 1;
        it = chns.erase (it);
        delete chn;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE ContChannels SET Lev=:lev, Addr=:addr, Con=:con WHERE Id=:id;");
    for (ObjChannel *chn : chns){
      if ((chn->st & 3) == 1){
        q.bindValue (":lev", chn->lev);
        q.bindValue (":addr", chn->addr);
        q.bindValue (":con", chn->con);
        q.bindValue (":id", chn->id);
        if (!q.exec ())
          throw 1;
        chn->st &= 0xf8;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO ContChannels (Id, Cont, Chan, Num, Lev, Addr, Con) "
        "VALUES (DEFAULT, ?, ?, ?, ?, ?, ?) RETURNING Id;");
    q.bindValue (0, quint32(contid));
    for (ObjChannel *chn : chns){
      if ((chn->st & 2) != 0){
        q.bindValue (1, chn->ch);
        q.bindValue (2, chn->num);
        q.bindValue (3, chn->lev);
        q.bindValue (4, chn->addr);
        q.bindValue (5, chn->con);
        if (!q.exec () || !q.next ())
          throw 1;
        chn->id = q.value (0).toUInt ();
        chn->st &= 0xf8;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveContChannels | Ошибка при "
          "сохранении канала связи абонента!", 0x15, 6);
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readServChannels (ServChannels &chnls, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  SrvChannel *chn;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    if (!oid || !db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (chnls);
    chnls.clear ();
    q.prepare ("SELECT s.Id, s.Chan, s.Num, c.Name, s.Mode FROM ServChannels s, "
        "Channels c WHERE s.ObjId=:id AND c.Id=s.Chan ORDER BY s.Chan, s.Num");
    q.bindValue (":id", oid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      chn = new SrvChannel;
      chn->id = q.value (0).toUInt ();
      chn->ch = char(q.value (1).toUInt ());
      chn->num = quint8(q.value (2).toUInt ());
      chn->name = q.value (3).toString ();
      chn->mode = quint8(q.value (4).toUInt ());
      chnls.push_back (chn);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::contObjects (ObjList &objlst, Ui32List &clst, quint8 syst)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ItkPgObject *ao;
  QString str = "(";
  int i = 0;

  if (!db.isValid () || !db.isOpen ())
    return;
  try{
    for (quint32 id : clst){
      if (i)
        str += ", ";
      str += QString::number (id);
      i ++;
    }
    str += ")";
    str = "SELECT i.Id, i.Name, i.Num, i.Tp, o.Syst FROM Items i, Objects o WHERE"
        " i.Id NOT IN " + str + " AND o.Id=i.Id AND o.Act=TRUE";
    if (syst)
      str += QString (" AND o.Syst=%1 ORDER BY i.Num").arg (syst);
    else
      str += " ORDER BY o.Syst, i.Num";
    q.prepare (str);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ao = new ItkPgObject (this);
      ao->m_Id = q.value (0).toUInt ();
      ao->m_Name = q.value (1).toString ();
      ao->m_Num = q.value (2).toUInt ();
      ao->m_Type = quint16(q.value (3).toUInt ());
      ao->m_Syst = quint8(q.value (4).toUInt ());
      ao->m_Act = true;
      objlst.push_back (ao);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::updateObjCenter (quint32 num, quint32 cntnum, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  quint32 cn = 0;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    if (!num || !cntnum || !sys)
      throw 1;
    q.prepare ("SELECT CntNum FROM ObjCenters WHERE ObjNum=:num AND Syst=:sys");
    q.bindValue (":num", num);
    q.bindValue (":sys", sys);
    if (!q.exec ())
      throw 1;
    if (q.next ())
      cn = q.value (0).toUInt ();
    if (!cn){
      q.prepare ("INSERT INTO ObjCenters (ObjNum, CntNum, Syst) VALUES (?, ?, ?)");
      q.bindValue (0, num);
      q.bindValue (1, cntnum);
      q.bindValue (2, sys);
      if (!q.exec ())
        throw 1;
    }
    else if (cn != cntnum){
      q.prepare ("UPDATE ObjCenters SET CntNum=:cnum WHERE ObjNum=:num AND Syst=:sys");
      q.bindValue (":num", num);
      q.bindValue (":cnum", cntnum);
      q.bindValue (":sys", sys);
      if (!q.exec ())
        throw 1;
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
quint32 TechPgDataKeeper::getObjCenter (quint32 num, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  quint32 cnum = 0;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.prepare ("SELECT CntNum FROM ObjCenters WHERE ObjNum=:num AND Syst=:sys");
    q.bindValue (":num", num);
    q.bindValue (":sys", sys);
    if (!q.exec () || !q.next ())
      throw 1;
    cnum = q.value (0).toUInt ();
  }
  catch (...){
  }
  return cnum;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::getObjCenters (ObjCenters &cntrs, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  quint32 num, cnum;

  cntrs.clear ();
  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.prepare ("SELECT ObjNum, CntNum FROM ObjCenters WHERE ObjNum!=0 AND Syst=:sys");
    q.bindValue (":sys", sys);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      num = q.value (0).toUInt ();
      cnum = q.value (1).toUInt ();
      if (cntrs.find (num) == cntrs.end ())
        cntrs.insert (num, cnum);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readShells (ShellClients &shells, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ShellClient *sh;

  try{
    qDeleteAll (shells);
    shells.clear ();
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.prepare ("SELECT Id, Num, Addr, Con FROM ApplShells WHERE ObjId=:oid ORDER BY Num");
    q.bindValue (":oid", oid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      sh = new ShellClient;
      sh->id = q.value (0).toUInt ();
      sh->objid = oid;
      sh->num = quint8(q.value (1).toUInt ());
      sh->addr = q.value (2).toString ();
      sh->con = q.value (3).toBool ();
      shells.push_back (sh);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveShells (ShellClients &shells, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ShellClients::iterator it;
  ShellClient *sh;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ApplShells WHERE Id=:id;");
    it = shells.begin ();
    while (it != shells.end ()){
      sh = *it;
      if ((sh->st & 4) != 0){
        q.bindValue (":id", sh->id);
        if (!q.exec ())
          throw 1;
        it = shells.erase (it);
        delete sh;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE ApplShells SET Addr=:addr, Con=:con WHERE Id=:id;");
    for (ShellClient *sh : shells){
      if ((sh->st & 3) == 1){
        q.bindValue (":id", sh->id);
        q.bindValue (":addr", sh->addr);
        q.bindValue (":con", sh->con);
        if (!q.exec ())
          throw 1;
        sh->st &= 0xf0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO ApplShells (Id, ObjId, Num, Addr, Con) VALUES"
        " (DEFAULT, ?, ?, ?, ?) RETURNING Id;");
    q.bindValue (0, oid);
    for (ShellClient *sh : shells){
      if ((sh->st & 2) != 0){
        q.bindValue (1, sh->num);
        q.bindValue (2, sh->addr);
        q.bindValue (3, sh->con);
        if (!q.exec () || !q.next ())
          throw 1;
        sh->id = q.value (0).toUInt ();
        sh->st &= 0xf0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveShells | Ошибка при сохранении "
          "оболочки!", 0x15, 6);
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readRouteTable (AbnRoutTable &table, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  AbonentRout *ar;
  quint8 sys;

  try{
    qDeleteAll (table);
    table.clear ();
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.prepare ("SELECT Syst FROM Objects WHERE Id=:id");
    q.bindValue (":id", oid);
    if (!q.exec () || !q.next ())
      throw 1;
    sys = quint8(q.value (0).toUInt ());
    q.clear ();
    q.prepare ("SELECT i.Num, t.Id FROM AbnRouteTable t, Items i, Objects o "
        "WHERE o.Syst=:sys AND (o.Id=:id OR o.Id IN (SELECT c.Cont FROM "
        "ContObjects c WHERE c.Obj=:oid)) AND t.ObjId=o.Id AND i.Id=o.Id");
    q.bindValue (":sys", sys);
    q.bindValue (":id", oid);
    q.bindValue (":oid", oid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ar = new AbonentRout;
      ar->id = q.value (0).toUInt ();
      ar->num = q.value (1).toUInt ();
      table.push_back (ar);
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveRouteTable (AbnRoutTable &table, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db), q1 (db);
  AbnRoutTable::iterator it;
  AbonentRout *ar;
  quint32 id = 1;
  quint8 sys;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("SELECT Syst FROM Objects WHERE Id=:id;");
    q.bindValue (":id", oid);
    if (!q.exec () || !q.next ())
      throw 1;
    sys = quint8(q.value (0).toUInt ());
    q1.prepare ("SELECT i.Id FROM Items i, Objects o WHERE o.Id=i.Id AND "
        "i.Num=:num AND o.Syst=:sys;");
    q1.bindValue (":sys", sys);
    q.clear ();
    q.prepare ("DELETE FROM AbnRouteTable WHERE ObjId=:id;");
    it = table.begin ();
    while (it != table.end ()){
      ar = *it;
      if ((ar->st & 4) != 0){
        q1.bindValue (":num", ar->id);
        if (!q1.exec () || !q1.next ())
          throw 2;
        id = q1.value (0).toUInt ();
        q.bindValue (":id", id);
        if (!q.exec ())
          throw 3;
        it = table.erase (it);
        delete ar;
      }
      else
        it ++;
    }
    q.clear ();
    q.prepare ("UPDATE AbnRouteTable SET Id=:id WHERE ObjId=:oid;");
    for (AbonentRout *ar : table){
      if ((ar->st & 3) == 1){
        q1.bindValue (":num", ar->id);
        if (!q1.exec () || !q1.next ())
          throw 4;
        id = q1.value (0).toUInt ();
        q.bindValue (":oid", id);
        q.bindValue (":id", ar->num);
        if (!q.exec ())
          throw 5;
        ar->st = 0;
      }
    }
    q.clear ();
    q.prepare ("INSERT INTO AbnRouteTable (ObjId, Id) VALUES (?, ?);");
    for (AbonentRout *ar : table){
      if ((ar->st & 2) != 0){
        q1.bindValue (":num", ar->id);
        if (!q1.exec () || !q1.next ())
          throw 1;
        id = q1.value (0).toUInt ();
        q.bindValue (0, id);
        q.bindValue (1, ar->num);
        if (!q.exec ())
          throw 1;
        ar->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveRouteTable | Ошибка при сохранении"
          " таблицы маршрутизации!", 0x15, 6);
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readObjFtChanges (ObjFeatures &ftrs, quint32 oid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ObjFtChangeEvent *fcev;

  if (!db.isValid () || !db.isOpen ())
    return;
  qDeleteAll (ftrs);
  ftrs.clear ();
  try{
    q.prepare ("SELECT Dtm, ItId, Ft FROM ObjEventChanges WHERE ObjId=:oid AND"
        " Pres=TRUE");
    q.bindValue (":oid", oid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      fcev = new ObjFtChangeEvent;
      fcev->dtm = q.value (0).toDateTime ();
      fcev->objid = oid;
      fcev->itid = q.value (1).toUInt ();
      fcev->ft = quint16(q.value (2).toUInt ());
      fcev->pres = true;
      ftrs.push_back (fcev);
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readObjFtChanges | Ошибка при считывании"
          " события изменения признака!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::saveObjFtChange (ObjFtChangeEvent *fcev)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  bool res = false, fpres;

  try{
    if (!fcev)
      throw 1;
    if (!fcev->itid)
      fcev->itid = fcev->objid;
    q.prepare ("SELECT Pres FROM ObjEventChanges WHERE ItId=:id AND Ft=:ft");
    q.bindValue (":id", fcev->itid);
    q.bindValue (":ft", fcev->ft);
    if (!q.exec ())
      throw 1;
    if (q.next ()){
      fpres = q.value (0).toBool ();
      if (fcev->pres || fcev->pres != fpres){
        q.clear ();
        q.prepare ("UPDATE ObjEventChanges SET Dtm=:dtm, Pres=:pr WHERE "
            "ItId=:id AND Ft=:ft");
        q.bindValue (":dtm", fcev->dtm);
        q.bindValue (":pr", fcev->pres);
        q.bindValue (":id", fcev->itid);
        q.bindValue (":ft", fcev->ft);
        if (!q.exec ())
          throw 1;
        res = true;
      }
    }
    else{
      q.clear ();
      q.prepare ("INSERT INTO ObjEventChanges (Dtm, ObjIt, ItId, Ft, Pres) "
          "VALUES (?, ?, ?, ?, ?)");
      q.bindValue (0, fcev->dtm);
      q.bindValue (1, fcev->objid);
      q.bindValue (2, fcev->itid);
      q.bindValue (3, fcev->ft);
      q.bindValue (4, fcev->pres);
      if (!q.exec ())
        throw 1;
      res = true;
    }
    if (res && getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveObjFtChange | Событие изменения "
          "признака сохранено.", 0x15);
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveObjFtChange | Ошибка при сохранении"
          " события изменения признака!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
ContConnEvent* TechPgDataKeeper::abnConnectEv (quint32 cid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ContConnEvent *cev = nullptr;
  bool pres;

  q.prepare ("SELECT Dtm, Pres FROM AbnConnChanges WHERE Cont=:cid");
  q.bindValue (":cid", cid);
  if (q.exec () && q.next ()){
    pres = q.value (1).toBool ();
    if (pres){
      cev = new ContConnEvent;
      cev->id = cid;
      cev->dtm = q.value (0).toDateTime ();
      cev->con = false;
    }
  }
  return cev;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::saveAbnConnEv (ContConnEvent *cce)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  bool pres, res = false;

  try{
    if (!cce)
      throw 1;
    q.prepare ("SELECT Pres FROM AbnConnChanges WHERE Cont=:cid");
    q.bindValue (":cid", cce->id);
    if (!q.exec ())
      throw 1;
    if (q.next ()){
      pres = q.value (0).toBool ();
      if (cce->con == pres){
        q.clear ();
        q.prepare ("UPDATE AbnConnChanges SET Dtm=:dtm, Pres=:pr WHERE Cont=:cid");
        q.bindValue (":dtm", cce->dtm);
        q.bindValue (":pr", !cce->con);
        q.bindValue (":cid", cce->id);
        if (!q.exec ())
          throw 1;
        res = true;
      }
    }
    else if (cce->con){
      q.clear ();
      q.prepare ("INSERT INTO AbnConnChanges (Cont, Dtm, Pres) VALUES (?, ?, ?)");
      q.bindValue (0, cce->id);
      q.bindValue (1, cce->dtm);
      q.bindValue (2, !cce->con);
      if (!q.exec ())
        throw 1;
      res = true;
    }
    if (res && getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveAbnConnEv | Событие изменения "
          "связи сохранено.", 0x15);
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveAbnConnEv | Ошибка при сохранении"
          " события изменения связи!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readEvents (ObjectEvents &events, EvFilter *flt)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  QString str;
  QByteArray ba;
  ItkSrvEvent *ev;
  bool fl = false;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (events);
    events.clear ();
    str = "SELECT e.Dtm, e.ObjId, e.ItId, e.ItType, e.EvType, e.Val, t.Name FROM "
        "ItkObjEvents e, EventTypes t WHERE t.Id=e.EvType";
    if (flt){
      if (flt->objid)
        str += QString (" AND e.ObjId=%1").arg (flt->objid);
      if (!flt->evs.empty ()){
        str += " AND e.EvType IN (";
        for (quint8 ev : flt->evs){
          if (fl)
            str += ", ";
          else
            fl = true;
          str += QString::number (ev);
        }
        str += ")";
      }
      if ((flt->fltm & 1) != 0)
        str += " AND e.Dtm >= :dtm1";
      if ((flt->fltm & 2) != 0)
        str += " AND e.Dtm <= :dtm2";
    }
    str += " ORDER BY e.Dtm, e.ObjId";
    q.prepare (str);
    if ((flt->fltm & 1) != 0)
      q.bindValue (":dtm1", flt->dtm1);
    if ((flt->fltm & 2) != 0)
      q.bindValue (":dtm2", flt->dtm2);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ev = new ItkSrvEvent;
      ev->dtm = q.value (0).toDateTime ();
      ev->objid = q.value (1).toUInt ();
      ev->itid = q.value (2).toUInt ();
      ev->ittp = quint8(q.value (3).toUInt ());
      ev->evtp = quint16(q.value (4).toUInt ());
      ba = q.value (5).toByteArray ();
      ev->name = q.value (6).toString ();
      Value::fromMessagePackBinary (ba.data (), quint32(ba.size ()), ev->cont);
      if (ev->cont.isValid ())
        events.push_back (ev);
      else
        delete ev;
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readEvents | Ошибка при считывании "
          "списка событий!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveEvent (ItkSrvEvent *ev)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    if (!ev)
      throw 1;
    if ((ev->st & 4) != 0 || !ev->cont.isValid ())
      throw 1;
    if ((ev->st & 2) != 0){
      q.prepare ("INSERT INTO ItkObjEvents (Dtm, ObjId, ItId, ItType, EvType, Val) "
          "VALUES (?, ?, ?, ?, ?, ?)");
      q.bindValue (0, ev->dtm);
      q.bindValue (1, ev->objid);
      q.bindValue (2, ev->itid);
      q.bindValue (3, ev->ittp);
      q.bindValue (4, ev->evtp);
      q.bindValue (5, ev->cont.toMessagePack (true));
      if (!q.exec ())
        throw 1;
      ev->st = 0;
    }
    else if ((ev->st & 3) == 1){
      q.prepare ("UPDATE ItkObjEvents SET Val=:val WHERE Dtm=:dtm AND ObjId=:oid "
          "AND ItId=:iid AND ItType=:itp");
      q.bindValue (":val", ev->cont.toMessagePack (true));
      q.bindValue (":dtm", ev->dtm);
      q.bindValue (":oid", ev->objid);
      q.bindValue (":iid", ev->itid);
      q.bindValue (":itp", ev->ittp);
      if (!q.exec ())
        throw 1;
      ev->st = 0;
    }
    res = true;
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveEvent | Ошибка при сохранении"
          " события!", 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveEvents (ObjectEvents &events)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  ObjectEvents::iterator it;
  ItkSrvEvent *ev;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ItkObjEvents WHERE Dtm=:dtm AND ObjId=:oid AND "
        "ItId=:iid AND ItType=:itp;");
    it = events.begin ();
    while (it != events.end ()){
      ev = *it;
      if ((ev->st & 4) != 0){
        q.bindValue (":dtm", ev->dtm);
        q.bindValue (":oid", ev->objid);
        q.bindValue (":iid", ev->itid);
        q.bindValue (":itp", ev->ittp);
        if (!q.exec ())
          throw 1;
        it = events.erase (it);
        delete ev;
      }
      else
        it ++;
    }
    for (ItkSrvEvent *ev : events){
      if ((ev->st & 3) == 1)
        ev->st = 0;
    }
    q.clear ();
    q.prepare ("INSERT INTO ItkObjEvents (Dtm, ObjId, ItId, ItType, EvType, Val) "
        "VALUES (?, ?, ?, ?, ?, ?);");
    for (ItkSrvEvent *ev : events){
      if ((ev->st & 2) != 0){
        q.bindValue (0, ev->dtm);
        q.bindValue (1, ev->objid);
        q.bindValue (2, ev->itid);
        q.bindValue (3, ev->ittp);
        q.bindValue (4, ev->evtp);
        q.bindValue (5, ev->cont.toMessagePack (true));
        if (!q.exec ())
          throw 1;
        ev->st = 0;
      }
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveEvents | Ошибка при сохранении"
          " списка событий!", 0x15, 6);
    q.exec ("ROLLBACK");
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::dbClear (QDateTime &dtm)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  bool fl;

  if (db.isValid () && db.isOpen ()){
    q.exec ("BEGIN");
    q.prepare ("DELETE FROM ItkObjEvents WHERE Dtm<:dtm;");
    q.bindValue (":dtm", dtm);
    fl = q.exec ();
    q.clear ();
    q.prepare ("DELETE FROM AlertSeance WHERE Dtm<:dtm;");
    q.bindValue (":dtm", dtm);
    if (q.exec ())
      fl = true;
    if (fl){
      q.exec ("COMMIT");
      if (getLogger ())
        m_Log->writeLog ("TechPgDataKeeper | dbClear | Журнал событий очищен.", 0x15);
    }
    else{
      q.exec ("ROLLBACK");
      if (getLogger ())
        m_Log->writeLog ("TechPgDataKeeper | dbClear | Журнал событий "
            "не очищен!", 0x15);
    }
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::dbVacuum ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);

  if (getLogger ())
    m_Log->writeLog ("TechPgDataKeeper | dbVacuum | Начало процедуры.", 0x15);
  if (q.exec ("VACUUM (FULL, ANALYZE);")){
    if (m_Log)
      m_Log->writeLog ("TechPgDataKeeper | dbVacuum | Проведена сборка мусора.", 0x15);
  }
  if (m_Log)
    m_Log->writeLog ("TechPgDataKeeper | dbVacuum | Завершение процедуры.", 0x15);
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveAlertSeance (PgAlertSeance *als, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  bool res = false;

  if (!db.isValid () || !db.isOpen () || !als || (als->st & 4) != 0)
    return false;
  if (!als->st)
    return true;
  try{
    q.exec ("BEGIN");
    if ((als->st & 3) == 1){
      q.prepare ("UPDATE AlertSeance SET Dtm=:dtm, Code=:code, Src=:src, Type="
          ":tp, Num=:num, Name=:name WHERE Id=:id;");
      q.bindValue (":dtm", als->tmstart);
      q.bindValue (":code", als->code);
      q.bindValue (":src", als->src);
      q.bindValue (":tp", als->type);
      q.bindValue (":num", als->num);
      q.bindValue (":name", als->name);
      q.bindValue (":id", als->id);
      if (!q.exec ())
        throw 1;
    }
    else{
      q.prepare ("INSERT INTO AlertSeance (Id, Dtm, Code, Src, Syst, Type, Num, Name)"
          " VALUES (DEFAULT, ?, ?, ?, ?, ?, ?, ?) RETURNING Id;");
      q.bindValue (0, als->tmstart);
      q.bindValue (1, als->code);
      q.bindValue (2, als->src);
      q.bindValue (3, sys);
      q.bindValue (4, als->type);
      q.bindValue (5, als->num);
      q.bindValue (6, als->name);
      if (!q.exec () || !q.next ())
        throw 1;
      als->id = q.value (0).toUInt ();
    }
    als->st = 0;
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveAlertSeance | Сеанс оповещения не"
          " сохранен!", 0x15);
  }
  return res;
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveAlertObjects (Ui32List &objlst, quint32 sid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  Ui32Set ids;
  quint32 id;
  bool res = false;

  if (!db.isValid () || !db.isOpen ())
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("SELECT Abn FROM AlertObjects WHERE Seance=:sid;");
    q.bindValue (":sid", sid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      id = q.value (0).toUInt ();
      if (!ids.contains (id))
        ids.insert (id);
    }
    q.clear ();
    q.prepare ("INSERT INTO AlertObjects (Seance, Abn) VALUES (?, ?);");
    q.bindValue (0, sid);
    for (quint32 n : objlst){
      q.bindValue (1, n);
      if (!q.exec ())
        throw 1;
    }
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveAlertObjects | Объекты сеанса "
          "оповещения не сохранены!", 0x15);
  }
  return res;
}

// -----------------------------------------------------------------------------
PgAlertSeance* TechPgDataKeeper::readAlertSeance (quint64 &acode, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  PgAlertSeance *als = nullptr;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.prepare ("SELECT Id FROM AlertSeance WHERE Code=:code AND Syst=:sys "
        "ORDER BY Dtm DESC");
    q.bindValue (":code", acode);
    q.bindValue (":sys", sys);
    if (!q.exec () || !q.next ()){
      if (getLogger ())
        m_Log->writeLog ("TechPgDataKeeper | readAlertSeance | Сеанс не "
            "найден.", 0x15);
      throw 1;
    }
    als = new PgAlertSeance;
    als->id = q.value (0).toUInt ();
    als->code = acode;
    q.clear ();
    q.prepare ("SELECT Dtm, Src, Type, Num, Name FROM AlertSeance WHERE Id=:id");
    q.bindValue (":id", als->id);
    if (!q.exec () || !q.next ())
      throw 1;
    als->tmstart = q.value (0).toDateTime ();
    als->src = q.value (1).toUInt ();
    als->type = quint8(q.value (2).toUInt ());
    als->num = quint16(q.value (3).toUInt ());
    als->name = q.value (4).toString ();
  }
  catch (...){
    if (als){
      delete als;
      als = nullptr;
    }
  }
  return als;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readAlertObjects (Ui32List &objlst, quint32 sid)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  quint32 id;

  objlst.clear ();
  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.prepare ("SELECT Abn FROM AlertObjects WHERE Seance=:sid");
    q.bindValue (":sid", sid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      id = q.value (0).toUInt ();
      objlst.push_back (id);
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readAlertObjects | Ошибка при "
          "считывании объектов сеанса оповещения!", 0x15);
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::saveAlertEvent (PgAlertEvent *ae)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  bool res = false;

  if (!db.isValid () || !db.isOpen () || !ae)
    return false;
  try{
    q.exec ("BEGIN");
    q.prepare ("INSERT INTO AlertEvents (Sid, Src, Oid, CNum, Cid, Dst, Dtm, "
        "Val) VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    q.bindValue (0, ae->sid);
    q.bindValue (1, ae->src);
    q.bindValue (2, ae->oid);
    q.bindValue (3, ae->num);
    q.bindValue (4, ae->ctp);
    q.bindValue (5, ae->dst);
    q.bindValue (6, ae->dtm);
    q.bindValue (7, ae->cont);
    if (!q.exec ())
      throw 1;
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | saveAlertEvent | Событие сеанса "
          "оповещения не сохранено!", 0x15);
  }
  return res;
}

using ObjNumMap = QMap<quint32, quint32>;
// -----------------------------------------------------------------------------
void TechPgDataKeeper::readSeances (TsAlertSeances &alerts, EvFilter *flt, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db), q1 (db);
  ObjNumMap objnums;
  ObjNumMap::iterator it;
  QString str;
  QByteArray ba;
  QDateTime dtm;
  TsAlertSeance *al;
  AlObject *obj;
  AlObjEvent *ev;
  quint32 id, num;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (alerts);
    alerts.clear ();
    q.prepare ("SELECT i.Id, i.Num FROM Objects o, Items i WHERE o.Syst=:sys AND"
        " o.Act=TRUE AND i.Id=o.Id");
    q.bindValue (":sys", sys);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      id = q.value (0).toUInt ();
      num = q.value (1).toUInt ();
      objnums.insert (id, num);
    }
    str = "SELECT Id, Dtm, Code, Src, Type, Num, Name FROM AlertSeance WHERE "
        "Syst=:sys";
    if (flt){
      if ((flt->fltm & 1) != 0)
        str += " AND Dtm >= :dtm1";
      if ((flt->fltm & 2) != 0)
        str += " AND Dtm <= :dtm2";
    }
    str += " ORDER BY Dtm";
    q.clear ();
    q.prepare (str);
    q.bindValue (":sys", sys);
    if ((flt->fltm & 1) != 0)
      q.bindValue (":dtm1", flt->dtm1);
    if ((flt->fltm & 2) != 0){
      dtm = flt->dtm2.addDays (1);
      q.bindValue (":dtm2", dtm);
    }
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      al = new TsAlertSeance;
      al->m_Id = q.value (0).toUInt ();
      al->m_TmStart = q.value (1).toDateTime ();
      al->m_Code = q.value (2).toULongLong ();
      al->m_Src = q.value (3).toUInt ();
      al->m_Type = quint8(q.value (4).toUInt ());
      al->m_Num = quint16(q.value (5).toUInt ());
      al->m_Name = q.value (6).toString ();
      alerts.push_back (al);
    }
    q.clear ();
    q.prepare ("SELECT o.Abn, i.Name, i.Num, i.Tp FROM AlertObjects o, Items i "
        "WHERE o.Seance=:sid AND i.Id=o.Abn ORDER BY i.Num");
    for (TsAlertSeance *al : alerts){
      q.bindValue (":sid", al->m_Id);
      if (!q.exec ())
        throw 1;
      while (q.next ()){
        obj = new AlObject;
        obj->m_Id = q.value (0).toUInt ();
        obj->m_Name = q.value (1).toString ();
        obj->m_Num = q.value (2).toUInt ();
        obj->m_Type = quint16(q.value (3).toUInt ());
        al->m_Objects.insert (obj->m_Num, obj);
      }
    }
    q.clear ();
    q.prepare ("SELECT Src, Dst, Dtm, Cid, CNum, Val FROM AlertEvents WHERE "
        "Sid=:sid AND Oid=:oid ORDER BY Dtm, Cid, CNum");
    q1.prepare ("SELECT Dtm, CNum, Val FROM AlertEvents WHERE Sid=:sid AND "
        "Src=:src AND Oid=:oid AND Cid=7 ORDER BY Dtm, CNum");
    for (TsAlertSeance *al : alerts){
      q.bindValue (":sid", al->m_Id);
      for (AlObject *ao : al->m_Objects){
        q.bindValue (":oid", ao->m_Id);
        if (!q.exec ())
          throw 1;
        while (q.next ()){
          ev = new AlObjEvent;
          ev->m_SrcId = q.value (0).toUInt ();
          if ((it = objnums.find (ev->m_SrcId)) != objnums.end ())
            ev->m_SrcNum = it.value ();
          ev->m_DstId = q.value (1).toUInt ();
          if ((it = objnums.find (ev->m_DstId)) != objnums.end ())
            ev->m_DstNum = it.value ();
          ev->m_Dtm = q.value (2).toDateTime ();
          ev->m_Ctp = quint8(q.value (3).toUInt ());
          ev->m_Num = quint8(q.value (4).toUInt ());
          ba = q.value (5).toByteArray ();
          Value::fromMessagePackBinary (ba.data (), quint32(ba.size ()), ev->m_Cont);
          ao->m_Events.push_back (ev);
        }
      }
      q1.bindValue (":sid", al->m_Id);
      q1.bindValue (":src", al->m_Src);
      q1.bindValue (":oid", al->m_Src);
      if (!q1.exec ())
        throw 1;
      while (q1.next ()){
        ev = new AlObjEvent;
        ev->m_SrcId = al->m_Src;
        if ((it = objnums.find (ev->m_SrcId)) != objnums.end ())
          ev->m_SrcNum = it.value ();
        ev->m_Ctp = 7;
        ev->m_Dtm = q1.value (0).toDateTime ();
        ev->m_Num = quint8(q1.value (1).toUInt ());
        ba = q1.value (2).toByteArray ();
        Value::fromMessagePackBinary (ba.data (), quint32(ba.size ()), ev->m_Cont);
        al->m_Events.push_back (ev);
      }
    }
    objnums.clear ();
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readSeances | Ошибка при считывании "
          "списка сеансов!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::readAlObjSounds (AlObjEvents &evts, quint32 sid, quint32 anum,
    quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db);
  QByteArray ba;
  AlObjEvent *ev;
  quint32 id = 0;

  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    qDeleteAll (evts);
    evts.clear ();
    q.prepare ("SELECT i.Id FROM Objects o, Items i WHERE o.Syst=:sys AND"
        " i.Num=:num AND o.Act=TRUE AND i.Id=o.Id");
    q.bindValue (":sys", sys);
    q.bindValue (":num", anum);
    if (!q.exec () || !q.next ())
      throw 1;
    if ((id = q.value (0).toUInt ()) == 0)
      throw 1;
    q.clear ();
    q.prepare ("SELECT e.Src, i.Num, e.Dtm, e.CNum, e.Val FROM AlertEvents e, "
        "Items i WHERE e.Sid=:sid AND e.Oid=:oid AND e.Cid=7 AND i.Id=e.Src "
        "ORDER BY e.Dtm, e.CNum");
    q.bindValue (":sid", sid);
    q.bindValue (":oid", id);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      ev = new AlObjEvent;
      ev->m_ObjId = id;
      ev->m_ObjNum = anum;
      ev->m_Ctp = 7;
      ev->m_SrcId = q.value (0).toUInt ();
      ev->m_SrcNum = q.value (1).toUInt ();
      ev->m_Dtm = q.value (2).toDateTime ();
      ev->m_Num = quint8(q.value (3).toUInt ());
      ba = q.value (4).toByteArray ();
      Value::fromMessagePackBinary (ba.data (), quint32(ba.size ()), ev->m_Cont);
      evts.push_back (ev);
    }
  }
  catch (...){
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | readAlObjSounds | Ошибка при считывании"
          " списка событий!", 0x15, 6);
  }
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::joinAlertSeances (quint64 &acode, quint64 &oldcode, quint8 sys)
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  QSqlQuery q (db), q1 (db);
  quint32 id, oldid, aid;
  bool res = false;

  getLogger ();
  try{
    if (!db.isValid () || !db.isOpen ())
      throw 1;
    q.exec ("BEGIN");
    q.prepare ("SELECT Id FROM AlertSeance WHERE Code=:code AND Syst=:sys "
        "ORDER BY Dtm DESC;");
    q.bindValue (":code", acode);
    q.bindValue (":sys", sys);
    if (!q.exec () || !q.next ()){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | joinAlertSeances | Сеанс не "
            "найден!", 0x15);
      throw 1;
    }
    id = q.value (0).toUInt ();
    q.bindValue (":code", oldcode);
    if (!q.exec () || !q.next ()){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | joinAlertSeances | Сеанс не "
            "найден!", 0x15);
      throw 1;
    }
    oldid = q.value (0).toUInt ();
    q.clear ();
    q.prepare ("SELECT Abn FROM AlertObjects WHERE Seance=:sid;");
    q.bindValue (":sid", oldid);
    q1.prepare ("UPDATE AlertObjects SET Seance=:sid, Abn=:aid WHERE Seance=:sid0"
        " AND Abn=:aid0;");
    q1.bindValue (":sid", id);
    q1.bindValue (":sid0", oldid);
    if (!q.exec ())
      throw 1;
    while (q.next ()){
      aid = q.value (0).toUInt ();
      q1.bindValue (":aid", aid);
      q1.bindValue (":aid0", aid);
      if (!q1.exec ()){
        if (m_Log)
          m_Log->writeLog (QString ("TechPgDataKeeper | joinAlertSeances | Абонент"
              " №%1 не перенесен в другой сеанс!"), 0x15);
      }
    }
    q.clear ();
    q.prepare ("UPDATE AlertEvents SET Sid=:sid WHERE Sid=:sid0;");
    q.bindValue (":sid", id);
    q.bindValue (":sid0", oldid);
    if (!q.exec ()){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | joinAlertSeances | События не "
            "перенесены в другой сеанс!", 0x15);
    }
    q.prepare ("DELETE FROM AlertSeance WHERE Id=:sid;");
    q.bindValue (":sid", oldid);
    q.exec ();
    q.exec ("COMMIT");
    res = true;
  }
  catch (...){
    q.exec ("ROLLBACK");
    if (getLogger ())
      m_Log->writeLog ("TechPgDataKeeper | joinAlertSeances | Сеансы не "
          "объединены!", 0x15, 6);
  }
  return res;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::addContObject (QSqlDatabase &db, quint32 obj, quint32 cont, quint8 lev)
{
  QSqlQuery q (db);

  try{
    switch (lev){
    case 0:
      lev = 2;
      break;
    case 2:
      lev = 0;
      break;
    }
    q.clear ();
    q.prepare ("INSERT INTO ContObjects (Id, Obj, Cont, Lev) VALUES (DEFAULT, ?, ?, ?);");
    q.bindValue (0, cont);
    q.bindValue (1, obj);
    q.bindValue (2, lev);
    q.exec ();
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::updateContObject (QSqlDatabase &db, quint32 obj, quint32 cont, quint8 lev)
{
  QSqlQuery q (db);
  quint32 id;

  try{
    q.prepare ("SELECT Id FROM ContObjects WHERE Obj=:obj AND Cont=:cont;");
    q.bindValue (":obj", cont);
    q.bindValue (":cont", obj);
    if (q.exec ()){
      switch (lev){
      case 0:
        lev = 2;
        break;
      case 2:
        lev = 0;
        break;
      }
      if (q.next ()){
        id = q.value (0).toUInt ();
        q.clear ();
        q.prepare ("UPDATE ContObjects SET Lev=:lev WHERE Id=:id;");
        q.bindValue (":lev", lev);
        q.bindValue (":id", id);
        q.exec ();
      }
      else{
        q.clear ();
        q.prepare ("INSERT INTO ContObjects (Id, Obj, Cont, Lev) VALUES (DEFAULT, ?, ?, ?);");
        q.bindValue (0, cont);
        q.bindValue (1, obj);
        q.bindValue (2, lev);
        q.exec ();
      }
    }
  }
  catch (...){
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::deleteContObject (QSqlDatabase &db, quint32 obj, quint32 cont)
{
  QSqlQuery q (db);

  q.prepare ("DELETE FROM ContObjects WHERE Obj=:obj AND Cont=:cont;");
  q.bindValue (":obj", cont);
  q.bindValue (":cont", obj);
  q.exec ();
}

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::getLogger ()
{
  return (m_LHost != nullptr && (m_Log = m_LHost->GetLogger (&m_LogFlags)) != nullptr);
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::slotTimer ()
{
  QSqlDatabase db = QSqlDatabase::database ("itk");
  int res = 0;
  bool fl = false;

  getLogger ();
  try{
    while (!fl){
      if (db.databaseName () != m_BsName){
        if (db.isOpen ())
          db.close ();
        db.setDatabaseName (m_BsName);
      }
      fl = db.open ();
      if (!fl){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | slotTimer | Неудачная попытка "
              "подсоединения к БД '" + m_BsName + "'.", 0x15, 6);
        {
          QSqlError er = db.lastError ();
          if (m_Log)
            m_Log->writeLog ("TechPgDataKeeper | slotTimer | Ошибка " + er.text () +
                ".", 0x15, 6);
          m_TryOpenCounter ++;
          if (m_TryOpenCounter > 5){
            res = -1;
            throw 1;
          }
          if (er.type () != 1)
            throw 1;
        }
        db.setDatabaseName ("postgres");
        if (!db.open ()){
          if (m_Log)
            m_Log->writeLog ("TechPgDataKeeper | slotTimer | Не могу открыть БД "
                "'postgres'!", 0x15, 6);
          throw 1;
        }
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | slotTimer | База данных 'postgres'"
              " открыта.", 0x15);
        QSqlQuery q (db);
        //if (!q.exec ("CREATE DATABASE \"" + m_BsName + "\" OWNER postgres TEMPLATE"
        //    " postgres;")){
        if (!q.exec ("CREATE DATABASE \"" + m_BsName + "\" OWNER postgres "
            "ENCODING = 'UTF8';")){
          if (m_Log)
            m_Log->writeLog ("TechPgDataKeeper | slotTimer | Не могу создать БД '" +
                m_BsName + "'!", 0x15, 6);
          throw 1;
        }
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | slotTimer | База данных '" +
              m_BsName + "' создана.", 0x15);
        db.close ();
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | slotTimer | База данных 'postgres'"
              " закрыта.", 0x15);
        db.setDatabaseName (m_BsName);
      }
    }
  }
  catch (...){
  }
  if (fl){
    m_TryOpenCounter = 0;
    if (m_Log)
      m_Log->writeLog ("TechPgDataKeeper | slotTimer | База данных '" + m_BsName +
          "' открыта.", 0x15);
    if (checkData (db)){
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | slotTimer | База данных '" + m_BsName +
            "' готова к работе.", 0x15);
    }
    else if (m_Log)
      m_Log->writeLog ("TechPgDataKeeper | slotTimer | Необходима настройка БД '" +
          m_BsName + "'.", 0x15);
    emit onInitFinished (0);
  }
  else{
    if (db.isOpen ()){
      db.close ();
      if (m_Log)
        m_Log->writeLog ("TechPgDataKeeper | slotTimer | База данных 'postgres'"
            " закрыта.", 0x15);
    }
    if (res < 0)
      emit onInitFinished (res);
    else
      m_InitTimer->start ();
  }

  /*QSqlDatabase db = QSqlDatabase::database ("alert");
  if (db.isValid () && db.isOpen ()){
    QSqlQuery q (db);
    QString qw = QString ().sprintf ("SELECT COUNT(*) FROM AlertCmd_%02u", jrn->m_tm.Year%100);
    if (!q.exec (qw)){
      if (m_Log){
        xPrint (str, 256, "P166_Tv | slotAddCommand | Не найдена таблица AlertCmd_%02u.Создаем расширение.", jrn->m_tm.Year%100);
        m_Log->WriteLog (str , 0x11, 0);
      }
      try{
        dirname += QString ().sprintf ("/%04u", jrn->m_tm.Year);
        QDir dr (dirname);
        if (!dr.exists ()){
          if (!dr.mkdir (dirname)){
            if (m_Log)
              m_Log->WriteLog (("P166_Tv | slotWriteJournal | Не создан каталог " + dirname).toAscii ().data (), 0x11, 6);
            throw 1;
          }
        }
        qw = QString ().sprintf ("CREATE TABLESPACE AlertHist%02u LOCATION \'%s\';",
          jrn->m_tm.Year%100, dirname.toAscii ().data ());
        if (m_Log)
          m_Log->WriteLog (("P166_Tv | slotAddCommand | " + qw).toAscii ().data () , 0x11, 7);
        if (!q.exec (qw)){
          if (m_Log)
            m_Log->WriteLog ("P166_Tv | slotAddCommand | Табличное пространство не создано!" , 0x11, 7);
        }
        qw = QString ().sprintf ("CREATE TABLE AlertCmd_%02u ("
          " CHECK (Dt>='%04u-01-01' AND Dt<'%04u-01-01')"
          ") INHERITS (AlertCmdJournal) TABLESPACE AlertHist%02u;", jrn->m_tm.Year%100, jrn->m_tm.Year,
          jrn->m_tm.Year + 1, jrn->m_tm.Year%100);
        if (!q.exec (qw))
          throw 1;
        if (m_Log)
          m_Log->WriteLog ("P166_Tv | slotAddCommand | ***** Таблица AlertCmd_XX создана." , 0x11, 7);
        qw = QString ().sprintf (
          "CREATE OR REPLACE FUNCTION CmdJournal_trigger () "
          "RETURNS TRIGGER AS $$ "
          "BEGIN "
          "  IF (NEW.Dt>='%04u-01-01' AND NEW.Dt<'%04u-01-01') THEN "
          "    INSERT INTO AlertCmd_%02u VALUES (NEW.*); "
          "  ELSE "
          "    RAISE EXCEPTION 'Data out of range!'; "
          "  END IF; "
          "  RETURN NULL; "
          "END; "
          "$$ "
          "LANGUAGE plpgsql;", jrn->m_tm.Year, jrn->m_tm.Year + 1, jrn->m_tm.Year%100);
        if (!q.exec (qw))
          throw 1;
        if (m_Log)
          m_Log->WriteLog ("P166_Tv | slotAddCommand | ***** Функция создана." , 0x11, 7);
        q.exec ("DROP TRIGGER IF EXISTS InsAlertCmdTrigger ON AlertCmdJournal;");
        qw =
          "CREATE TRIGGER InsAlertCmdTrigger BEFORE INSERT ON AlertCmdJournal "
          "FOR EACH ROW EXECUTE PROCEDURE CmdJournal_trigger ();";
        if (!q.exec (qw))
          throw 1;
        if (m_Log)
          m_Log->WriteLog ("P166_Tv | slotAddCommand | ***** Триггер создан." , 0x11, 7);
        qw = QString ().sprintf ("CREATE INDEX AlertCmdJourn%02uInd ON AlertCmd_%02u (Dt, Num, Cod) TABLESPACE AlertHist%02u;",
          jrn->m_tm.Year%100, jrn->m_tm.Year%100, jrn->m_tm.Year%100);
        if (!q.exec (qw))
          throw 1;
        if (m_Log)
          m_Log->WriteLog ("P166_Tv | slotAddCommand | ***** Индекс создан." , 0x11, 7);
      }
      catch (...){
        if (m_Log)
          m_Log->WriteLog ("P166_Tv | slotAddCommand | Произошла ошибка при расширении пространства для журнала!" , 0x11, 6);
        res = false;
      }
    }
    if (res){
      xPrint (str, 256, "%04u-%02u-%02uT%02u:%02u:%02u", jrn->m_tm.Year, jrn->m_tm.Month, jrn->m_tm.Day,
        jrn->m_tm.Hour, jrn->m_tm.Minute, jrn->m_tm.Second);
      if (jrn->m_cmd == 0x31){
        q.prepare ("INSERT INTO AlertCmdJournal (Dt, Num, Cod, Cmd, Cont, Src, Usr, Lock, Tst, P166, Vid)"
          " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        q.bindValue (0, str);
        q.bindValue (1, jrn->m_num);
        q.bindValue (2, jrn->m_cod);
        q.bindValue (3, jrn->m_cmd);
        q.bindValue (4, jrn->m_ba);
        q.bindValue (5, jrn->m_src);
        q.bindValue (6, m_Alias);
        q.bindValue (7, jrn->m_lock);
        q.bindValue (8, jrn->m_test);
        q.bindValue (9, jrn->m_p166);
        q.bindValue (10, jrn->m_vid);
      }
      else{
        q.prepare ("INSERT INTO AlertCmdJournal (Dt, Num, Cod, Cmd, Cont) VALUES (?, ?, ?, ?, ?)");
        q.bindValue (0, str);
        q.bindValue (1, jrn->m_num);
        q.bindValue (2, jrn->m_cod);
        q.bindValue (3, jrn->m_cmd);
        q.bindValue (4, jrn->m_ba);
      }
      if (!q.exec ())
        res = false;
    }
    if (!res && m_Log)
      m_Log->WriteLog ("P166_Tv | slotAddCommand | Запись в журнал не добавлена!" , 0x11, 6);
  }
  else if (m_Log)
    m_Log->WriteLog ("P166_Tv | slotAddCommand | Журнальная запись не создана! База данных недоступна!" , 0x11, 6);*/
}

// -----------------------------------------------------------------------------
extern "C" Q_DECL_EXPORT ThSrvDataKeeper* CreateObjInstanse (const KeeperDataInit &di)
{
  return new TechPgDataKeeper (di);
}

// -----------------------------------------------------------------------------
} // namespace ItkSrv
