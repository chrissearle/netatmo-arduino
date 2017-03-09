Display netatmo data on arduino/5110 screen

## script

Python code to run on a web server - this just fetches the info and does some json parsing.
Install it with cron and redirect output to a static file for serving over http

It requires the following four environment variables:

* CLIENT_ID = netatmo app client id
* CLIENT_SECRET = netatmo app client secret
* USERNAME = netatmo account username
* PASSWORD = netatmo account password

To create an app - see https://dev.netatmo.com/dev/createanapp

## arduino

TODO