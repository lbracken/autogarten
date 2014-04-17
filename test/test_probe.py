# -*- coding: utf-8 -*-
"""
    autogarten.test.test_probe
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
import logging
import random
import urllib2

from apscheduler.scheduler import Scheduler
from datetime import datetime
from time import sleep

import date_util

verbose = False
control_server_hostname = None
control_server_port = None
token = None

sched = Scheduler()
sensor_data = []

sync_count = 0

probe_id = "test_probe"


def send_probe_sync_request():
    global sync_count

    url = "http://%s:%d/probe_sync" %\
        (control_server_hostname, control_server_port)
    print " **********************************************************"
    print " > Sending probe sync request to: %s with token [%s]" % (url, token)

    request_content = {
        "probe_id" : probe_id,
        "token" : token,
        "connection_attempts" : 1,
        "sync_count" : sync_count,
        "curr_time" : date_util.get_current_timestamp(),
        "sensor_data" : sensor_data
    }

    # Print Request
    print "---- Request ----"
    print json.dumps(request_content, indent=1)

    request = urllib2.Request(url)
    request.add_header('Content-Type', 'application/json')

    response = urllib2.urlopen(request, json.dumps(request_content))
    response_content_str = response.read()
    response_content = json.loads(response_content_str)

    # Clear sensor data
    del sensor_data[:]
    sync_count += 1

    # Print Response
    print ""
    print "---- Response ----"
    print "%s" % response_content_str
    print ""

    # Schedule a timer for the next probe sync
    next_probe_sync = get_next_timer_interval(response_content["interval"])
    exec_data = datetime.fromtimestamp(next_probe_sync)
    sched.add_date_job(send_probe_sync_request, exec_data, [])

    # Unschedule any sensor jobs (intervals)
    if sync_count > 1:
        sched.unschedule_func(read_sensor)

    # Schedule timers for all sensor commands.
    # This Python implementtion is differnet from Arduion (and more clumsy)
    for sensor_command in response_content["sensor_commands"]:
        next_sensor_read = get_next_timer_interval(sensor_command["interval"])
        exec_data = datetime.fromtimestamp(next_sensor_read)

        sched.add_interval_job(
            read_sensor,
            seconds=sensor_command["interval"],
            start_date=str(exec_data),
            args=[sensor_command["id"]])

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


def get_seconds_since_midnight():
    now = datetime.now()
    midnight = now.replace(hour=0, minute=0, second=0, microsecond=0)
    return int((now - midnight).total_seconds())


def get_seconds_since_hour():
    now = datetime.now()
    hour = now.replace(minute=0, second=0, microsecond=0)
    return int((now - hour).total_seconds())    


def read_sensor(sensor_id):

    data_point = {
        "id" : sensor_id,
        "timestamp" : date_util.get_current_timestamp()
    }

    # See module header for details on data point values
    if sensor_id == "tmp0":
        data_point["data"] = 70.0

    elif sensor_id == "tmp1":
        data_point["data"] = get_seconds_since_hour() / 100

    elif sensor_id == "pho0":
        data_point["data"] = get_seconds_since_midnight() / 100

    elif sensor_id == "mos0":
        data_point["data"] = random.randrange(0, 100)

    print json.dumps(data_point, indent=1)
    sensor_data.append(data_point)


def parse_args():
    """ Parse the command line arguments

    """
    global verbose, control_server_hostname, control_server_port, token

    parser = argparse.ArgumentParser(description="autogarten Test Probe")
    parser.add_argument("-v", "--verbose", action="store_true",
            help="Make the operation talkative")
    parser.add_argument("-s", "--host", default="localhost",
            help="Control Server Host")    
    parser.add_argument("-p", "--port", type=int, default=5000,
            help="Control Server Port")
    parser.add_argument("-t", "--token", default="changeme",
            help="Control Server Auth Token")
    args = parser.parse_args()   
    
    verbose = args.verbose
    control_server_hostname = args.host
    control_server_port = args.port
    token = args.token
    return args


if __name__ == "__main__":
    args  = parse_args()
    logging.basicConfig()

    # Start the scheduler
    sched.start()

    print "--------------------------------------< autogarten Test Probe >----"
    send_probe_sync_request()

    while True:
        sleep(1)