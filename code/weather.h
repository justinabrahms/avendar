#ifndef WEATHER_H
#define WEATHER_H

int calculate_seasonal_temperature_modifier();
int calculate_base_temperature(int temperatureIndex);
int calculate_daily_temperature_modifier(int cloudCover);

#endif
