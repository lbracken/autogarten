autogarten
==========
Autogarten is a project that facilitates automated gardening using Arduino.  You can easily monitor temperature, light levels, soil mositure and more, then use that data to intelligently trigger actions like watering your garden.

An autogarten installation consists of one or more Arduino based **_Probes_** that communicate with a central Python based **_Control Server_**.  For each **_Probe_** you define a set of **_sensors_** that will collect data, as well as a set of **_actuators_** that can perform actions.  Probes then sync with the Control Server over WiFi to upload sensor data and get instructions for the actuators.
