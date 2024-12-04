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
    let rc =  await  evjrnl.getAll();


  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
