#!/bin/bash

# Create a new zip
rm -rf Autogarten.zip
zip -r Autogarten.zip Autogarten

# Copy the latest into sketchbook library
cp Autogarten/*.cpp ~/sketchbook/libraries/Autogarten/
cp Autogarten/*.h ~/sketchbook/libraries/Autogarten/
