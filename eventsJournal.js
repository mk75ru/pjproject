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
  async getDataWithCustomNameEv_(tableName, timeField, nameEvField, startTime, endTime, targetValues = []) {
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
  async getAbonentEventsTable_(tableName, timeField, startTime, endTime, targetValues, abonentNumbers) {
    try {

        const targetValuesDependent = targetValues?.filter(v => 
                 ['sipChannelChanged', 'sipRegistrationStatus','eventAlarmSessionBgi'].includes(v)
             ) || [];
             
        const targetValuesIndependent = targetValues?.filter(v => 
                 ['eventAlarmSession', 'smsGateway', 'voiceGateway'].includes(v)                   
             ) || [];
        
        logger.info("targetValues: %s", targetValues);
        logger.info("targetValuesDependent: %s", targetValuesDependent);     
        logger.info("targetValuesIndependent: %s", targetValuesIndependent);
        
        const targetPlaceholdersDependent   = targetValuesDependent.map((_, i) => 
          `$${i + 3}`).join(', ');
        const targetPlaceholdersIndependent = targetValuesIndependent.map((_, i) => 
          `$${i + 3 + targetValuesDependent.length}`).join(', ');
        const abonentsListPlaceholders = abonentNumbers.map((_, i) => 
          `$${i + 3 + targetValuesDependent.length + targetValuesIndependent.length}`).join(', ');
        
        const query = `
WITH filtered_events AS (
    SELECT *
    FROM ${tableName} t
    WHERE 
        (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
        AND ((data_ev ->> 'evType')::text IN (${targetPlaceholdersDependent} , ${targetPlaceholdersIndependent}))
        AND (
            ((data_ev ->> 'numAbonent')::text IN (${abonentsListPlaceholders}))                 
            OR
            (NOT data_ev ? 'numAbonent')
        )
    ORDER BY ${timeField} ASC  -- Ранние события сначала
),
-- Запрос для поиска отсутствующих событий
missing_events_report AS (
    WITH target_values AS (
        SELECT unnest($3::text[]) as target_name
    ),
    abonent_numbers AS (
        SELECT unnest($4::text[]) as num_abonent
    ),
    all_combinations AS (
        SELECT 
            an.num_abonent,
            tv.target_name
        FROM abonent_numbers an
        CROSS JOIN target_values tv
    ),
    present_events AS (
        SELECT 
            data_ev ->> 'numAbonent' as num_abonent,
            data_ev ->> 'evType' as ev_type,
            COUNT(*) as count
        FROM ${tableName}
        WHERE 
            ${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2)
            AND (data_ev ->> 'evType')::text IN (${targetPlaceholdersDependent}, ${targetPlaceholdersIndependent})
            AND (data_ev ->> 'numAbonent')::text IN (${abonentsListPlaceholders})
        GROUP BY data_ev ->> 'numAbonent', data_ev ->> 'evType'
    ),
    missing_events AS (
        SELECT 
            ac.num_abonent,
            ac.target_name as missing_event_type
        FROM all_combinations ac
        LEFT JOIN present_events pe 
            ON ac.num_abonent = pe.num_abonent 
            AND ac.target_name = pe.ev_type
        WHERE pe.count IS NULL
    ),
    grouped_missing AS (
        SELECT 
            num_abonent,
            jsonb_agg(missing_event_type) as missing_event_types
        FROM missing_events
        GROUP BY num_abonent
    )
    SELECT 
        an.num_abonent,
        COALESCE(gm.missing_event_types, '[]'::jsonb) as missing_events
    FROM abonent_numbers an
    LEFT JOIN grouped_missing gm ON an.num_abonent = gm.num_abonent
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

        const params = [startTime, endTime, ...targetValuesDependent, ...targetValuesIndependent, ...abonentNumbers];
        
        const result = await pool.query(query, params);
        logger.info('eventsJournal: getAbonentEventsTable:\n%s ',  po(result.rows));
        return result;
        
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
      "alarmStartBgi",        // Запуск оповещения БГИ
      "alarmEndBgi",          // Завершение оповещения БГИ
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
    "eventTypesListDepent":[<string>],         // Типы событий которые нужно отдать
                                               "eventAlarmSession",
                                               "eventAlarmSessionBgi",
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
  async get_(request) {
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
        if(result !== undefined) {          
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
//------------------------------------------------------------------------------------


async getPreviousEvents(tableName, timeField, startTime, missingTargetValues) {
    try {
        logger.info('Getting previous events for missingTargetValues: %s', 
                   po(missingTargetValues));
        
        if (missingTargetValues.length === 0 ) {
            logger.info('No missing target values provided, returning empty array');
            return [];
        }
        //const missingTargetValuesPlaceholders = missingTargetValues.map((_, i) => `$${i + 3}`).join(', ');
        const missingTargetValuesJstring = JSON.stringify(missingTargetValues);

const query = `
WITH ranked_events AS (
    SELECT 
        *,
        data_ev ->> 'numAbonent' as num_abonent,
        data_ev ->> 'evType' as ev_type,
        ROW_NUMBER() OVER (
            PARTITION BY 
                COALESCE(data_ev->>'numAbonent', 'notAbonent'), 
                data_ev->>'evType'
            ORDER BY t.${timeField} DESC
        ) as rn
    FROM ${tableName} t,
    jsonb_each_text($2::jsonb) AS obj(key, value)
    WHERE 
        t.${timeField} < TO_TIMESTAMP($1)
        AND (                  
          (
            t.data_ev ? 'numAbonent' AND
            obj.key = t.data_ev->>'numAbonent'
          )
          OR
          (
            NOT (t.data_ev ? 'numAbonent') AND
            obj.key = 'notAbonent'
          )
        )
        AND (obj.value::jsonb @> to_jsonb(t.data_ev ->> 'evType'))
)
SELECT 
    COALESCE(num_abonent, 'notAbonent') as numAbonent,
    jsonb_agg(
        jsonb_build_object(
            'id', id,
            'time', ${timeField},
            'name_ev', name_ev,
            'evType', ev_type,
            'data_ev', data_ev,
            'is_previous', true
        )
        ORDER BY ${timeField} ASC
    ) as events_array
FROM ranked_events
WHERE rn = 1
GROUP BY COALESCE(num_abonent, 'notAbonent')
ORDER BY numAbonent;
`;


        const params = [startTime,missingTargetValuesJstring];
        
        logger.info('Previous events query: %s', query);
        logger.info('Previous events params: %s',po( params));
        
        const result = await pool.query(query, params);
        logger.info('Found %s previous events', result.rows.length);
        logger.info('!!!!!!!!!!!!!! previous events %s', po(result.rows));
        return result;
        /*
        return result.rows.map(row => ({
            num_abonent: row.num_abonent,
            ev_type: row.ev_type,
            event: row.previous_event
        }));
        */
        
    } catch (error) {
        console.error('Error getting previous events:', error);
        logger.error('Error in getPreviousEvents: %s', error.message);
        return [];
    }
}


async getAbonentEventsTable(tableName, timeField, startTime, endTime, targetValues, abonentNumbers) {
    try {
        const targetValuesDependent = targetValues?.filter(v => 
                 ['sipChannelChanged', 'sipRegistrationStatus','eventAlarmSessionBgi'].includes(v)
             ) || [];
             
        const targetValuesIndependent = targetValues?.filter(v => 
                 ['eventAlarmSession', 'smsGateway', 'voiceGateway'].includes(v)                   
             ) || [];
        
        logger.info("targetValues: %s", targetValues);
        logger.info("targetValuesDependent: %s", targetValuesDependent);     
        logger.info("targetValuesIndependent: %s", targetValuesIndependent);
        
        // Преобразуем номера абонентов в строки для сравнения с текстовым полем
        const abonentNumbersAsStrings = abonentNumbers.map(num => num.toString());
        
        const targetPlaceholdersDependent = targetValuesDependent.map((_, i) => `$${i + 3}`).join(', ');
        const targetPlaceholdersIndependent = targetValuesIndependent.map((_, i) => `$${i + 3 + targetValuesDependent.length}`).join(', ');
        const abonentsListPlaceholders = abonentNumbersAsStrings.map((_, i) => `$${i + 3 + targetValuesDependent.length + targetValuesIndependent.length}`).join(', ');
        
        // Упрощенный запрос - сначала получаем события в диапазоне
        const queryInRange = `
WITH events_in_range AS (
    SELECT 
        *,
        data_ev ->> 'numAbonent' as num_abonent,
        data_ev ->> 'evType' as ev_type
    FROM ${tableName} t
    WHERE 
        (${timeField} BETWEEN TO_TIMESTAMP($1) AND TO_TIMESTAMP($2))
        AND ((data_ev ->> 'evType')::text IN (${targetPlaceholdersDependent}, ${targetPlaceholdersIndependent}))
        AND (
            ((data_ev ->> 'numAbonent')::text IN (${abonentsListPlaceholders}))                 
            OR
            (NOT data_ev ? 'numAbonent')
        )
)
SELECT 
    COALESCE(num_abonent, 'notAbonent') as numAbonent,
    jsonb_agg(
        jsonb_build_object(
            'id', id,
            'time', ${timeField},
            'name_ev', name_ev,
            'evType', ev_type,
            'data_ev', data_ev,
            'is_previous', false
        )
        ORDER BY ${timeField} ASC
    ) as events_array
FROM events_in_range
GROUP BY COALESCE(num_abonent, 'notAbonent')
ORDER BY numAbonent;
        `;

        const params = [
            startTime, 
            endTime, 
            ...targetValuesDependent, 
            ...targetValuesIndependent, 
            ...abonentNumbersAsStrings
        ];
        
        logger.info('Executing getAbonentEventsTable with params: %s', params);
        const rangeEvents = await pool.query(queryInRange, params);
        logger.info('eventsJournal: getAbonentEventsTable result: %s rows', rangeEvents.rows.length);        
        // Проверка на пустые массивы
        if (targetValuesDependent.concat(targetValuesIndependent).length === 0 || abonentNumbersAsStrings.length === 0) {
            logger.info('No target values or abonent numbers, skipping previous events search');
            return { rows: rangeEvents.rows.map(row => ({
                numAbonent: row.numabonent,
                events: row.events_array,
                previousEvents: [],
                hasPreviousEvents: false
            })) };
        }
        logger.info('eventsJournal: getAbonentEventsTable result: %s ', po(rangeEvents.rows));
        let isExistAbonents = {}; 
        for(let abonentNumber of abonentNumbersAsStrings) {
            isExistAbonents[abonentNumber] = false;
        }
        isExistAbonents['notAbonent'] = false;
        let missingTargetValues = {};
        for (const row of rangeEvents.rows) {
            //logger.info('eventsJournal: getAbonentEventsTable row: %s', po(row));  
            // Если абонент есть в списке абонентов, то забираем его список событий
            const targetValuesDependentForAbonent = targetValuesDependent.filter(v => abonentNumbersAsStrings.includes(row.numabonent));
            // Если абонент не абонент, то забираем его список событий независимых от номера
            const targetValuesIndependentForAbonent = targetValuesIndependent.filter(v => row.numabonent === 'notAbonent');
            if(targetValuesDependentForAbonent.length === 0 && targetValuesIndependentForAbonent.length === 0) {
              continue;
            }
            isExistAbonents[row.numabonent] = true;
            // Находим события, которых нет в списке
            //let missingTargetValuesForOneAbonent = targetValuesDependentForAbonent.concat(targetValuesIndependentForAbonent).filter(v => !row.events_array.map(e => e.evType).includes(v));
            let missingTargetValuesForOneAbonent = targetValuesDependentForAbonent.concat(targetValuesIndependentForAbonent);
            missingTargetValues[row.numabonent] = missingTargetValuesForOneAbonent;            
            
        }
        for(let abonentNumber in isExistAbonents) {
            if (isExistAbonents[abonentNumber] == false) {
                if(abonentNumber === 'notAbonent') {
                  missingTargetValues[abonentNumber] = targetValuesIndependent;  
                }
                else {
                  missingTargetValues[abonentNumber] = targetValuesDependent;
                }                
            }
        }

        logger.info('eventsJournal: getAbonentEventsTable missingTargetValues: %s', po(missingTargetValues));
        
        // Теперь отдельно найдем предыдущие события для каждого абонента и типа
        const previousEvents = await this.getPreviousEvents(
            tableName, 
            timeField, 
            startTime, 
            missingTargetValues 
        );
        

        let rangeEventsMap = {};
        for( let abonent  of rangeEvents.rows) {
          rangeEventsMap[abonent.numabonent] = abonent;          
        }
        let previousEventsMap = {};
        for( let abonent  of previousEvents.rows) {          
          previousEventsMap[abonent.numabonent] = abonent;
        }
        let abonentNumbersFull = abonentNumbersAsStrings.concat(['notAbonent']);
        for( let abonent  of abonentNumbersFull) {
          if((previousEventsMap[abonent]) && (!rangeEventsMap[abonent]) ) {                        
            rangeEventsMap[abonent] = previousEventsMap[abonent];
          }
          else if((previousEventsMap[abonent]) && (rangeEventsMap[abonent]) ) {            
            rangeEventsMap[abonent].events_array = previousEventsMap[abonent].events_array.concat(rangeEventsMap[abonent].events_array);                       
          }
        }

        return rangeEventsMap;
       
        
    } catch (error) {
        console.error('Error executing query:', error);
        logger.error('Error in getAbonentEventsTable: %s', error.message);
        throw error;
    }
}




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
            return result;
          }
          default: {
            throw new Error("Unknown requestType: " + req.requestType);
          } 
        }
      }

      // ... остальной код метода get 
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




//------------------------------------------------------------------------------------
};


const evJrnl = new eventsJournal();
export default evJrnl;

//export let evJrnl = new eventsJournal(1);
