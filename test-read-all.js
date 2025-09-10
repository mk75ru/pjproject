import {logger,po} from './logger.js';







  
/*
"eventAlarmSession"       "alarmStart" "alarmEnd"
"smsGateway"              "connected" "disconnected"
"voiceGateway"            "connected" "disconnected"
"streaming"               "streamStart" "streamStop"
"sipChannelChanged"       "sipChannelUp" "sipChannelDestroy"
"sipRegistrationStatus"   "sipRegistered" "sipUnregistered"
*/


(async () => {
  try {

    let evj = await import('eventsJournal');
    let evjrnl =  evj.default;
    await evjrnl.run();
    {
      logger.info('REQUEST ---------------------- 1 ---------------------');
      //let request ={startDate: 1731380232, endDate: 1731380232*2, evType:["alarmStart","alarmEnd","connected"]}
      let request ={
                    requestType: "monitoring",
                    //startDateHuman: "2025-09-08T14:20:33.000",
                    //startDateHuman: "2025-09-08T14:20:40.000",
                    //startDateHuman: "2025-09-08T14:22:02.000",
                    //startDateHuman: "2025-09-08T14:22:05.000",
                    //startDateHuman: "2025-09-08T17:46:53.000",
                    //endDateHuman: "2025-09-08T17:49:02.000",
                    startDateHuman: "2025-09-09T18:00:00.000",
                    endDateHuman: "2025-09-10T18:59:02.000",
                    eventTypesList:["eventAlarmSession","eventAlarmSessionBgi", "smsGateway", "voiceGateway", "streaming", "sipChannelChanged", "sipRegistrationStatus" ],
                    abonentNumbersList:[9]             
                  }
      let request_j = JSON.stringify(request)
      let rc =  await  evjrnl.get(request);
      let  payload =  {err: "success", data: rc}

/*
    const presenceTable = await evjrnl.getTargetPresenceTableHuman(
      "events_schema.events_table",
      "human_date_ev",
      "name_ev",
      "2025-09-08T17:46:53.000",  // startTime
      "2025-09-08T17:49:02.000",  // endTime
      ["alarmStart", "alarmEnd", "connected", "disconnected","sipRegistered","sipUnregistered"]
    );
    
    logger.info('REQUEST result:: %s ', po(payload));
*/
    }
  }
  catch(e) {
    logger.error(e,'REQUEST result:: %s ');
  }
}
)();
