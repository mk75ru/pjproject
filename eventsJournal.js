/*
 * Работа с базой данных
 */

import pg from 'pg'
const { Pool } = pg
let pool;
import 'dotenv/config'


import {logger,po} from './logger.js';



import { fileURLToPath } from 'url';
import { dirname } from 'path';
import { log } from 'console';
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename)







function delay(ms) {
  return new Promise((resolve, reject) => {
    setTimeout(resolve, ms);
  });
}

class eventsJournal {
  constructor(amountAlarmClients) {
    this._isConnected = false;
    this._idSess = 0;
    this._isRunning = false;
    if(amountAlarmClients === undefined) {
      this._amountAlarmClients=1;
    }
    else {
      this._amountAlarmClients=amountAlarmClients;
    }
  }
  getModuleDir() {
    return __dirname + "/";
  }
  setNewSession(idSess) {
    this._isRunning = true;
    this._counterAlarmStart = this._amountAlarmClients;
    this._counterAlarmEnd   = this._amountAlarmClients;
    this._idSess  = idSess;
    this._dataEv  = {}
  }
  isRunning() {
    return this._isRunning;
  }
  async stop() {
    if (this._client !== undefined) {
      if (this._timer !== undefined ) clearInterval(this._timer);
      this._timer = undefined;
      this._client.release();
      this._client = undefined;
      this._isConnected = false;
      await pool.end();
      logger.info('[poolstop] - pool is stopped');
    }
  }

  async run(amountAlarmClients) {
    if(amountAlarmClients === undefined) {
      this._amountAlarmClients=1;
    }
    else {
      this._amountAlarmClients=amountAlarmClients;
    }

    pool = new Pool({
      user: process.env.DB_USER,
      host: process.env.DB_HOST,
      database: process.env.DB_NAME,
      password: process.env.DB_PASSWORD,
      port: process.env.DB_PORT,
    });

    let cntTimeout = 4;
    while(!this._isConnected) {
      try{
        pool.on('connect', (client) => {
          logger.trace('[poolevent] - connect');
        });
        pool.on('acquire', (client) => {
          logger.trace('[poolevent] - acquire');
        });
        pool.on('error', (err, client) => {
          logger.trace('[poolevent] - error %s',po(err));
        });
        pool.on('release', (err, client) => {
          logger.info('[poolevent] - release %s',po(err));
        });
        pool.on('remove', (client) => {
          logger.trace('[poolevent] - remove');
        });
        this._client = await pool.connect();
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
          }
          catch(e) {
            logger.error(e,'Error drop_chunks');
          }
        };
        await drop_chunks();
        this._timer =  setInterval(async ()=>{
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
  async insert(name_ev, description_ev, version_data_ev, data_ev) {
    if(!this._isConnected) {
      throw new Error();
    }
    try {
      let data_ev_save = null;
      if(( name_ev === "alarmStart") && (this._counterAlarmStart > 0)) {
        this._dataEv["alarmStart"] = {...this._dataEv["alarmStart"],...data_ev}
        this._counterAlarmStart--;
        if(this._counterAlarmStart === 0) {
          data_ev_save = this._dataEv["alarmStart"];
        }
      }
      else
      if(( name_ev === "alarmEnd") && (this._counterAlarmEnd > 0)) {
        this._dataEv["alarmEnd"] = {...this._dataEv["alarmEnd"],...data_ev}
        this._counterAlarmEnd--;
        if(this._counterAlarmEnd === 0) {
          data_ev_save = this._dataEv["alarmEnd"];
        }
      }
      else {
        data_ev_save = data_ev;
      }

      if(data_ev_save !== null) {
        let unixtime_sec = Math.round(new Date().getTime() / 1000)
        const result_ts = await pool.query('SELECT TO_TIMESTAMP($1)', [unixtime_sec]);
        logger.trace('eventsJournal: ts:%s', po(result_ts.rows[0].to_timestamp));
        let date_ev =  unixtime_sec
        let human_date_ev =  result_ts.rows[0].to_timestamp;
        const result = await pool.query('INSERT INTO events_schema.events_table  (date_ev , human_date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev, data_ev) VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING *',
                                      [date_ev,human_date_ev,name_ev, description_ev,this._idSess ,version_data_ev, data_ev_save]);
        logger.info('eventsJournal: date_ev=%s human_date_ev=%s  name_ev=%s description_ev=%s idSess=%s',
                    date_ev , human_date_ev ,name_ev, description_ev,this._idSess);
        logger.trace('eventsJournal: insert: %s', po(result.rows));
        if( name_ev === "alarmEnd") {
          this._idSess  = 0;
        }
        return result.rows[0];
      }
      return null;
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
                                      [data_ev, id]);
      logger.trace('eventsJournal: update: %s ',  po(result.rows));
    } catch (err) {
      logger.error(err.stack,"eventsJournal: update");
    }
  }


  // Версия с возможностью указать поле name_ev (если оно может называться по-разному)
async  getDataWithCustomNameEv_(tableName, timeField, nameEvField, startTime, endTime, targetValues = []) {
    //const client = await pool.connect();                      
    try {
        const targetPlaceholders = targetValues.map((_, index) => `$${index + 3}`).join(', ');
        logger.info("\ntargetPlaceholders: %s\n",targetPlaceholders);
        const query = `
 WITH target_in_range AS (
     SELECT EXISTS (
         SELECT 1 FROM ${tableName}
         WHERE ${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)
                            AND ${nameEvField} IN (${targetPlaceholders})
     ) as has_target
 )
     -- Временный вывод для отладки
, debug_output AS (
    SELECT has_target, 
           'Debug: has_target = ' || has_target::text as debug_info
    FROM target_in_range
)
SELECT *, (SELECT debug_info FROM debug_output) as debug
FROM ${tableName} t
WHERE 
    -- Всегда показываем события диапазона |$1| |$2| |${timeField}|    
    (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
     OR
     (
         -- Если в диапазоне нет целевых событий, добавляем последние целевые события перед диапазоном
         (SELECT NOT has_target FROM target_in_range)
         AND ${timeField} < TO_TIMESTAMP($1)
         AND ${nameEvField} IN (${targetPlaceholders})
         AND ${timeField} = (
             SELECT MAX(${timeField})
             FROM ${tableName}
             WHERE ${timeField} < TO_TIMESTAMP($1)
             AND ${nameEvField} = t.${nameEvField}
             AND ${nameEvField} IN (${targetPlaceholders})
         )
     )
   ORDER BY id DESC, ${timeField} DESC
        `;
const query_ = `
SELECT *
FROM ${tableName} t
WHERE 
    (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
ORDER BY id DESC, ${timeField} DESC
        `;        
        //-- ORDER BY ${timeField}
        logger.info("\nquery: %s",query);
        //const startTimeStr = startTime.toISOString();
        //const endTimeStr = endTime.toISOString();        
        //const startTime_ = new Date(startTime);
        //const endTime_ = new Date(endTime);
        const params = [startTime, endTime, ...targetValues];
        const params_ = [startTime, endTime];
        logger.info("\nparams: %s",params);
        const result = await pool.query(query, params);
        //let result = await pool.query('SELECT * FROM events_schema.events_table WHERE (human_date_ev BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)) ORDER BY id DESC, human_date_ev DESC',
        //                              [startTime,endTime]);
        logger.info('eventsJournal: getDataWithCustomNameEv: %s ',  po(result.rows));
        return result.rows;
        
    } catch (error) {
        console.error('Error executing query:', error);
        throw error;
    } finally {
        //client.release();
    }
}


async getDataWithCustomNameEv(tableName, timeField, nameEvField, startTime, endTime, targetValues = [],abonentNumbersList = []) {
    //const client = await pool.connect();
    
    try {
        //let isPresentTargetValuesTable = await  this.getTargetPresenceTable(tableName, timeField, nameEvField, startTime, endTime, targetValues);
        //let targetValues_IsNotPresentInTimeRange = [];        
        //for(let item of isPresentTargetValuesTable) {
        //    if (item.is_present_in_range === false) {
        //      targetValues_IsNotPresentInTimeRange.push(item.target_name);
        //    }
        // }
      
        const targetPlaceholders = targetValues.map((_, index) => `$${index + 3}`).join(', ');
        const abonentsListPlaceholders = abonentNumbersList.map((_, index ) => `$${ index + targetValues.length + 3}`).join(', ');
        logger.info("\ntargetPlaceholders: %s\n",targetPlaceholders);
        logger.info("\nabonentsList: %s\n",abonentsListPlaceholders);
        //const targetValuesPlaceholders_IsNotPresentInTimeRange = targetValues_IsNotPresentInTimeRange.map((_, index) => `$${index + 3}`).join(', ');
                
        // Сначала выполним отдельный запрос для проверки
        const checkQuery = `
            SELECT EXISTS (
                SELECT 1 FROM ${tableName}
                WHERE ${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)
                AND ${nameEvField} IN (${targetPlaceholders})
            ) as has_target
        `;
        
        const checkParams = [startTime, endTime, ...targetValues];
        //const checkResult = await pool.query(checkQuery, checkParams);
        //const hasTarget = checkResult.rows[0].has_target;

        //let isExistTargetsOutTimeRange = false;
        //if (targetValues_IsNotPresentInTimeRange.length > 0) {
        //  isExistTargetsOutTimeRange = true;
        //}

        //logger.info('DEBUG: checkResult = %s', po(checkResult.rows));
        //logger.info('DEBUG: has_target = %s', hasTarget);
        //logger.info('DEBUG: NOT has_target = %s', !hasTarget);
        
        /*
                           --, 

        */
        // Теперь основной запрос        
        /* 
        const query = `
            SELECT *,
                   ${hasTarget} as debug_has_target,
                   ${!hasTarget} as debug_not_has_target

            FROM ${tableName} t
            WHERE 
                (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
                            AND (data_ev ->> 'evType')::text IN ${targetPlaceholders}
                            AND (data_ev ->> 'numAbonent')::numeric  IN ${abonentsList}
                 OR
                 (
                    ${isExistTargetsOutTimeRange}  -- подставляем результат напрямую
                    AND ${timeField} < TO_TIMESTAMP($1)
                    AND ${nameEvField} IN (${targetValuesPlaceholders_IsNotPresentInTimeRange})
                     AND ${timeField} = (
                        SELECT MAX(${timeField})
                        FROM ${tableName}
                        WHERE ${timeField} < TO_TIMESTAMP($1)
                        AND ${nameEvField} = t.${nameEvField}
                        AND ${nameEvField} IN (${targetValuesPlaceholders_IsNotPresentInTimeRange})
                     )
                 )
            ORDER BY id DESC, ${timeField} DESC
        `;
                -- AND ((data_ev ->> 'evType')::text IN ${targetPlaceholders})
                -- AND ((data_ev ->> 'numAbonent')::numeric  IN ${abonentsListPlaceholders})                                 
        */

                /*

                                 -- OR  
                 -- (                     
                 --   AND ${timeField} < TO_TIMESTAMP($1)
                 --   AND (data_ev ->> 'evType')::text IN (${targetValuesPlaceholders_IsNotPresentInTimeRange})
                 --   AND (data_ev ->> 'numAbonent')::text  IN (${abonentsListPlaceholders})
                 --   AND ${timeField} = (
                 --       SELECT MAX(${timeField})
                 --       FROM ${tableName}
                 --       WHERE ${timeField} < TO_TIMESTAMP($1)
                 --       AND (data_ev ->> 'evType')::text = t.(data_ev ->> 'evType')::text
                 --       AND (data_ev ->> 'evType')::text IN (${targetValuesPlaceholders_IsNotPresentInTimeRange})
                 --       AND (data_ev ->> 'numAbonent')::text  IN (${abonentsListPlaceholders})
                 --     )
                 -- )
                  Использование оператора ? (содержит ли массив элемент)
                  sql

                  SELECT *
                  FROM your_table
                  WHERE data_ev -> 'numAbonents' ? '12345';

                */
        const query = `
            SELECT *
            FROM ${tableName} t
            WHERE 
                (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
                AND ((data_ev ->> 'evType')::text IN (${targetPlaceholders}))
                AND (
                      ((data_ev ->> 'numAbonent')::text  IN (${abonentsListPlaceholders}))                 
                      OR
                      (NOT data_ev ? 'numAbonent')
                    )
                AND (
                      data_ev #> '{bgiSession, bgiList}' IS NULL
                      OR
                      data_ev @> '{"bgiSession": {"bgiList": [{"numAbonent": 1}]}}'
                    )                      
            ORDER BY id DESC, ${timeField} DESC
        `;

        // const params = [startTime, endTime, ...targetValues_IsNotPresentInTimeRange];
        const params = [startTime, endTime, ...targetValues, ...abonentNumbersList];
        const result = await pool.query(query, params);
        logger.info('eventsJournal: getDataWithCustomNameEv: %s ',  po(result.rows));
        return result.rows;
        
    } catch (error) {
        console.error('Error executing query:', error);
        throw error;
    } finally {
        //client.release();
    }
}

async getTargetPresenceTableHuman(tableName, timeField, nameEvField, startDateHuman, endDateHuman, targetValues = [],abonentNumbersList = []) {
  return await this.getTargetPresenceTable(tableName, timeField, nameEvField, 
    Math.floor(new Date(startDateHuman).getTime() / 1000), 
    Math.floor(new Date(endDateHuman).getTime() / 1000), 
    targetValues);
}

async getTargetPresenceTable(tableName, timeField, nameEvField, startTime, endTime, targetValues = [],abonentNumbersList = []) {
    //const client = await pool.connect();
    
    try {
        // Создаем массив значений для VALUES
        const valuesList = targetValues.map((v, i) => `($${i + 3})`).join(', ');
        const abonentsList = abonentNumbersList.map((v, i) => `($${i + 3})`).join(', ');
        
        const query = `
WITH target_presence AS (
    SELECT 
        target_values.target_name,
        EXISTS (
            SELECT 1 FROM ${tableName}
            WHERE ${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)
            AND (data_ev ->> 'evType')::text = target_values.target_name            
            AND (data_ev ->> 'numAbonent')::numeric  IN ${abonentsList}
        ) as is_present_in_range
    FROM (VALUES ${valuesList}) as target_values(target_name)
)
SELECT * FROM target_presence
ORDER BY target_name;
        `;
//--AND  data_ev.numAbonent IN (${abonentNumbersList}) 
        const params = [startTime, endTime, ...targetValues];
        
        const result = await pool.query(query, params);
        logger.info('eventsJournal: getTargetPresenceTable: %s ',  po(result.rows));
        return result.rows;
        
    } catch (error) {
        console.error('Error executing query:', error);
        throw error;
    } finally {
        //client.release();
    }
}

async getAbonentEventsTable(tableName, timeField, startTime, endTime, targetValues, abonentNumbers) {
    try {
        const targetPlaceholders = targetValues.map((_, i) => `$${i + 3}`).join(', ');
        const abonentsListPlaceholders = abonentNumbers.map((_, i) => `$${i + 3 + targetValues.length}`).join(', ');
        
        const query = `
WITH filtered_events AS (
    SELECT *
    FROM ${tableName} t
    WHERE 
        (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
        AND ((data_ev ->> 'evType')::text IN (${targetPlaceholders}))
        AND (
            ((data_ev ->> 'numAbonent')::text IN (${abonentsListPlaceholders}))                 
            OR
            (NOT data_ev ? 'numAbonent')
        )
        AND (
            data_ev #> '{bgiSession, bgiList}' IS NULL
            OR
            data_ev @> '{"bgiSession": {"bgiList": [{"numAbonent": 1}]}}'
        )
    ORDER BY ${timeField} ASC  -- Ранние события сначала
)
SELECT 
    COALESCE(data_ev ->> 'numAbonent', 'notAbonent') as numAbonent,
    jsonb_agg(
        jsonb_build_object(
            'id', id,
            'time', ${timeField},
            'name_ev', name_ev,
            'evType', data_ev ->> 'evType',
            'data_ev', data_ev            
        )
        ORDER BY ${timeField} ASC  -- Сохраняем порядок    
    ) as events_array
FROM filtered_events
GROUP BY COALESCE(data_ev ->> 'numAbonent', 'notAbonent')
ORDER BY numAbonent;
        `;

        const params = [startTime, endTime, ...targetValues, ...abonentNumbers];
        
        const result = await pool.query(query, params);
        logger.info('eventsJournal: getAbonentEventsTable:\n%s ',  po(result.rows));
        return result.rows;
        
    } catch (error) {
        console.error('Error executing query:', error);
        throw error;
    }
}


async getAbonentEventsWithBgiSession(tableName, timeField, startTime, endTime, targetValues, abonentNumbers) {
    try {
        const targetPlaceholders = targetValues.map((_, i) => `$${i + 3}`).join(', ');
        const abonentsListPlaceholders = abonentNumbers.map((_, i) => `$${i + 3 + targetValues.length}`).join(', ');
        
        const query = `
WITH filtered_events AS (
    SELECT *,
           COALESCE(
               data_ev ->> 'numAbonent',
               (SELECT elem ->> 'numAbonent' 
                FROM jsonb_array_elements(
                    COALESCE(data_ev #> '{bgiSession,bgiList}', '[]'::jsonb)
                ) as elem 
                WHERE elem ? 'numAbonent'
                LIMIT 1),
               'unknown'
           ) as extracted_numAbonent
    FROM ${tableName} t
    WHERE 
        (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
        AND ((data_ev ->> 'evType')::text IN (${targetPlaceholders}))
        AND (
            (COALESCE(
                data_ev ->> 'numAbonent',
                (SELECT elem ->> 'numAbonent' 
                 FROM jsonb_array_elements(
                     COALESCE(data_ev #> '{bgiSession,bgiList}', '[]'::jsonb)
                 ) as elem 
                 WHERE elem ? 'numAbonent'
                 LIMIT 1)
            ) IN (${abonentsListPlaceholders}))
            OR
            (data_ev ->> 'numAbonent' IS NULL 
             AND data_ev #> '{bgiSession,bgiList}' IS NULL)
        )
)
SELECT 
    extracted_numAbonent as numAbonent,
    jsonb_agg(
        jsonb_build_object(
            'id', id,
            'time', ${timeField},
            'name_ev', name_ev,
            'evType', data_ev ->> 'evType',
            'data_ev', data_ev
        )
        ORDER BY ${timeField} ASC
    ) as events_array
FROM filtered_events
GROUP BY extracted_numAbonent
ORDER BY extracted_numAbonent;
        `;

        const params = [startTime, endTime, ...targetValues, ...abonentNumbers];
        
        const result = await pool.query(query, params);
        logger.info('eventsJournal: getAbonentEventsTableWithBgiSession:\n%s ',  po(result.rows));
        return result.rows;
        
    } catch (error) {
        console.error('Error executing query:', error);
        throw error;
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
      "streamStart",            // Запуск воспроизведения звукозаписи
      "streamStop",            // Остановка воспроизведения звукозаписи
    ],
    "idSess":<integer>,        // Номер сеанса оповещения
    "allFields":<boolean>,     // Если поле задано и равно true то в ответ на запрос отдаются все поля
                                 записи базы данных    
  }
  или                                 
  {
    "requestType" : "monitoring",
    "startDate":<unix time>,  // Начальная дата из диапазона запроса , по времени
    "endDate":<unix time>,    // Конечная дата из диапазона запроса , по времени
    "withPreviousEvent":<boolean>  // Если поле задано и равно true то 
      // в ответ на запрос отдаются события предшествующие начальной дате диапазона. 
    "eventTypesList":[<string>],         // Типы событий которые нужно отдать
                                               "eventAlarmSession",
                                               "sipUnregistered",
                                               "smsGateway",   
                                               "voiceGateway",
                                               "sipChannelChanged",
                                               "sipRegistrationStatus"

    "abonentNumbersList":[<integer>],    // Номера абонентов события которых нужно отдать     
  }  
  Поля startDate и  endDate должны быть всегда, кроме запроса по idSess
  Поле evType может отсутствовать или быть пустым массивом
  Если поле idSess присутствует то поля  startDate  endDate не учавствуют в запросе

  Время startDateHuman  и endDateHuman должны быть в формате YYYY-MM-DDTHH:mm:ss.sssZ
    Формат — YYYY-MM-DDTHH:mm:ss.sssZ. В нём указаны часы, минуты, секунды и миллисекунды. g-blog.onrender.compurpleschool.ru
    Особенности:

    T — разделитель между датой и временем.
    HH:mm:ss.sss — часы, минуты, секунды и миллисекунды.
    Z — настройки временной зоны. Если Z присутствует, дата будет в формате UTC, если Z отсутствует — в локальном часовом поясе (это работает только если указано время).
 */
  async get(request) {
    let req = request;
    try {      
      let result;
      if(req.startDateHuman !== undefined) {        
        req.startDate = Math.floor(new Date(req.startDateHuman).getTime() / 1000);
      }     
      if(req.endDateHuman !== undefined) {        
        req.endDate = Math.floor(new Date(req.endDateHuman).getTime() / 1000);
      }     

      if(req.requestType !== undefined) {
        switch(req.requestType) {
          case "monitoring": {
             logger.info("get monitoring");
             result = await  this.getAbonentEventsTable("events_schema.events_table", "human_date_ev", 
                 req.startDate,
                 req.endDate,
                 req.eventTypesList,
                 req.abonentNumbersList               
             );
             logger.info('eventsJournal: getAbonentEventsTable:\n%s ',  po(result.rows));
            /*             
             result = await  this.getDataWithCustomNameEv("events_schema.events_table",
                 "human_date_ev",
                 "data_ev.evType",                 
                 req.startDate,
                 req.endDate,
                 req.eventTypesList,
                 req.abonentNumbersList               
              );
              */              
            break;
          }
          default: {
            throw new Error("Unknown requestType: " + req.requestType);
          } 
        }
        if(result === undefined) {
          return result.rows
        } else {
          return [];
        }        
      }
      let fields_part = "date_ev ,name_ev, description_ev,id_session_ev ,version_data_ev";
      let fields_full = "*";
      let fields = fields_part;      
      logger.info('eventsJournal: get: %s ',  po(req));
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
          } else {
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
      //logger.info('eventsJournal: get length: %s ', result.rows.length );
      logger.trace('eventsJournal: get: %s ',  po(result.rows));
      return  result.rows;
    } catch (err) {
      logger.error(err.stack,"eventsJournal: get");
      throw(err);
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
};


const evJrnl = new eventsJournal();
export default evJrnl;

//export let evJrnl = new eventsJournal(1);
