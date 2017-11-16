#!/usr/bin/env python3
# encoding=utf-8
import datetime
from pprint import pprint
from urllib.error import HTTPError

import lnetatmo
import json
import os
import sys


def load_secrets():
    try:
        with open(os.path.join(os.path.dirname(__file__), 'secrets.json')) as f:
            return json.load(f)
    except FileNotFoundError:
        return {}


def build_json(weather_data):
    data = weather_data.lastData()

    indoor = data['Indoor']
    outdoor = data['Outdoor']
    rain = data['Rain gauge']
    wind = data['Wind Gauge']

    return {
        'Indoor':
            {
                'CO2': indoor['CO2'],
                'Humidity': indoor['Humidity'],
                'Noise': indoor['Noise'],
                'Pressure': indoor['Pressure'],
                'Temperature': indoor['Temperature'],
                'MaxTemp': indoor['max_temp'],
                'MinTemp': indoor['min_temp'],
                'PressureTrend': indoor['pressure_trend'],
                'LastSeen': indoor['When'],
                'LastSeenUTC': datetime.datetime.utcfromtimestamp(indoor['When']).strftime('%Y-%m-%dT%H:%M:%SZ'),
            },
        'Outdoor':
            {
                'Humidity': outdoor['Humidity'],
                'Temperature': outdoor['Temperature'],
                'MaxTemp': outdoor['max_temp'],
                'MinTemp': outdoor['min_temp'],
                'TemperatureTrend': outdoor['temp_trend'],
                'LastSeen': outdoor['When'],
                'LastSeenUTC': datetime.datetime.utcfromtimestamp(outdoor['When']).strftime('%Y-%m-%dT%H:%M:%SZ'),
            },
        'Rain':
            {
                'Rain': rain['Rain'],
                'RainHour': rain['sum_rain_1'] if 'sum_rain_1' in rain else None,
                'RainDay': rain['sum_rain_24'] if 'sum_rain_24' in rain else None,
                'LastSeen': rain['When'],
                'LastSeenUTC': datetime.datetime.utcfromtimestamp(rain['When']).strftime('%Y-%m-%dT%H:%M:%SZ'),
            },
        'Wind':
            {
                'GustStrength': wind['GustStrength'],
                'WindStrength': wind['WindStrength'],
                'MaxWind': wind['max_wind_str'],
                'LastSeen': wind['When'],
                'LastSeenUTC': datetime.datetime.utcfromtimestamp(wind['When']).strftime('%Y-%m-%dT%H:%M:%SZ'),
            }
    }


def dump_json(data):
    fh = sys.stdout

    if len(sys.argv) > 1:
        fh = open(sys.argv[1], "w")

    print(data, file=fh)

    if len(sys.argv) > 1:
        fh.close()


secrets = load_secrets()

try:
    authorization = lnetatmo.ClientAuth(clientId=secrets.get('CLIENT_ID', ''),
                                        clientSecret=secrets.get('CLIENT_SECRET', ''),
                                        username=secrets.get('USERNAME', ''),
                                        password=secrets.get('PASSWORD', ''))

    weatherData = lnetatmo.WeatherStationData(authorization)

    #pprint(weatherData.__dict__)

    output = build_json(weatherData)

    json_data = json.dumps(output)

    dump_json(json_data)
except HTTPError as err:
    print("Fetch error {err.code} - {err.msg}".format(err=err), file=sys.stderr)
