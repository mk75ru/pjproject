import {logger,po} from './logger.js';

(async () => {
  try {
    const evj  = await import('eventsJournal');
    let evjrnl = new evj.default;
    await evjrnl.run();
    await evjrnl.insert("alarmStart", "Начало оповещения",1, {idSess:1})
    let  id = await evjrnl.insert("alarmEnd", "Окончание оповещения",1, {idSess:1})
    await evjrnl.add({speakers:[{name:"speaker1",number:12},{name:"speaker2",number:13}] },id);
    await evjrnl.getAll();
  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
