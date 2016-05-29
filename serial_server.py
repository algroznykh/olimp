# coding=utf-8
"""
Server for reading data from serial port
processing it and saving to database.
Intended for receiving temperature data
from DS18B20 sensor.

"""

import logging
import os
import sqlite3
from datetime import datetime

from serial import Serial

DB_NAME = 'temperature.db'
LOG_FILE = 'temperature.log'
delimiter = ','


class TempReceiver:

    def __init__(self, port, db_name):
        self.connection = sqlite3.connect(DB_NAME)
        self.cursor = self.connection.cursor()
        self.port = Serial(port)

    def listen(self):
        current_data = {}
        self.port.readline()  # to empty buffer
        while True:
            ser_data = self.port.readline()
            res = self.process_data(ser_data)
            if res is None:
                continue
            addr, raw, cal = res
            if current_data.get(addr, None):
                self.save_data(current_data)
                current_data = {addr: (raw, cal)}
            else:
                current_data[addr] = raw, cal

    def process_data(self, data):
        data = data.strip().split(delimiter)
        if len(data) != 2:
            return None
        chip_id = data[0]
        raw_temp = data[1]
        cal_temp = self.calibrate(raw_temp)
        return chip_id, raw_temp, cal_temp

    def calibrate(self, temp):
        return temp

    def save_data(self, data):
        """write one row of data to database"""
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        for addr, temp in data.iteritems():
            self.cursor.execute(
            'select * from "sqlite_master" '
            'where type="table" '
            'and name="{}"'.format(addr)
            )
            if not self.cursor.fetchall():
                logging.info('new sensor detected: {}'.
                            format(addr))
                self.cursor.execute(
                'create table if not exists "{}"'
                '(timestamp, raw_temp, cal_temp)'.
                format(addr)
                )
            self.cursor.execute(
            'insert into "{}" (timestamp, raw_temp, cal_temp) '
            'values ("{}", "{}", "{}")'.
            format(addr, timestamp, temp[0], temp[1])
            )
        self.connection.commit()

if __name__ == '__main__':
    # import ipdb; ipdb.set_trace()
    logging.basicConfig(filename=LOG_FILE, level=logging.INFO)
    serial_port = os.sys.argv[1]
    server = TempReceiver(serial_port, DB_NAME)
    logging.info('Starting server at port {}'.format(serial_port))
    server.listen()

