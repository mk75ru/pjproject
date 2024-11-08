/*
 * Работа с базой данных
 */

import {pool} from './db.js';
import {logger,po} from './logger.js';

export default class eventsJournal {
  constructor(
  ) {this._isConnected = false;}
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
  async insert(name_ev, description_ev,version_data_ev, data_ev) {
    if(!this._isConnected) {
      throw new Error();
    }
    try {
      const result_ts = await pool.query('SELECT TO_TIMESTAMP($1)', [Math.round(new Date().getTime() / 1000)]);
      logger.info('eventsJournal: ts:%s', po(result_ts.rows[0].to_timestamp));
      let date_ev =  result_ts.rows[0].to_timestamp;
      const result = await pool.query('INSERT INTO events_schema.events_table  (date_ev ,name_ev, description_ev, version_data_ev, data_ev) VALUES ($1, $2, $3, $4, $5) RETURNING *',
                                      [date_ev ,name_ev, description_ev, version_data_ev, data_ev]);
      logger.info('eventsJournal: insert: %s', po(result.rows));
      return result.rows[0].id;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: insert");
      throw(err);
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
                                      [ data_ev, id]);
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

  async getAll() {
    try {
      const result = await pool.query('SELECT * FROM events_schema.events_table');
      logger.info('eventsJournal: get all: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: get all");
    }
  }


};
