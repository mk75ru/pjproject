#!/bin/sh

pjsua  --cli-telnet-port=9090 --use-cli --realm=asterisk  --id=sip:101@127.0.0.1  --registrar=sip:127.0.0.1 --no-tcp --username=101 --password=101  --auto-answer=200 --ec-tail=30 --ec-opt=4
