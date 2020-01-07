#!/usr/bin/python
import requests

r = requests.post("https://admin:admin@iot-telit-distributor-proxy-route-dev.app.sbb.ch/api/monitoring/clients/raspi", data='body')
print('called ' + r.url)

