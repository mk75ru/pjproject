import {logger,po} from './logger.js';

function delay(ms) {
  return new Promise((resolve, reject) => {
    setTimeout(resolve, ms);
  });
}


(async () => {
  try {
    const evj  = await import('eventsJournal');
    let evjrnl = new evj.default;
    await evjrnl.run();
    let cnt = +10;
    while(cnt > 0) {
      logger.info("------------->%s",cnt);
      cnt--;
      await evjrnl.insert("alarmStart", "Начало оповещения",1, {idSess:1})
      await delay(500)
      await evjrnl.insert("alarmEnd", "Окончание оповещения",1, {idSess:1})
      await delay(500)
    }

    let rc =  await  evjrnl.getAll();


  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
