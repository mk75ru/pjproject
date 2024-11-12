import {logger,po} from './logger.js';

(async () => {
  try {
    const evj  = await import('eventsJournal');
    let evjrnl = new evj.default;
    await evjrnl.run();
    await evjrnl.insert_raw(0,"alarmStart", "Начало оповещения",1, {})
    await evjrnl.insert_raw(10000,"alarmEnd", "Окончание оповещения",1, {})
    await evjrnl.insert_raw(100000,"alarmStart", "Начало оповещения",1, {})
    await evjrnl.insert_raw(1000000,"alarmEnd", "Окончание оповещения",1, {})
    {
      logger.info('REQUEST ---------------------- 1 ---------------------');
      let request ={startDate: 0, endDate: 100000, evType:["alarmStart","alarmEnd"]}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request_j);
    }
    {
      logger.info('REQUEST ---------------------- 2 ---------------------');
      let request ={startDate: 0, endDate: 100000, evType:["alarmStart"]}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request_j);
    }
  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
