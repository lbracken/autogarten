# -*- coding: utf-8 -*-
"""
    autogarten.test.test_probe
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    Test module that simulates a probe to validate Control Server.
    See read_all_sensors() for list of sensors simulated by this probe.

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

sensor_freq = 15    # Read sensors every 15s
sync_freq = 31      # Sync with control server every 60s


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
        "sensor_freq" : sensor_freq,
        "sync_freq" : sync_freq,
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


def get_seconds_since_midnight():
    now = datetime.now()
    midnight = now.replace(hour=0, minute=0, second=0, microsecond=0)
    return int((now - midnight).total_seconds())


def get_seconds_since_hour():
    now = datetime.now()
    hour = now.replace(minute=0, second=0, microsecond=0)
    return int((now - hour).total_seconds())    


def read_all_sensors():

    if verbose:
        print ""
        print "Reading Sensors..."

    # tmp0 - A mock temperature sensor that always returns 70.0'
    record_sensor_data("tmp0", 70.0)

    # tmp1 - A mock temperature sensor that rises and falls over 1hr
    record_sensor_data("tmp1", get_seconds_since_hour() / 100)

    # pho0 - A mock light sensor that rises and falls over 24hrs
    record_sensor_data("pho0", get_seconds_since_midnight() / 100)

    # mos0 - A mock mositure sensor returns random values
    record_sensor_data("mos0", random.randrange(0, 100))


def record_sensor_data(sensor_id, value):

    data_point = {
        "id" : sensor_id,
        "timestamp" : date_util.get_current_timestamp(),
        "value" : value
    }

    if verbose:
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

    # Schedule probe syncs with control server and sensor reads
    sched.add_interval_job(send_probe_sync_request, seconds=sync_freq)
    sched.add_interval_job(read_all_sensors, seconds=sensor_freq)
    sched.print_jobs()

    while True:
        sleep(1)