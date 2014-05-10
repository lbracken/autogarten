# -*- coding: utf-8 -*-
"""
    autogarten.control_server
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Provides Flask based template rendering and web service support.

    :license: MIT, see LICENSE for more details.
"""

import argparse
import ConfigParser
import json
import math
import traceback

from datetime import datetime

from flask import abort
from flask import Flask
from flask import jsonify
from flask import make_response
from flask import render_template
from flask import request

from service import probe_service
from probe_sync import ProbeSync
import date_util

app = Flask(__name__)
verbose = False

token = None
time_diff_threshold = 5  # Value read from settings, but default of 5s.

def init_config():
    """ Read settings from config file

    """
    global token, time_diff_threshold

    config = ConfigParser.SafeConfigParser()
    config.read("settings.cfg")

    token = config.get("control_server", "token")
    time_diff_threshold = config.getint("control_server", "time_diff_threshold")

    # Setup Jinja Filters
    app.jinja_env.filters['format_number'] = format_number
    app.jinja_env.filters['format_date'] = format_date


@app.route("/")
def main_page():
    probe_overview = probe_service.get_probe_overview()
    return render_template("index.html", probe_overview=probe_overview)


@app.route("/probe_sync", methods=['POST'])
def probe_sync():

    response = {}

    # Read and validate probe sync request.  Assumes request data is of
    # content-type 'application/json'
    try:
        probe_sync = ProbeSync(request.json)
        if not probe_sync.is_valid():
            abort(400)

    except Exception,e :
        # If there are problems reading the request arguments, then
        # the request is bad.  Return a 400 HTTP Status Code - Bad
        # Request
        if verbose:
            print "  %s" % str(e)
            traceback.print_exc() 
        abort(400)

    if verbose:
        print ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> probe_sync"
        print request 

    # Validate probe sync request token
    if not probe_sync.token == token:
        print "[WARN] Connection attempt by probe '%s' from %s with invalid token." %\
            (probe_sync.probe_id, request.remote_addr)
        abort(401)

    # TODO: Compare probe's time with Control Server time.  Then
    # log if the difference is significant then log.
    time_diff = date_util.get_current_timestamp() - probe_sync.curr_time
    if math.fabs(time_diff) > time_diff_threshold:
        print "[WARN] Probe time is off by %ds" % time_diff

    # If the probe tried to connect to the Control Server more than
    # once, then log the the failed connection attemps
    if probe_sync.connection_attempts > 1:
        print "[WARN] Probe '%s' attemped to connect %d times" %\
            (probe_sync.probe_id, probe_sync.connection_attempts)

    response = probe_service.process_probe_sync(probe_sync)

    if verbose:
        print response
        print "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"

    return make_response(jsonify(response))


def format_number(value):
    """ Used as custom Jinja Filter to format numbers

    """
    return "%0.2f" % float(value)


def format_date(value):
    """ Used as custom Jinja Filter to format dates

    """
    today = datetime.today()

    # If the date is from today...
    if date_util.is_same_day(value, today):
        return "Today at " + value.strftime('%H:%M:%S')

    # If the date is from this year...
    if date_util.is_same_year(value, today):
        return value.strftime('%b %d at %H:%M')        

    return value.strftime('%b %d, %Y')


def parse_args():
    """ Parse the command line arguments

    """
    global verbose

    parser = argparse.ArgumentParser(description="autogarten Control Server")
    parser.add_argument("-v", "--verbose", action="store_true",
            help="Make the operation talkative")
    args = parser.parse_args()   
    
    verbose = args.verbose
    return args


if __name__ == "__main__":
    args = parse_args()
    init_config()

    print "----------------------------------< autogarten Control Server >----"
    app.run(debug=True) # If running directly from the CLI, run in debug mode.