# -*- coding: utf-8 -*-
"""
    autogarten.service.probe_service
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Module that provides probe related services.

    :license: MIT, see LICENSE for more details.
"""

import json

from datetime import date
from datetime import datetime
from datetime import timedelta

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


def get_probe_overview():
    """ Returns overview information on all probes in the system

    """
    probe_overview = []

    # Get status information on all probes
    probes_status = db_probe_status.find({})
    for probe_status in probes_status:

        # Create the probe return collection
        probe = {
            "id" : probe_status["_id"],
            "desc" : "Mock probe, generates interesting fake data with Python", # TODO ps["desc"],
            "last_contact" : probe_status["last_contact"],
            "first_contact" : probe_status["first_contact"],
            "last_restart" : probe_status["last_restart"],
            "sync_count" : probe_status["sync_count"],
            "sensors" : []
        }
        
        # Get recent sensor data
        end_time = datetime.now()
        start_time = end_time - timedelta(days=7)
        sensors_data = get_sensor_data_for_probe(
            probe["id"], start_time, end_time)

        for sensor_data in sensors_data:

            sensor = {
                "id" : sensor_data["_id"],
                "desc" : "TODO Sensor Description...",
                "units_label" : "&deg;",  # TODO: Need data type (degrees, etc)
                "curr_value" : sensor_data["data"].values()[-1],
                "min_value" : sensor_data["min_value"],
                "max_value" : sensor_data["max_value"]
            }

            data = bucketize_data(168,
                date_util.get_timestamp(start_time),
                date_util.get_timestamp(end_time),
                sensor_data["data"])

            sensor["data"] = data
            sensor["avg_value"] = average_list_values(data)
            probe["sensors"].append(sensor)

        probe_overview.append(probe)

    return probe_overview


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
    update_set = { "last_contact" : now}

    if sync_count <= 1:
        update_set["last_restart"] = now

    db_probe_status.update(
        {"_id" : probe_id}, {
            "$set" : update_set,
            "$inc" : {"sync_count" : 1},
            "$setOnInsert" : {"first_contact" : now}
        }, True)  # True for upsert


def get_sensor_data_for_probe(probe_id, start_time, end_time):
    """ Gets sensor data over the given time range for the given probe.

        *** Runs the following mongoDB aggregation ***

        db.sensor_data.aggregate(

          { $match : 
            {
              "probe_id" : "test_probe",
              "sensor_id" : "pho0",
              "day" : {
                $gte : ISODate("2014-05-06T00:00:00Z"),
                $lte : ISODate("2014-05-09T00:00:00Z")
              }
            }
          },

          {
            $group :
            {
              "_id" : { "probe_id" : "$probe_id", "sensor_id" : "$sensor_id"},
              "min_value": { "$min" : "$min_value"},
              "max_value": { "$min" : "$max_value"},
              "max_value": { "$min" : "$max_value"},
              "data" : { "$push" : "$data" },
            }
          }
        )
    """
    sensor_data = db_sensor_data.aggregate([
        {"$match": {
            "probe_id" : probe_id,
            "day" : {
                "$gte" : start_time,
                "$lte" : end_time
            }
        }},
        {"$group" : {
            "_id" : "$sensor_id",
            "min_value": { "$min" : "$min_value"},
            "max_value": { "$max" : "$max_value"},
            "data" : { "$push" : "$data" },     
        }}
    ])

    for sensor in sensor_data["result"]:

        # Values are still grouped by day and need to be merged
        merged_values = {}
        for values in sensor["data"]:
            merged_values.update(values)
        sensor["data"] = merged_values

    # TODO: Values are for the days in the start to end range.  The
    # time isn't respected.  Need to go through and prune values
    # before/after time range?

    return sensor_data["result"]


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
        value = data_point["data"]

        db_sensor_data.update(
            {"_id" : get_metric_id(day, probe_id, sensor_id)},
            {"$set" : {
                "probe_id" : probe_id,
                "sensor_id" : sensor_id,
                "day" : day,
                "data.%d" % (timestamp) : value},
             "$inc" : { 
                "count_values" : 1,
                "sum_values" : value},
             "$min" : {
                "min_value" : value},
             "$max" : {
                "max_value" : value}
            }, True)  # True for upsert

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

    if probe_id == "test_probe2":
        interval = 300   # Sensor read interval in seconds
        sensor_commands.append({"id" : "tmp0", "interval" : interval})     

    return sensor_commands


def bucketize_data(bucket_count, min_bucket, max_bucket, data):
    """ Maps the given data into the specified number of buckets.

    """
    buckets = {}
    divisor = (max_bucket - min_bucket) / bucket_count
    #print "Min: %d   Max: %d    Div: %d" % (min_bucket, max_bucket, divisor)

    # Map the data to the buckets
    for d in data:
        if int(d) < min_bucket or int(d) > max_bucket:
            #print "Invalid Data Point %s" % d
            continue

        # Determine bucket, then add to bucket list
        key = (int(d) - min_bucket) / divisor
        if key not in buckets:
            buckets[key] = [data[d]]
        else:
            buckets[key].append(data[d])

    
    # Average the values in each bucket
    for key in buckets:
        buckets[key] = average_list_values(buckets[key])

    return buckets.values()


def average_list_values(l):
    return sum(l)/float(len(l))


# Initialize connection to DB when loading module
init_db()    