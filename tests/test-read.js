import {logger,po} from './logger.js';

(async () => {
  try {
    const evj  = await import('eventsJournal');
    let evjrnl = new evj.default;
    await evjrnl.run();
//    await evjrnl.insert("alarmStart", "Начало оповещения",1, {idSess:1})
//    await evjrnl.insert("alarmEnd", "Окончание оповещения",1, {idSess:1})
    {
      logger.info('REQUEST ---------------------- 1 ---------------------');
      let request ={startDate: 1731380232, endDate: 1731380232*2, evType:["alarmStart","alarmEnd","connected"]}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request);
      let  payload =  {err: "success", data: rc}
      logger.info('REQUEST result:: %s ', po(payload));

    }
    {
      logger.info('REQUEST ---------------------- 2 ---------------------');
      let request ={startDate: 1731380232, endDate: 1731380232*2, evType:[]}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request);
      let  payload =  {err: "success", data: rc}
      logger.info('REQUEST result:: %s ', po(payload));
    }
    {
      logger.info('REQUEST ---------------------- 3 ---------------------');
      let request ={idSess:1, evType:["alarmStart"]}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request);
      let  payload =  {err: "success", data: rc}
      logger.info('REQUEST result:: %s ', po(payload));
    }
    {
      logger.info('REQUEST ---------------------- 4 ---------------------');
      let request ={startDate: 1731380232, endDate: 1731380232*2}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request);
      let  payload =  {err: "success", data: rc}
      logger.info('REQUEST result:: %s ',po(payload));
    }
  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
