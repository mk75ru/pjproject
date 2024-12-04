import {logger,po} from './logger.js';

(async () => {
  try {
    const evj  = await import('eventsJournal');
    let evjrnl = new evj.default;
    await evjrnl.run();
    {
      logger.info('REQUEST ---------------------- 1 ---------------------');
      let request ={startDate: 1731380232, endDate: 1731380232*2, evType:["alarmStart","alarmEnd","connected"]}
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request);
      let  payload =  {err: "success", data: rc}
      logger.info('REQUEST result:: %s ', po(payload));

    }
  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
