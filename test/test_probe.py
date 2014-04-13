# -*- coding: utf-8 -*-
"""
    autogarten.test.test_probe
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    Test module that simulates a probe to validate Control Server.

    TODO: This test probe has the following sensors...
      * tmp0 - A mock temperature sensor that always returns 70.0' 
      * tmp1 - A mock temperature sensor that rises and falls over 1hr
      * pho0 - A mock light sensor that rises and falls over 24hrs
      * mos0 - A mock mositure sensor returns random values

    :license: MIT, see LICENSE for more details.
"""

import argparse
import json
import urllib2

from datetime import datetime
import date_util

verbose = False
control_server_hostname = None
control_server_port = None
token = None

probe_id = "test_probe"


def send_probe_sync_request():

    url = "http://%s:%d/probe_sync" %\
        (control_server_hostname, control_server_port)
    print " > Sending probe sync request to: %s with token [%s]" % (url, token)


    # !! TEMP ONLY >>>>>>>>>>>>>>>>
    current_timestamp = date_util.get_current_timestamp()
    sensor_data = [
        {"id":"tmp0", "timestamp" : current_timestamp, "data": 70.0},
        {"id":"tmp1", "timestamp" : current_timestamp, "data": 56.78},
        {"id":"pho0", "timestamp" : current_timestamp, "data": 0.95},
        {"id":"mos0", "timestamp" : current_timestamp, "data": 1},
    ]
    # <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    request_content = {
        "probe_id" : probe_id,
        "token" : token,
        "connection_attempts" : 1,
        "curr_time" : date_util.get_current_timestamp(),
        "sensor_data" : sensor_data
    }

    request = urllib2.Request(url)
    request.add_header('Content-Type', 'application/json')

    response = urllib2.urlopen(request, json.dumps(request_content))
    print "%s" % response.read()


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

    print "--------------------------------------< autogarten Test Probe >----"
    send_probe_sync_request()