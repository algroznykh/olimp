from datetime import datetime
import requests
import json
import random
import argparse
import time


def sensor_emulator(host, port, freq=1):
   """sends a json with emulated temperature data 
      to the given address""" 
   address = 'http://{}:{}'.format(host, port)
   print('starting transmit data to {}'.format(address))
   while True:
       res = { 
           "sensorId": "sensor_emulator",
           "timestamp": datetime.utcnow().strftime('%s'),
           "value": random.choice(range(100))
           }   
       try:
           requests.post(address, json=res)
       except Exception as e:
           print(e)
       time.sleep(freq)


if __name__ == '__main__':
   parser = argparse.ArgumentParser()
   parser.add_argument(type=str, dest='HOST')
   parser.add_argument(type=int, dest='PORT')
   args = parser.parse_args()

   sensor_emulator(args.HOST, args.PORT)
   
