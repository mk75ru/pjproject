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
      logger.info('eventsJournal: insert: %s', po(result.rows));
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
      logger.info('eventsJournal: ts:%s', po(result_ts.rows[0].to_timestamp));
      let date_ev =  unixtime_sec
      let human_date_ev =  result_ts.rows[0].to_timestamp;
      logger.info('eventsJournal: date_ev:%s',date_ev);
      logger.info('eventsJournal: human_date_ev:%s',po(human_date_ev));
      const result = await pool.query('INSERT INTO events_schema.events_table  (date_ev , human_date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev, data_ev) VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING *',
                                      [date_ev,human_date_ev,name_ev, description_ev,this._idSess ,version_data_ev, data_ev]);
      logger.info('eventsJournal: insert: %s', po(result.rows));
      if( name_ev === "alarmEnd") {
        this._idSess  = 0;
      }
      return result.rows;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: insert");
      throw(err);
    }
  }

  async update(human_data_ev , id) {
    if(!this._isConnected) {
      return;
    }
    if(id === undefined) {
      return
    }
    try {
      const result = await pool.query('UPDATE events_schema.events_table SET  human_data_ev = $1  WHERE id = $2 RETURNING *',
                                      [ human_data_ev, id]);
      logger.info('eventsJournal: update: %s ',  po(result.rows));
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
      logger.info('eventsJournal: delete: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: delete");
    }
  }
/*
 {
    "startDate":<unix time>,  // Начальная дата из диапазона запроса , по времени
    "endDate":<unix time>,    // Конечная дата из диапазона запроса , по времени
    "evType":[
      "alarmStart",           // Запуск оповещения
      "alarmEnd",             // Завершение оповещения
      "connected",            // Соединение установлено (транк, астериск, модем, устройтство)
      "disconnected",         // Соединение разорваное (транк, астериск, модем, устройтство)
      "palySound",            // Запуск воспроизведения звукозаписи
      "stopSound",            // Остановка воспроизведения звукозаписи
    ],
    "idSess":<integer>        // Номер сеанса оповещения
  }
  Поля startDate и  endDate должны быть всегда, кроме запроса по idSess
  Поле evType может отсутствовать или быть пустым массивом
  Если поле idSess присутствует то поля  startDate  endDate не учавствуют в запросе
 */

  async get(request) {
    let req =  JSON.parse(request);
    try {
      let result;
      if(req.idSess === undefined) {

        if((req.evType !== undefined) && (req.evType.length !== 0) )
        {
          result = await pool.query('SELECT * FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)) AND (name_ev = ANY($3::text[]))  ',
                                      [req.startDate,req.endDate,req.evType]);
        }
        else {
          result = await pool.query('SELECT * FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))',
                                      [req.startDate,req.endDate]);
        }
      }
      else {
        if((req.evType !== undefined) && (req.evType.length !== 0) ) {
          result = await pool.query('SELECT * FROM events_schema.events_table WHERE (name_ev = ANY($1::text[])) AND (id_session_ev = $2)',
                                  [req.evType, req.idSess ]);
        }
        else {
          result = await pool.query('SELECT * FROM events_schema.events_table WHERE  (id_session_ev = $1)',
                                  [req.idSess ]);
        }
      }
      logger.info('eventsJournal: get: %s ',  po(result.rows));
      return  result.rows;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: get");
    }
  }


  async getAll() {
    try {
      const result = await pool.query('SELECT * FROM events_schema.events_table');
      logger.info('eventsJournal: get all: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: get all");
    }
  }


};
