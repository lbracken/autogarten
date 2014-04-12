# -*- coding: utf-8 -*-
"""
    autogarten.control_server
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Provides Flask based template rendering and web service support.

    :license: MIT, see LICENSE for more details.
"""

import argparse
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

time_diff_threshold = 5  # 5 seconds

@app.route("/")
def main_page():
    return render_template("index.html")


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

    # TODO: Validate probe sync request token

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
    args  = parse_args()

    print "----------------------------------< autogarten Control Server >----"
    app.run(debug=True) # If running directly from the CLI, run in debug mode.