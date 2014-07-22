autogarten
==========
Autogarten is a project that facilitates automated gardening using [Arduino](http://www.arduino.cc/).  You can easily monitor temperature, light levels, soil mositure and more, then use that data to intelligently trigger actions like watering your garden or sending notifications.

An autogarten installation consists of one or more Arduino based **Probes** that communicate over WiFi with a central Python based **Control Server**.  For each Probe you define a set of **sensors** that will collect data, as well as a set of **actuators** that can perform actions.  Probes then sync with the Control Server to upload sensor data and get instructions for the actuators.  The Control Server provides a web based UI for visualizing collected data, and determines how to trigger the actuators based upon the data from other probes or the internet (ex: weather data).

<TODO: Add diagram of CtrlSrvr/Probe/Sensor/Actuator relationship>


# Running the Control Server

The autogarten Control Server is built as a Python [Flask](http://flask.pocoo.org/) application.  All communication with the Probes is over HTTP.  Before running the Control Server for the first time you'll need to edit `settings.cfg` and point to a running instance of [mongoDB](http://www.mongodb.org/) (v2.4+) which will store all collected data. Then, to start the Control Server...

    $ python -m control_server -v
    
The web based UI and web service will be available at [http://localhost:5000](http://localhost:5000).  When launching the Control Server this way it's running in debug and verbose mode.

## Setting an Auth Token

To prevent unauthorized Probes from syncing with the Control Server, an Auth Token (shared secret) is required to authenticate Probes.  It's a primitive form of security, but simple to setup and easy for an Arduino Probe to handle.  To change the default token, edit `settings.cfg`.  Keep in mind that all communication between Probes and the Control Server is currently only HTTP, so don't use a very sensitive value for the token.

## Generating Test Data

Now that the Control Server is running, you probably want to see some sample data before fully building an Arduino based Probe.  To accomplish this, there's a Python based test Probe that contains a variety of sensors which generate predictable test data.  To run...

    $ python -m test.test_data -v

It assumes the Control Server is running on [http://localhost:5000](http://localhost:5000) with the default auth token.  However, custom settings can be provided on the command line (-h for details).


# Building an Arduino based Probe

<TODO>

