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

db_sensor_data = None

probe_sync_interval = 15 * 60 * 1000    # Every 15 mins

def init_db():
    global db_sensor_data

    # Init DB
    db_sensor_data = mongo.get_sensor_data_collection()


def process_probe_sync(probe_sync):
    """ Processes a probe sync request.

    """

    probe_id = probe_sync.probe_id

    # Persist any sensor data
    persist_sensor_data(probe_id, probe_sync.sensor_data)

    # Persist any actuator history
    # TODO...

    # Determine commands for response
    # TODO...

    # Build response
    response = {
        "probe_id" : probe_sync.probe_id,
        "sensor_commands" : [] # TODO
    }

    # Finally provide time sync data.  Will be off by remaining
    # control server processing, one way latency and probe processing.
    now = datetime.now()
    response["curr_time"] = date_util.get_timestamp(now)
    response["next_sync"] = date_util.get_timestamp(now) + probe_sync_interval

    return response


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
        day = datetime(date_time.year, date_time.month, date_time.day)

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


# Initialize connection to DB when loading module
init_db()    