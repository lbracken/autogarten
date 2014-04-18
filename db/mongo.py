# -*- coding: utf-8 -*-
"""
    autogarten.db.mongo
    ~~~~~~~~~~~~~~~~~~~

    This module supports interaction with mongoDB.

    Usage: Call a get_<collection_name>() function to return a
    connection to a particular mongoDB collection.  The connection
    should he reused to limit the number of concurrent connections
    open to mongoDB.
    
    :license: MIT, see LICENSE for more details.
"""

import ConfigParser
import pymongo


# These values set from config file
db_host = None
db_port = None


def init_config():
    """ Read mongoDB connection settings from config file

    """
    global db_host, db_port

    config = ConfigParser.SafeConfigParser()
    config.read("settings.cfg")

    db_host = config.get("mongo", "db_host")
    db_port = config.getint("mongo", "db_port")  


def get_mongodb_connection(collection_name):
    print " * Connecting to mongoDB @ %s:%d   autogartenDB.%s" % \
            (db_host, db_port, collection_name)

    client = pymongo.MongoClient(db_host, db_port)
    return client["autogartenDB"][collection_name]


def get_probe_status_collection():
    collection = get_mongodb_connection("probe_status")
    return collection


def get_sensor_data_collection():
    collection = get_mongodb_connection("sensor_data")
    return collection


# Initialize config when loading module
init_config()