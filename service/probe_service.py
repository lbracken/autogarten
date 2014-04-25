# -*- coding: utf-8 -*-
"""
    autogarten.service.probe_service
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Module that provides probe related services.

    :license: MIT, see LICENSE for more details.
"""

import json
from datetime import datetime
from datetime import date

from db import mongo
from probe_sync import ProbeSync

import date_util

db_probe_status = None
db_sensor_data = None

probe_sync_interval = 1 * 60 * 60 # Probe sync interval in seconds

def init_db():
    global db_probe_status, db_sensor_data

    # Init DB
    db_probe_status = mongo.get_probe_status_collection()
    db_sensor_data = mongo.get_sensor_data_collection()


def get_probe_status():
    """ Returns status information on all probes in the system

    """
    db_query = {}
    probe_status = db_probe_status.find(db_query)
    return probe_status


def process_probe_sync(probe_sync):
    """ Processes a probe sync request.

    """

    probe_id = probe_sync.probe_id

    # Persist information about the probe and this sync
    update_probe_status(probe_id, probe_sync.sync_count)

    # Persist any sensor data
    persist_sensor_data(probe_id, probe_sync.sensor_data)

    # Persist any actuator history
    # TODO...

    # Determine commands for response
    sensor_commands = determine_sensor_commands(probe_id)

    # Build response
    response = {
        "probe_id" : probe_sync.probe_id,
        "interval" : probe_sync_interval,
        "sensor_commands" : sensor_commands
    }

    # Finally provide time sync data.  This will be off by remaining
    # control server processing, one way latency and probe processing.
    response["curr_time"] = date_util.get_current_timestamp()
    return response


def update_probe_status(probe_id, sync_count):
    """ Persist information about this probe and its sync

    """

    now = datetime.now()
    update_set = {
        "probe_id" : probe_id,
        "last_contact" : now,
    }

    if sync_count < 1:
        update_set["active_since"] = now

    db_probe_status.update(
        {"_id" : probe_id}, {
            "$set" : update_set,
            "$inc" : {"sync_count" : 1},
            "$setOnInsert" : {"first_contact" : now}
        }, True)  # True for upsert


def persist_sensor_data(probe_id, sensor_data):
    """ Perists the given sensor data in the DB.  Currently using a 
        Document-Oriented schema design. For more details see: 
          * http://blog.mongodb.org/post/65517193370/
              schema-design-for-time-series-data-in-mongodb

        TODO: If a single probe's sensor data has many data points to
        record at once, that could result in mongoDB having to
        reallocate/grow the document multiple times.  A future
        optimization would be to organize and write all values at once.

    """
    for data_point in sensor_data:

        sensor_id = data_point["id"]
        timestamp = data_point["timestamp"]
        date_time = datetime.fromtimestamp(timestamp)
        day = date_util.get_midnight(date_time)

        db_sensor_data.update(
            {"_id" : get_metric_id(day, probe_id, sensor_id)},
            {"$set" : {
                "probe_id" : probe_id,
                "sensor_id" : sensor_id,
                "day" : day,
                "values.%d" % (timestamp) : data_point["data"]
            }}, True)  # True for upsert

    return None


def get_metric_id(day, probe_id, instrument_id):
    """ Returns a string that can be used as the document id for
        storing metric data.  Metric data is stored at daily 
        granularity, the id is a composite key of the day, probe id
        and instrument (sensor/actuator) id.
          ex:  YYYYMMDD-probe_id-instrument_id

    """
    mm = date_util.pad_month_day_value(day.month)
    dd = date_util.pad_month_day_value(day.day)
    return "%s%s%s-%s-%s" % (day.year, mm, dd, probe_id, instrument_id)


def determine_sensor_commands(probe_id):
    """ Determines the sensor commands for the given probe.

        TODO: In the future, all commands should be determined based
        upon a config file that the user can easily define.  However,
        for now just hardcode the sensor command logic.

    """
    sensor_commands = []

    if probe_id == "test_probe":
        interval = 300   # Sensor read interval in seconds
        sensor_commands.append({"id" : "tmp0", "interval" : interval})
        sensor_commands.append({"id" : "tmp1", "interval" : interval})
        sensor_commands.append({"id" : "pho0", "interval" : interval})
        sensor_commands.append({"id" : "mos0", "interval" : interval})

    return sensor_commands


# Initialize connection to DB when loading module
init_db()    