#include "TechPgDataKeeper.h"
#include "Value.h"

using namespace UnVal;

namespace ItkSrv {

const char *seq01 =
  "CREATE SEQUENCE PropValKeyGen START WITH 1 INCREMENT BY 1 CACHE 1 NO CYCLE;",
*tbl01 =
  "CREATE TABLE FtValue4 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   INTEGER NOT NULL"
  ") WITH (fillfactor=90);",
*tbl02 =
  "CREATE TABLE FtValue5 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   BIGINT NOT NULL"
  ") WITH (fillfactor=90);",
*tbl03 =
  "CREATE TABLE FtValue10 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   BOOLEAN NOT NULL"
  ") WITH (fillfactor=90);",
*tbl04 =
  "CREATE TABLE FtValue11 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   FLOAT NOT NULL"
  ") WITH (fillfactor=90);",
*tbl05 =
  "CREATE TABLE FtValue13 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   VARCHAR(256)"
  ") WITH (fillfactor=90);",
*tbl06 =
  "CREATE TABLE FtValue14 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   TIMESTAMP NOT NULL"
  ") WITH (fillfactor=90);",
*tbl07 =
  "CREATE TABLE FtValue15 ("
  " Id    INTEGER PRIMARY KEY,"
  " Val   BYTEA"
  ") WITH (fillfactor=90);",
*tbl10 =
  "CREATE TABLE Properties ("
  " Id    SMALLINT PRIMARY KEY,"
  " Name  VARCHAR(64),"
  " VTp   SMALLINT DEFAULT 8,"
  " Vs    BOOLEAN"
  ") WITH (fillfactor=90);",
*cnt10[] = {
  "INSERT INTO Properties (Id, Name, VTp, Vs) VALUES (1, \'Адрес\', 11, FALSE);",
  "INSERT INTO Properties (Id, Name, VTp, Vs) VALUES (3, \'Версия прошивки\', 11, FALSE);"
},

*tbl12 =
  "CREATE TABLE Types ("
  " Id      SMALLINT PRIMARY KEY,"
  " Name    VARCHAR(48)"
  ") WITH (fillfactor=90);",
*cnt12[] = {
  "INSERT INTO Types (Id, Name) VALUES (1, \'ИТК ОС Компьютер\');",
  "INSERT INTO Types (Id, Name) VALUES (2, \'Квадрант\');",
  "INSERT INTO Types (Id, Name) VALUES (3, \'УЭС\');",
  "INSERT INTO Types (Id, Name) VALUES (4, \'КГО\');",
  "INSERT INTO Types (Id, Name) VALUES (5, \'УЗСР\');",
  "INSERT INTO Types (Id, Name) VALUES (6, \'ВАУ\');",
  "INSERT INTO Types (Id, Name) VALUES (7, \'Видеоперехват\');",
  "INSERT INTO Types (Id, Name) VALUES (8, \'Внешняя система\');",
  "INSERT INTO Types (Id, Name) VALUES (9, \'Старый парк\');",
  "INSERT INTO Types (Id, Name) VALUES (10, \'ЦКС\');",
  "INSERT INTO Types (Id, Name) VALUES (11, \'БСИ\');",
  "INSERT INTO Types (Id, Name) VALUES (12, \'Варяг\');",
  "INSERT INTO Types (Id, Name) VALUES (40, \'Скиф\');",
  "INSERT INTO Types (Id, Name) VALUES (41, \'Датчик ВАУ\');",
  "INSERT INTO Types (Id, Name) VALUES (42, \'Линия аудиовыхода\');",
  "INSERT INTO Types (Id, Name) VALUES (43, \'Модуль усилителя звука\');",
  "INSERT INTO Types (Id, Name) VALUES (44, \'Модуль электропитания\');",
  "INSERT INTO Types (Id, Name) VALUES (45, \'Модуль датчиков\');",
  "INSERT INTO Types (Id, Name) VALUES (46, \'Пара контактов\');",
  "INSERT INTO Types (Id, Name) VALUES (47, \'Реле\');",
  "INSERT INTO Types (Id, Name) VALUES (48, \'Микрофон\');",
  "INSERT INTO Types (Id, Name) VALUES (50, \'-- неизвестный --\');"
},
*tbl14 =
  "CREATE TABLE Features ("
  " Id      SMALLINT PRIMARY KEY,"
  " Name    VARCHAR(48)"
  ") WITH (fillfactor=90);",
*cnt14[] = {
  "INSERT INTO Features (Id, Name) VALUES (1, \'Отсутствует связь\');",
  "INSERT INTO Features (Id, Name) VALUES (2, \'Вскрытие оборудования\');",
  "INSERT INTO Features (Id, Name) VALUES (3, \'Отсутствует питание 220 В\');",
  "INSERT INTO Features (Id, Name) VALUES (4, \'Аккумулятор разряжен\');",
  "INSERT INTO Features (Id, Name) VALUES (5, \'Нет звукового давления\');",
  "INSERT INTO Features (Id, Name) VALUES (6, \'Асимметрия токов нагрузки\');",
  "INSERT INTO Features (Id, Name) VALUES (7, \'Отсутствует питание 380 В\');",
  "INSERT INTO Features (Id, Name) VALUES (8, \'Усилитель мощности неисправен\');",
  "INSERT INTO Features (Id, Name) VALUES (9, \'Ошибка аккумуляторной батареи\');",
  "INSERT INTO Features (Id, Name) VALUES (10, \'Ошибка в конфигурации устройства\');",
  "INSERT INTO Features (Id, Name) VALUES (11, \'Отсутствие звукового потока\');",
  "INSERT INTO Features (Id, Name) VALUES (12, \'Ручной запуск оповещения\');",
  "INSERT INTO Features (Id, Name) VALUES (13, \'Оповещение запущено другим центром\');",
  "INSERT INTO Features (Id, Name) VALUES (14, \'Включен микрофон\');",
  "INSERT INTO Features (Id, Name) VALUES (15, \'Нет связи с коммутатором\');",
  "INSERT INTO Features (Id, Name) VALUES (16, \'Отсутствует заготовленный сценарий\');",
  "INSERT INTO Features (Id, Name) VALUES (17, \'Отсутствует звуковой файл\');",
  "INSERT INTO Features (Id, Name) VALUES (18, \'Работа в автоматическом режиме\');",
  "INSERT INTO Features (Id, Name) VALUES (25, \'Обрыв линии\');",
  "INSERT INTO Features (Id, Name) VALUES (26, \'Короткое замыкание\');"
},
*tbl15 =
  "CREATE TABLE Abilities ("
  " Id      SMALLINT PRIMARY KEY,"
  " Name    VARCHAR(32)"
  ") WITH (fillfactor=90);",
*cnt15[] = {
  "INSERT INTO Abilities (Id, Name) VALUES (1, \'Устанавливает соединение\');",
  "INSERT INTO Abilities (Id, Name) VALUES (2, \'Принимает соединение\');",
  "INSERT INTO Abilities (Id, Name) VALUES (3, \'Поддерживает тех. контроль\');",
  "INSERT INTO Abilities (Id, Name) VALUES (4, \'Структура сети определена\');",
  "INSERT INTO Abilities (Id, Name) VALUES (5, \'Поддерживает обновление версий\');",
  "INSERT INTO Abilities (Id, Name) VALUES (6, \'Поддерживает запрос состояния\');",
  "INSERT INTO Abilities (Id, Name) VALUES (7, \'Поддерживает команды оповещения\');",
  "INSERT INTO Abilities (Id, Name) VALUES (8, \'Воспроизводит звук\');",
  "INSERT INTO Abilities (Id, Name) VALUES (9, \'Воспроизводит сирену\');",
  "INSERT INTO Abilities (Id, Name) VALUES (10, \'Принимает потоковый звук\');",
  "INSERT INTO Abilities (Id, Name) VALUES (11, \'Имеет заготовленные сценарии\');"
},
*tbl17 =
  "CREATE TABLE Channels ("
  " Id      SMALLINT PRIMARY KEY,"
  " Name    VARCHAR(32),"
  " DNum    SMALLINT NOT NULL,"
  " Tp      SMALLINT NOT NULL"
  ") WITH (fillfactor=90);",
*cnt17[] = {
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (49, \'Radio\', 1, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (50, \'GonetsM\', 4, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (51, \'CksTcpDevice\', 12, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (97, \'Ethernet\', 16, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (100, \'ModBus\', 20, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (111, \'OuterSystem\', 21, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (113, \'Kugo\', 46, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (114, \'П166ВАУ\', 38, 1);",
  "INSERT INTO Channels (Id, Name, DNum, Tp) VALUES (115, \'Sound\', 34, 2);"
},
*tbl19 =
  "CREATE TABLE PropValSets ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('PropValKeyGen'),"
  " Num     SMALLINT NOT NULL,"
  " Prop    SMALLINT NOT NULL,"
  " FOREIGN KEY(Prop) REFERENCES Properties(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",
*fun19 =
  "CREATE OR REPLACE FUNCTION PropValRem_trigger ()"
  " RETURNS TRIGGER AS $$"
  " BEGIN"
  "   DELETE FROM FtValue4 WHERE Id=OLD.Id;"
  "   DELETE FROM FtValue5 WHERE Id=OLD.Id;"
  "   DELETE FROM FtValue10 WHERE Id=OLD.Id;"
  "   DELETE FROM FtValue11 WHERE Id=OLD.Id;"
  "   DELETE FROM FtValue13 WHERE Id=OLD.Id;"
  "   DELETE FROM FtValue14 WHERE Id=OLD.Id;"
  "   DELETE FROM FtValue15 WHERE Id=OLD.Id;"
  "   RETURN NULL;"
  " END;"
  "$$ language plpgsql;",
*tr19[] = {
  "DROP TRIGGER IF EXISTS PrValSetRemTrigger ON PropValSets;",
  "CREATE TRIGGER PrValSetRemTrigger AFTER DELETE ON PropValSets"
  " FOR EACH ROW EXECUTE PROCEDURE PropValRem_trigger ();"},
*tbl20 =
  "CREATE TABLE TypeProperties ("
  " Tp      SMALLINT NOT NULL,"
  " Prop    SMALLINT NOT NULL,"
  " PRIMARY KEY(Tp, Prop),"
  " FOREIGN KEY(Tp) REFERENCES Types(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Prop) REFERENCES Properties(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",

*tbl25 =
  "CREATE TABLE TypeAbilities ("
  " Tp      SMALLINT NOT NULL,"
  " Abil    SMALLINT NOT NULL,"
  " PRIMARY KEY(Tp, Abil),"
  " FOREIGN KEY(Tp) REFERENCES Types(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Abil) REFERENCES Abilities(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",
*cnt25[] = {
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (1, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (1, 2);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (1, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (1, 8);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (1, 10);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (1, 11);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (2, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (2, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (2, 8);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (2, 9);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (2, 10);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (2, 11);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (3, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (3, 6);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (3, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (3, 9);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 6);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 8);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 9);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 10);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (4, 11);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (5, 2);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (5, 6);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (5, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (5, 9);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (6, 2);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (6, 6);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (6, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (6, 8);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (6, 9);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (6, 10);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (7, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (7, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (7, 8);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (7, 10);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (7, 11);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (8, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (8, 7);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (8, 8);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (8, 9);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (8, 10);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (8, 11);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (10, 2);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (11, 1);",
  "INSERT INTO TypeAbilities (Tp, Abil) VALUES (12, 1);"
},
*tbl26 =
  "CREATE TABLE TypeFeatures ("
  " Tp      SMALLINT NOT NULL,"
  " Ftr     SMALLINT NOT NULL,"
  " PRIMARY KEY(Tp, Ftr),"
  " FOREIGN KEY(Tp) REFERENCES Types(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Ftr) REFERENCES Features(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",
*tbl27 =
  "CREATE TABLE TypeChannels ("
  " Tp      SMALLINT NOT NULL,"
  " Chan    SMALLINT NOT NULL,"
  " PRIMARY KEY(Tp, Chan),"
  " FOREIGN KEY(Tp) REFERENCES Types(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Chan) REFERENCES Channels(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",
*cnt27[] = {
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (1, 97);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (1, 115);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (2, 97);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (2, 115);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (3, 97);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (4, 113);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (4, 115);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (5, 114);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (6, 114);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (6, 115);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (7, 97);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (7, 115);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (8, 111);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (10, 51);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (11, 113);",
  "INSERT INTO TypeChannels (Tp, Chan) VALUES (12, 97);"
},
*tbl28 = "CREATE TABLE EventTypes ("
  " Id      SMALLINT PRIMARY KEY,"
  " Name    VARCHAR(64),"
  " Rec     BOOLEAN DEFAULT FALSE"
  ") WITH (fillfactor=90);",
*cnt28[] = {
  "INSERT INTO EventTypes (Id, Name, Rec) VALUES (1, \'Выключение канала передачи\', FALSE);",
  "INSERT INTO EventTypes (Id, Name, Rec) VALUES (2, \'Изменение связи\', FALSE);",
  "INSERT INTO EventTypes (Id, Name, Rec) VALUES (3, \'Изменение признака\', FALSE);",
  "INSERT INTO EventTypes (Id, Name, Rec) VALUES (4, \'Нарушение алгоритма\', FALSE);",
  "INSERT INTO EventTypes (Id, Name, Rec) VALUES (5, \'Запуск сеанса оповещения\', FALSE);"
},

*tbl40 = "CREATE TABLE Systems ("
  " Id      SMALLINT PRIMARY KEY,"
  " Name    VARCHAR(64)"
  ") WITH (fillfactor=90);",
*seq50 = "CREATE SEQUENCE ItemsKeyGen START WITH 1 INCREMENT BY 1 CACHE"
    " 1 NO CYCLE;",
*tbl50 = "CREATE TABLE Items ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('ItemsKeyGen'),"
  " Parent  INTEGER,"
  " Name    VARCHAR(80),"
  " Num     INTEGER NOT NULL,"
  " Tp      SMALLINT NOT NULL,"
  " FOREIGN KEY(Tp) REFERENCES Types(Id)"
  ") WITH (fillfactor=80);",
*fun50 =
  "CREATE OR REPLACE FUNCTION RemItemCont_trigger ()"
  " RETURNS TRIGGER AS $$"
  " BEGIN"
  "   DELETE FROM Items WHERE Parent=OLD.Id;"
  "   RETURN NULL;"
  " END;"
  "$$ language plpgsql;",
*tr50[] = {
  "DROP TRIGGER IF EXISTS RemItemContTrigger ON Items;",
  "CREATE TRIGGER RemItemContTrigger AFTER DELETE ON Items"
  " FOR EACH ROW EXECUTE PROCEDURE RemItemCont_trigger ();"},
*tbl51 = "CREATE TABLE Objects ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('ItemsKeyGen'),"
  " Syst    SMALLINT NOT NULL,"
  " Act     BOOLEAN,"
  " FOREIGN KEY(Syst) REFERENCES Systems(Id)"
  ") WITH (fillfactor=100);",
*fun51 =
  "CREATE OR REPLACE FUNCTION RemObjCont_trigger ()"
  " RETURNS TRIGGER AS $$"
  " BEGIN"
  "   DELETE FROM Items WHERE Id=OLD.Id;"
  "   RETURN NULL;"
  " END;"
  "$$ language plpgsql;",
*tr51[] = {
  "DROP TRIGGER IF EXISTS RemObjContTrigger ON Objects;",
  "CREATE TRIGGER RemObjContTrigger AFTER DELETE ON Objects"
  " FOR EACH ROW EXECUTE PROCEDURE RemObjCont_trigger ();"},
*seq52 = "CREATE SEQUENCE ObjContKeyGen START WITH 1 INCREMENT BY 1 CACHE 1 NO CYCLE;",
*tbl52 = "CREATE TABLE ContObjects ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('ObjContKeyGen'),"
  " Obj     INTEGER NOT NULL,"
  " Cont    INTEGER NOT NULL,"
  " Lev     SMALLINT NOT NULL,"
  " CONSTRAINT ObjCnt UNIQUE (Obj, Cont),"
  " FOREIGN KEY(Obj) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Cont) REFERENCES Objects(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=100);",
*seq53 = "CREATE SEQUENCE ContChanKeyGen START WITH 1 INCREMENT BY 1 CACHE 1 NO CYCLE;",
// ALTER TABLE ContChannels ALTER Id SET DEFAULT nextval ('ObjShellKeyGen');
// ALTER TABLE ServChannels ALTER Id SET DEFAULT nextval ('ObjContKeyGen');
*tbl53 = "CREATE TABLE ContChannels ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('ContChanKeyGen'),"
  " Cont    INTEGER NOT NULL,"
  " Chan    SMALLINT NOT NULL,"
  " Num     SMALLINT NOT NULL,"
  " Addr    VARCHAR(40),"
  " Lev     SMALLINT DEFAULT 0,"
  " Con     BOOLEAN,"
  " CONSTRAINT CntCh UNIQUE (Cont, Chan, Num),"
  " FOREIGN KEY(Cont) REFERENCES ContObjects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Chan) REFERENCES Channels(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=80);",
*seq54 = "CREATE SEQUENCE ServChanKeyGen START WITH 1 INCREMENT BY 1 CACHE 1 NO CYCLE;",
*tbl54 = "CREATE TABLE ServChannels ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('ServChanKeyGen'),"
  " ObjId   INTEGER NOT NULL,"
  " Chan    SMALLINT NOT NULL,"
  " Num     SMALLINT NOT NULL,"
  " Mode    SMALLINT DEFAULT 0,"
  " Valid   BOOLEAN,"
  " CONSTRAINT ObjCh UNIQUE (ObjId, Chan, Num),"
  " FOREIGN KEY(ObjId) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Chan) REFERENCES Channels(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=80);",
*tbl55 = "CREATE TABLE ObjAbilities ("
  " Obj     INTEGER NOT NULL,"
  " Abil    SMALLINT NOT NULL,"
  " PRIMARY KEY(Obj, Abil),"
  " FOREIGN KEY(Obj) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Abil) REFERENCES Abilities(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=100);",
*tbl56 = "CREATE TABLE ItemFeatures ("
  " Itm     INTEGER NOT NULL,"
  " Ftr     SMALLINT NOT NULL,"
  " FtVal   SMALLINT NOT NULL,"
  " PRIMARY KEY(Itm, Ftr),"
  " FOREIGN KEY(Itm) REFERENCES Items(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Ftr) REFERENCES Features(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",
*tbl57 = "CREATE TABLE AbnRouteTable ("
  " ObjId   INTEGER PRIMARY KEY,"
  " Id      SMALLINT NOT NULL,"
  " FOREIGN KEY(ObjId) REFERENCES Objects(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=90);",
*seq59 = "CREATE SEQUENCE ObjShellKeyGen START WITH 1 INCREMENT BY 1 CACHE 1 NO CYCLE;",
*tbl59 = "CREATE TABLE ApplShells ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('ObjShellKeyGen'),"
  " ObjId   INTEGER NOT NULL,"
  " Num     SMALLINT NOT NULL,"
  " Addr    VARCHAR(40),"
  " Con     BOOLEAN,"
  " CONSTRAINT ObjNm UNIQUE (ObjId, Num),"
  " FOREIGN KEY(ObjId) REFERENCES Objects(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=80);",

*tbl60 = "CREATE TABLE ItkObjEvents ("
  " Dtm     TIMESTAMP NOT NULL,"
  " ObjId   INTEGER NOT NULL,"
  " ItId    INTEGER NOT NULL,"
  " ItType  SMALLINT NOT NULL,"
  " EvType  SMALLINT NOT NULL,"
  " Val     BYTEA,"
  " PRIMARY KEY (Dtm, ObjId, ItId, ItType),"
  " FOREIGN KEY(ObjId) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(EvType) REFERENCES EventTypes(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=100);",

*tbl61 = "CREATE TABLE ObjCenters ("
  " ObjNum  INTEGER NOT NULL,"
  " CntNum  INTEGER NOT NULL,"
  " Syst    SMALLINT NOT NULL,"
  " PRIMARY KEY (ObjNum, Syst)"
  ") WITH (fillfactor=100);",

*tbl62 = "CREATE TABLE ObjPropVals ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('PropValKeyGen'),"
  " Obj     INTEGER NOT NULL,"
  " Prop    SMALLINT NOT NULL,"
  " CONSTRAINT ObjPr UNIQUE (Obj, Prop),"
  " FOREIGN KEY(Obj) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Prop) REFERENCES Properties(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=100);",
*tr62[] = {
  "DROP TRIGGER IF EXISTS ObjPropRemTrigger ON ObjPropVals;",
  "CREATE TRIGGER ObjPropRemTrigger AFTER DELETE ON ObjPropVals"
  " FOR EACH ROW EXECUTE PROCEDURE PropValRem_trigger ();"},

*tbl63 = "CREATE TABLE ObjEventChanges ("
  " Dtm     TIMESTAMP NOT NULL,"
  " ObjId   INTEGER NOT NULL,"
  " ItId    INTEGER NOT NULL,"
  " Ft      SMALLINT NOT NULL,"
  " Pres    BOOLEAN,"
  " PRIMARY KEY (ItId, Ft),"
  " FOREIGN KEY(ObjId) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(ItId) REFERENCES Items(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Ft) REFERENCES Features(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=100);",
*tbl64 = "CREATE TABLE AbnConnChanges ("
  " Cont    INTEGER NOT NULL PRIMARY KEY,"
  " Dtm     TIMESTAMP NOT NULL,"
  " Pres    BOOLEAN,"
  " FOREIGN KEY(Cont) REFERENCES ContObjects(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=100);",

*seq67 = "CREATE SEQUENCE AlSeanceKeyGen START WITH 1 INCREMENT BY 1 CACHE 1 NO CYCLE;",
*tbl67 = "CREATE TABLE AlertSeance ("
  " Id      INTEGER PRIMARY KEY DEFAULT nextval ('AlSeanceKeyGen'),"
  " Dtm     TIMESTAMP NOT NULL,"
  " Code    BIGINT NOT NULL,"
  " Src     INTEGER NOT NULL,"
  " Syst    SMALLINT NOT NULL,"
  " Type    SMALLINT DEFAULT 0,"
  " Num     SMALLINT DEFAULT 0,"
  " Name    VARCHAR(128),"
  " FOREIGN KEY(Src) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Syst) REFERENCES Systems(Id)"
  ") WITH (fillfactor=80);",
*tbl68 = "CREATE TABLE AlertObjects ("
  " Seance  INTEGER NOT NULL,"
  " Abn     INTEGER NOT NULL,"
  " PRIMARY KEY (Seance, Abn),"
  " FOREIGN KEY(Seance) REFERENCES AlertSeance(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Abn) REFERENCES Objects(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=80);",
*tbl69 = "CREATE TABLE AlertEvents ("
  " Sid     INTEGER NOT NULL,"
  " Src     INTEGER NOT NULL,"
  " Oid     INTEGER NOT NULL,"
  " CNum    SMALLINT NOT NULL,"
  " Cid     SMALLINT NOT NULL,"
  " Dst     INTEGER NOT NULL,"
  " Dtm     TIMESTAMP NOT NULL,"
  " Val     BYTEA,"
  " PRIMARY KEY (SId, Src, Oid, CNum, Cid),"
  " FOREIGN KEY(Sid) REFERENCES AlertSeance(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Src) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Oid) REFERENCES Objects(Id) ON DELETE CASCADE,"
  " FOREIGN KEY(Dst) REFERENCES Objects(Id) ON DELETE CASCADE"
  ") WITH (fillfactor=80);",

*tbl100 = "CREATE TABLE CurrentObjNumber ("
  " Id      SMALLINT PRIMARY KEY,"
  " Num     SMALLINT NOT NULL"
  ") WITH (fillfactor=80);";

// -----------------------------------------------------------------------------
bool TechPgDataKeeper::checkData (QSqlDatabase &db)
{
  QSqlQuery q (db);
  int n = 1, i;
  bool res = false;

  getLogger ();
  try{
    if (!q.exec ("ANALYZE FtValue15;")){
      if (!q.exec (seq01)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
              "последовательности PropValKeyGen!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана "
            "последовательность PropValKeyGen.").arg (n ++), 0x15);
      if (!q.exec (tbl07)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "FtValue15!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " FtValue15.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE FtValue4;")){
      if (!q.exec (tbl01)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
              "таблицы FtValue4!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана "
            "таблица FtValue4.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE FtValue5;")){
      if (!q.exec (tbl02)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "FtValue5!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " FtValue5.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE FtValue10;")){
      if (!q.exec (tbl03)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "FtValue10!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " FtValue10.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE FtValue11;")){
      if (!q.exec (tbl04)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "FtValue11!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " FtValue11.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE FtValue13;")){
      if (!q.exec (tbl05)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "FtValue13!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " FtValue13.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE FtValue14;")){
      if (!q.exec (tbl06)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "FtValue14!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " FtValue14.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Properties;")){
      if (!q.exec (tbl10)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Properties!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " Properties.").arg (n ++), 0x15);
      for (i = 0; i < 2; ++ i){
        if (!q.exec (cnt10[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу Properties.").arg (n ++), 0x15);
    }

    if (!q.exec ("ANALYZE Types;")){
      if (!q.exec (tbl12)){
        if (m_Log)
          m_Log->writeLog ("ItkPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Types!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("ItkPgDataKeeper | checkData | %1. Создана таблица"
            " Types.").arg (n ++), 0x15);
      for (i = 0; i < 22; ++ i){
        if (!q.exec (cnt12[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу Types.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Features;")){
      if (!q.exec (tbl14)){
        if (m_Log)
          m_Log->writeLog ("ItkPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Features!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("ItkPgDataKeeper | checkData | %1. Создана таблица"
            " Features.").arg (n ++), 0x15);
      for (i = 0; i < 20; ++ i){
        if (!q.exec (cnt14[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу Features.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Abilities;")){
      if (!q.exec (tbl15)){
        if (m_Log)
          m_Log->writeLog ("ItkPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Abilities!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("ItkPgDataKeeper | checkData | %1. Создана таблица"
            " Abilities.").arg (n ++), 0x15);
      for (i = 0; i < 11; ++ i){
        if (!q.exec (cnt15[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу Abilities.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Channels;")){
      if (!q.exec (tbl17)){
        if (m_Log)
          m_Log->writeLog ("ItkPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Channels!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("ItkPgDataKeeper | checkData | %1. Создана таблица"
            " Channels.").arg (n ++), 0x15);
      for (i = 0; i < 9; ++ i){
        if (!q.exec (cnt17[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу Channels.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE PropValSets;")){
      if (!q.exec (tbl19)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "PropValSets!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " PropValSets.").arg (n ++), 0x15);
      if (!q.exec (fun19)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании функции "
              "PropValRem_trigger!", 0x15, 6);
        throw 1;
      }
      for (i = 0; i < 2; ++ i){
        if (!q.exec (tr19[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлен триггер"
            " для таблицы PropValSets.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE TypeProperties;")){
      if (!q.exec (tbl20)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "TypeProperties!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " TypeProperties.").arg (n ++), 0x15);
    }

    if (!q.exec ("ANALYZE TypeAbilities;")){
      if (!q.exec (tbl25)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "TypeAbilities!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " TypeAbilities.").arg (n ++), 0x15);
      for (i = 0; i < 47; ++ i){
        if (!q.exec (cnt25[i])){
          throw 1;
          if (m_Log)
            m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при добавлении "
                "данных в таблицу TypeAbilities!", 0x15, 6);
        }
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу TypeAbilities.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE TypeFeatures;")){
      if (!q.exec (tbl26)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "TypeFeatures!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " TypeFeatures.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE TypeChannels;")){
      if (!q.exec (tbl27)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "TypeChannels!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " TypeChannels.").arg (n ++), 0x15);
      for (i = 0; i < 16; ++ i){
        if (!q.exec (cnt27[i])){
          if (m_Log)
            m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при добавлении "
                "данных в таблицу TypeChannels!", 0x15, 6);
          throw 1;
        }
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу TypeChannels.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE EventTypes;")){
      if (!q.exec (tbl28)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "EventTypes!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " EventTypes.").arg (n ++), 0x15);
      for (i = 0; i < 5; ++ i){
        if (!q.exec (cnt28[i])){
          if (m_Log)
            m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при добавлении "
                "данных в таблицу EventTypes!", 0x15, 6);
          throw 1;
        }
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлены данные"
            " в таблицу EventTypes.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Systems;")){
      if (!q.exec (tbl40)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Systems!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " Systems.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Items;")){
      if (!q.exec (seq50)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
              "последовательности ItemsKeyGen!", 0x15, 6);
        throw 1;
      }
      if (!q.exec (tbl50)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Items!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " Items.").arg (n ++), 0x15);
      if (!q.exec (fun50)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании функции "
              "RemItemCont_trigger!", 0x15, 6);
        throw 1;
      }
      for (i = 0; i < 2; ++ i){
        if (!q.exec (tr50[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлен триггер"
            " для таблицы Items.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE Objects;")){
      if (!q.exec (tbl51)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "Objects!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " Objects.").arg (n ++), 0x15);
      if (!q.exec (fun51)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании функции "
              "RemObjCont_trigger!", 0x15, 6);
        throw 1;
      }
      for (i = 0; i < 2; ++ i){
        if (!q.exec (tr51[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлен триггер"
            " для таблицы Objects.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ContObjects;")){
      if (!q.exec (seq52)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
              "последовательности ObjContKeyGen!", 0x15, 6);
        throw 1;
      }
      if (!q.exec (tbl52)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ContObjects!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ContObjects.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ContChannels;")){
      if (!q.exec (seq53)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
                "последовательности ContChanKeyGen!", 0x15, 6);
        throw 1;
      }
      if (!q.exec (tbl53)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ContChannels!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ContChannels.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ServChannels;")){
      if (!q.exec (seq54)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
                "последовательности ServChanKeyGen!", 0x15, 6);
        throw 1;
      }
      if (!q.exec (tbl54)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ServChannels!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ServChannels.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ObjAbilities;")){
      if (!q.exec (tbl55)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ObjAbilities!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ObjAbilities.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ItemFeatures;")){
      if (!q.exec (tbl56)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ItemFeatures!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ItemFeatures.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE AbnRouteTable;")){
      if (!q.exec (tbl57)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "AbnRouteTable!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " AbnRouteTable.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ApplShells;")){
      if (!q.exec (seq59)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
                "последовательности ObjShellKeyGen!", 0x15, 6);
        throw 1;
      }
      if (!q.exec (tbl59)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ApplShells!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ApplShells.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ItkObjEvents;")){
      if (!q.exec (tbl60)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ItkObjEvents!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ItkObjEvents.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ObjCenters;")){
      if (!q.exec (tbl61)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ObjCenters!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ObjCenters.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ObjPropVals;")){
      if (!q.exec (tbl62)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ObjPropVals!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ObjPropVals.").arg (n ++), 0x15);
      for (i = 0; i < 2; ++ i){
        if (!q.exec (tr62[i]))
          throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Добавлен триггер"
            " для таблицы ObjPropVals.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE ObjEventChanges;")){
      if (!q.exec (tbl63)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "ObjEventChanges!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " ObjEventChanges.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE AbnConnChanges;")){
      if (!q.exec (tbl64)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "AbnConnChanges!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " AbnConnChanges.").arg (n ++), 0x15);
    }

    if (!q.exec ("ANALYZE AlertSeance;")){
      if (!q.exec (seq67)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании "
                "последовательности AlSeanceKeyGen!", 0x15, 6);
        throw 1;
      }
      if (!q.exec (tbl67)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "AlertSeance!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " AlertSeance.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE AlertObjects;")){
      if (!q.exec (tbl68)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "AlertObjects!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " AlertObjects.").arg (n ++), 0x15);
    }
    if (!q.exec ("ANALYZE AlertEvents;")){
      if (!q.exec (tbl69)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "AlertEvents!", 0x15, 6);
        throw 1;
      }
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " AlertEvents.").arg (n ++), 0x15);
    }

    if (!q.exec ("ANALYZE CurrentObjNumber;")){
      if (!q.exec (tbl100)){
        if (m_Log)
          m_Log->writeLog ("TechPgDataKeeper | checkData | Ошибка при создании таблицы "
              "CurrentObjNumber!", 0x15, 6);
        throw 1;
      }
      q.exec ("INSERT INTO CurrentObjNumber (Id, Num) VALUES (1, 4);");
      q.exec ("INSERT INTO CurrentObjNumber (Id, Num) VALUES (2, 2);");
      q.exec ("INSERT INTO CurrentObjNumber (Id, Num) VALUES (3, 1);");
      if (m_Log)
        m_Log->writeLog (QString ("TechPgDataKeeper | checkData | %1. Создана таблица"
            " CurrentObjNumber.").arg (n ++), 0x15);
    }
    res = true;
  }
  catch (...){
  }
  return res;
}

// -----------------------------------------------------------------------------
QString TechPgDataKeeper::valTableName (quint8 tp)
{
  QString str = "FtValue";

  switch (tp){
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    str += "4";
    break;
  case 7:
  case 8:
    str += "5";
    break;
  case 9:
    str += "11";
    break;
  case 10:
    str += "10";
    break;
  case 11:
  case 15:
  case 16:
    str += "13";
    break;
  case 12:
  case 17:
  case 18:
    str += "15";
    break;
  case 13:
  case 14:
    str += "14";
    break;
  }
  return str;
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::variantToValue (QVariant &qv, Value &val, quint8 tp)
{
  QDateTime dtm;
  Value v;
  QString str;
  QByteArray ba;

  switch (tp){
  case 1:
  case 3:
  case 5:
    qv.toInt ();
    val.SetBsValue (new Integer32 (qv.toInt ()));
    break;
  case 2:
  case 4:
  case 6:
    val.SetBsValue (new UInteger32 (qv.toUInt ()));
    break;
  case 7:
    val.SetBsValue (new Integer64 (qv.toLongLong ()));
    break;
  case 8:
    val.SetBsValue (new UInteger64 (qv.toULongLong ()));
    break;
  case 9:
    val.SetBsValue (new Double (qv.toDouble ()));
    break;
  case 10:
    val.SetBsValue (new Boolean (qv.toBool ()));
    break;
  case 11:
    val.SetBsValue (new String (qv.toString ()));
    break;
  case 12:
  case 18:
    val.SetBsValue (new Binary (qv.toByteArray ()));
    break;
  case 13:
  case 14:
    if (tp == 13){
      str = qv.toDate ().toString ("yyyy.MM.ddT00:00:00");
      dtm = QDateTime::fromString (str, "yyyy.MM.ddThh:mm:ss");
    }
    else
      dtm = qv.toDateTime ();
    val.SetBsValue (new DateTime (dtm.toSecsSinceEpoch ()));
    break;
  case 15:
    val.SetBsValue (new Ipv4Addr (qv.toString ()));
    break;
  case 16:
    val.SetBsValue (new Ipv4Addr (qv.toString (), 200));
    break;
  case 17:
    ba = qv.toByteArray ();
    Value::fromMessagePackBinary (ba.data (), quint32(ba.size ()), val);
    if (v.isValid () && v.getType () == VT_ArDouble)
      val = v;
    break;
  }
}

// -----------------------------------------------------------------------------
void TechPgDataKeeper::valueToVariant (Value &val, QVariant &qv, quint8 tp)
{
  UnValNumber *vn;
  UnValDecimal *vd;
  QDateTime dtm;

  qv.clear ();
  if (val.isValid ()){
    switch (tp){
    case 1:
    case 3:
    case 5:
      if ((vn = dynamic_cast<UnValNumber*>(val.GetBsValue ())) != nullptr)
        qv = vn->getInt ();
      break;
    case 2:
    case 4:
    case 6:
      if ((vn = dynamic_cast<UnValNumber*>(val.GetBsValue ())) != nullptr)
        qv = vn->getUInt ();
      break;
    case 7:
      if ((vn = dynamic_cast<UnValNumber*>(val.GetBsValue ())) != nullptr)
        qv = vn->getInt64 ();
      break;
    case 8:
      if ((vn = dynamic_cast<UnValNumber*>(val.GetBsValue ())) != nullptr)
        qv = vn->getUInt64 ();
      break;
    case 9:
      if ((vd = dynamic_cast<UnValDecimal*>(val.GetBsValue ())) != nullptr)
        qv = vd->getDoubleVal ();
      break;
    case 10:
      if (val.getType () == VT_Bool)
        qv = static_cast<Boolean*>(val.GetBsValue ())->getBoolean ();
      break;
    case 11:
    case 15:
      qv = val.toString ();
      break;
    case 12:
    case 18:
      if (val.getType () == VT_Bin)
        qv = static_cast<Binary*>(val.GetBsValue ())->getByteArray ();
      break;
    case 13:
    case 14:
      if (val.getType () == VT_DateTime)
        dtm = static_cast<DateTime*>(val.GetBsValue ())->getDateTime ();
      if (tp == 13)
        qv = dtm.date ();
      else
        qv = dtm;
      break;
    case 16:
      if (val.getType () == VT_Ipv4Addr)
        qv = static_cast<Ipv4Addr*>(val.GetBsValue ())->getAddrStr ();
      break;
    case 17:
      if (val.getType () == VT_ArDouble)
        qv = val.toMessagePack ();
      break;
    }
  }
}

// -----------------------------------------------------------------------------
} // namespace ItkSrv
