/*
 * Работа с базой данных
 */

import {pool} from './db.js';
import {logger,po} from './logger.js';


function delay(ms) {
  return new Promise((resolve, reject) => {
    setTimeout(resolve, ms);
  });
}

export default class eventsJournal {
  constructor(
  ) {
    this._isConnected = false;
    this._idSess = 0;
  }
  async run() {
    let cntTimeout = 4;
    while(!this._isConnected) {
      try{
        pool.on('connect', (client) => {
          logger.info('[poolevent] - connect');
        });
        pool.on('acquire', (client) => {
          logger.info('[poolevent] - acquire');
        });
        pool.on('error', (err, client) => {
          logger.info('[poolevent] - error %s',po(err));
        });
        pool.on('release', (err, client) => {
          logger.info('[poolevent] - release %s',po(err));
        });
        pool.on('remove', (client) => {
          logger.info('[poolevent] - remove');
        });

        let res = await pool.query('SELECT NOW()');
        logger.info('Connected to the database: %s ', po(res.rows));
        this._isConnected = true;
        let drop_chunks = async ()=>{
          try{
            {
              let selectdrop  = "SELECT drop_chunks('events_schema.events_table', older_than => INTERVAL '" + process.env.DB_DROP_INTERVAL + "');"
              const result = await pool.query(selectdrop);
              logger.info('eventsJournal: drop older_than result:%s', po(result.rows[0]));
            }
/*
            {
              let selectdrop  = "SELECT drop_chunks('events_schema.events_table', newer_than =>  INTERVAL '1 day');"
              const result = await pool.query(selectdrop);
              logger.info('eventsJournal: drop newer_than  result:%s', po(result.rows[0]));
            }
*/
//            const result = await pool.query(
//              "SELECT drop_chunks('events_schema.events_table', older_than => INTERVAL '$1',newer_than => now() + interval '1 day');",
//              [process.env.DB_DROP_INTERVAL]);
          }
          catch(e) {
            logger.error(e,'Error drop_chunks');
          }
        };
        await drop_chunks();
        setInterval(async ()=>{
          await drop_chunks();
        }, 1000*3600*24*7); // 1 week, 604800000
      } catch(err) {
        logger.error(err.stack,'Error connecting to the database');
        cntTimeout--;
        if(cntTimeout === 0) {
          break;
        }
        await delay(1000);
      }
    }
  }
  async insert_raw(date_ev_sec ,name_ev, description_ev,version_data_ev, data_ev) {
    if( name_ev === "alarmStart") {
      this._idSess  = data_ev.idSess;
    }
    if(!this._isConnected) {
      throw new Error();
    }
    try {
      const result = await pool.query('INSERT INTO events_schema.events_table  (date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev, data_ev) VALUES (TO_TIMESTAMP($1), $2, $3, $4, $5, $6) RETURNING *',
                                      [date_ev_sec ,name_ev, description_ev,this._idSess ,version_data_ev, data_ev]);
      logger.info('eventsJournal: date_ev_sec: %s name_ev=%s description_ev=%s idSess=%s',
                  date_ev_sec ,name_ev, description_ev, this._idSess);
      logger.trace('eventsJournal: insert: %s', po(result.rows));
      if( name_ev === "alarmEnd") {
        this._idSess  = 0;
      }
      return result.rows;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: insert");
      throw(err);
    }
  }

  async insert(name_ev, description_ev,version_data_ev, data_ev) {
    if( name_ev === "alarmStart") {
      this._idSess  = data_ev.idSess;
    }
    if(!this._isConnected) {
      throw new Error();
    }
    try {
      let unixtime_sec = Math.round(new Date().getTime() / 1000)
      const result_ts = await pool.query('SELECT TO_TIMESTAMP($1)', [unixtime_sec]);
      logger.trace('eventsJournal: ts:%s', po(result_ts.rows[0].to_timestamp));
      let date_ev =  unixtime_sec
      let human_date_ev =  result_ts.rows[0].to_timestamp;
      const result = await pool.query('INSERT INTO events_schema.events_table  (date_ev , human_date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev, data_ev) VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING *',
                                      [date_ev,human_date_ev,name_ev, description_ev,this._idSess ,version_data_ev, data_ev]);
      logger.info('eventsJournal: date_ev=%s human_date_ev=%s  name_ev=%s description_ev=%s idSess=%s',
                  date_ev , human_date_ev ,name_ev, description_ev,this._idSess);
      logger.trace('eventsJournal: insert: %s', po(result.rows));

      if( name_ev === "alarmEnd") {
        this._idSess  = 0;
        this._idAlarmEnd  = result.rows[0].id;
      }
      return result.rows[0].id;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: insert");
      throw(err);
    }
  }
  /*
   * Функция для добавление в поле data_ev дополнительной информации
   *  data_ev - данные в формате json
   *  id записи полученный при вызове insert
   *  если id не задан то берется id последнейго события alarmEnd
   */
  async add(data_ev , id) {
    if(!this._isConnected) {
      return;
    }
    if(id === undefined) {
      if(this._idAlarmEnd === undefined) {
        return
      }
      id = this._idAlarmEnd;
    }
    try {
      const onerecord = await pool.query(
         'SELECT * FROM events_schema.events_table WHERE (id = $1)',[id]);
      let new_data_ev  = {...data_ev,...onerecord.rows[0].data_ev}
      const result = await pool.query('UPDATE events_schema.events_table SET  data_ev = $1  WHERE id = $2 RETURNING *',
                                      [ new_data_ev, id]);
      logger.trace('eventsJournal: add: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: add");
    }
  }

  async update(data_ev , id) {
    if(!this._isConnected) {
      return;
    }
    if(id === undefined) {
      return
    }
    try {
      const result = await pool.query('UPDATE events_schema.events_table SET  data_ev = $1  WHERE id = $2 RETURNING *',
                                      [data_ev, id]);
      logger.trace('eventsJournal: update: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: update");
    }
  }

  async delete(id) {
    if(!this._isConnected) {
      return;
    }
    try {
      const result =await pool.query('DELETE FROM events_schema.events_table WHERE id = $1', [id]);
      logger.trace('eventsJournal: delete: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: delete");
    }
  }
/*
 {
    "idEv":<integer>,         // Если поле задано то в ответ на запрос отправляется
                              // запись с id заданным в idEv , остальные опции запроса игнорируются
    "startDate":<unix time>,  // Начальная дата из диапазона запроса , по времени
    "endDate":<unix time>,    // Конечная дата из диапазона запроса , по времени
    "evType":[
      "alarmStart",           // Запуск оповещения
      "alarmEnd",             // Завершение оповещения
      "connected",            // Соединение установлено (транк, астериск, модем, устройтство)
      "disconnected",         // Соединение разорваное (транк, астериск, модем, устройтство)
      "playSound",            // Запуск воспроизведения звукозаписи
      "stopSound",            // Остановка воспроизведения звукозаписи
    ],
    "idSess":<integer>,       // Номер сеанса оповещения
    "allFields":<boolean>     // Если поле задано и равно true то в ответ на запрос отдаются все поля
                                 записи базы данных
  }
  Поля startDate и  endDate должны быть всегда, кроме запроса по idSess
  Поле evType может отсутствовать или быть пустым массивом
  Если поле idSess присутствует то поля  startDate  endDate не учавствуют в запросе
 */


  async get(request) {
    let req = request;
    try {
      let result;
      let fields_part = "date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev";
      let fields_full = "*";
      let fields = fields_part;
      if(req.allFields !== undefined) {
        if(req.allFields === true) {
          logger.trace("get full");
          fields = fields_full;
        }
      }
      if(req.idEv !== undefined) {
          result = await pool.query('SELECT * FROM events_schema.events_table WHERE (id = $1)',
                                  [req.idEv]);

      }
      else if(req.idSess === undefined) {

        if((req.evType !== undefined) && (req.evType.length !== 0) )
        {
          if (fields === "*") {
            result = await pool.query('SELECT * FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)) AND (name_ev = ANY($3::text[])) ORDER BY id DESC, human_date_ev DESC  ',
                                    [req.startDate,req.endDate,req.evType]);
          }
          else {
            result = await pool.query('SELECT id, date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)) AND (name_ev = ANY($3::text[])) ORDER BY id DESC, human_date_ev DESC  ',
                                    [req.startDate,req.endDate,req.evType]);
          }
        }
        else {
          if (fields === "*") {
            result = await pool.query('SELECT * FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)) ORDER BY id DESC, human_date_ev DESC',
                                      [req.startDate,req.endDate]);
          }
          else {
            result = await pool.query('SELECT id, date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)) ORDER BY id DESC, human_date_ev DESC',
                                      [req.startDate,req.endDate]);
          }
        }
      }
      else {
        if((req.evType !== undefined) && (req.evType.length !== 0) ) {
          if (fields === "*") {
            result = await pool.query('SELECT * FROM events_schema.events_table WHERE (name_ev = ANY($1::text[])) AND (id_session_ev = $2) ORDER BY id DESC, human_date_ev DESC',
                                  [req.evType, req.idSess ]);
          }
          else {
            result = await pool.query('SELECT id, date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev FROM events_schema.events_table WHERE (name_ev = ANY($1::text[])) AND (id_session_ev = $2) ORDER BY id DESC, human_date_ev DESC',
                                  [req.evType, req.idSess ]);
          }
        }
        else {
          if (fields === "*") {
            result = await pool.query('SELECT * FROM events_schema.events_table WHERE  (id_session_ev = $1) ORDER BY id DESC, human_date_ev DESC',
                                  [req.idSess ]);
          }
          else {
            result = await pool.query('SELECT id, date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev  FROM events_schema.events_table WHERE  (id_session_ev = $1) ORDER BY id DESC, human_date_ev DESC',
                                  [req.idSess ]);
          }
        }
      }
      logger.info('eventsJournal: get length: %s ', result.rows.length );
      logger.trace('eventsJournal: get: %s ',  po(result.rows));
      return  result.rows;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: get");
    }
  }

  async getAll() {
    try {
      const result = await pool.query('SELECT * FROM events_schema.events_table ORDER BY id DESC, human_date_ev DESC ');
      logger.info('eventsJournal: get all: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: get all");
    }
  }

  async delete() {
    try {
      const result = await pool.query('SELECT drop_chunks(events_schema.events_table, older_than => INTERVAL \'5 seconds\'  ) ');
      logger.info('eventsJournal: delete: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: delete");
    }
  }

};
