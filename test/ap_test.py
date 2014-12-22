# -*- coding: utf-8 -*-
"""
    autogarten.test.ap_test
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    Test module that simulates a probe to validate Control Server.

    This test probe has the following sensors...
      * tmp0 - A mock temperature sensor that always returns 70.0' 
      * tmp1 - A mock temperature sensor that rises and falls over 1hr
      * pho0 - A mock light sensor that rises and falls over 24hrs
      * mos0 - A mock mositure sensor returns random values

    :license: MIT, see LICENSE for more details.
"""

import argparse
import json
import random
import urllib2

from apscheduler.scheduler import Scheduler
from datetime import datetime
from time import sleep

import date_util

sched = Scheduler()
sensor_data = []



def create_schedule():
    global sched

    i = 15
    next_sensor_read = get_next_timer_interval(i)
    exec_data = datetime.fromtimestamp(next_sensor_read)

    sched.add_interval_job(
        read_sensor,
        seconds=i,
        start_date=str(exec_data),
        args=["s0"])

    sched.add_interval_job(
        read_sensor,
        seconds=i,
        start_date=str(exec_data),
        args=["s1"]) 

    sched.add_interval_job(
        read_sensor,
        seconds=i,
        start_date=str(exec_data),
        args=["s2"])                    

    sched.print_jobs()
    print "..."

def get_next_timer_interval(interval):
    """ Given an interval (in seconds), determine the starting point of
        the next period of this interval.  All intervals are based upon
        time since midnight of the current day.

        For example, if the current day was Jan 1st, 1970, with an
        interval of 3600 (1hr), and this method was called at 2:30a...

        Timestamp  Time
                0  12:00a
             3600  01:00a
             7200  02:00a
             9000  02:30a  <--- If calling here...
            10800  03:00a  <--- Timestamp for 3a will be returned

    """
    now_datetime = datetime.now()
    now = date_util.get_timestamp(now_datetime)
    midnight = date_util.get_timestamp(date_util.get_midnight(now_datetime))
    return midnight + ((now - midnight)/interval) * interval + interval  



def read_sensor(sensor_id):


    print "Reading Sensor: %s  %d" % (sensor_id, len(sensor_data))
    sensor_data.append(0)





if __name__ == "__main__":

    # Start the scheduler
    sched.start()
    create_schedule()

    while True:
        sleep(1)