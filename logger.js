// logger.js
import { fileURLToPath } from 'url';
import { dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// import * as pino from 'pino';
import pino from 'pino'
import pinoPretty from 'pino-pretty'
import 'dotenv/config'

const pathToLog =  process.env.PATH_TO_AUTOCALL_LOG || "automated-calling-system.log";


const transport = pino.transport({
  targets: [
    {
      level: 'trace',
      target: 'pino/file',
      options:  { destination: pathToLog },
//      options: { destination: `${__dirname}/app.log` },
    },
    {
      level: 'trace',
      target: 'pino-pretty', // по-умолчанию логирует в стандартный вывод  pino/file
    },
  ],
});

export const logger  =  pino(
 {
  level: process.env.PINO_LOG_LEVEL || 'info',
  timestamp: pino.stdTimeFunctions.isoTime,
 },
 transport
);


export function po(o) { //pretty object
  return JSON.stringify(o, null, 2);
}
