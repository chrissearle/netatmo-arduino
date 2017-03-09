#!/usr/bin/env python3
# encoding=utf-8

import lnetatmo
import json

authorization = lnetatmo.ClientAuth()

weatherData = lnetatmo.WeatherStationData(authorization)

data = weatherData.lastData()
dataIndoor = data['Indoor']
dataOutdoor = data['Outdoor']
dataRain = data['Rain gauge']
dataWind = data['Wind Gauge']

output = {
    'Indoor':
        {
            'CO2': dataIndoor['CO2'],
            'Humidity': dataIndoor['Humidity'],
            'Noise': dataIndoor['Noise'],
            'Pressure': dataIndoor['Pressure'],
            'Temperature': dataIndoor['Temperature'],
            'MaxTemp': dataIndoor['max_temp'],
            'MinTemp': dataIndoor['min_temp'],
            'PressureTrend': dataIndoor['pressure_trend']
        },
    'Outdoor':
        {
            'Humidity': dataOutdoor['Humidity'],
            'Temperature': dataOutdoor['Temperature'],
            'MaxTemp': dataOutdoor['max_temp'],
            'MinTemp': dataOutdoor['min_temp'],
            'TemperatureTrend': dataOutdoor['temp_trend']
        },
    'Rain':
        {
            'Rain': dataRain['Rain'],
            'RainHour': dataRain['sum_rain_1'],
            'RainDay': dataRain['sum_rain_24']
        },
    'Wind':
        {
            'GustStrength': dataWind['GustStrength'],
            'WindStrength': dataWind['WindStrength'],
            'MaxWind': dataWind['max_wind_str']
        }
}

json_data = json.dumps(output)

print(json_data)
