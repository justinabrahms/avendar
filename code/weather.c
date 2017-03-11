#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "weather.h"
#include "Direction.h"

void	init_weather		args ( (void) );
void	weather_update		args ( (void) );

void init_weather( void )
{
    AREA_DATA *pArea;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
	pArea->w_cur.cloud_cover	= 0;
	pArea->w_cur.storm_str		= 0;
        pArea->w_cur.precip_index	= 0;
	pArea->w_cur.lightning_str	= 0;
	pArea->w_cur.wind_dir		= pArea->base_wind_dir;
	pArea->w_new.tornado		= FALSE;
    }
}

static int interpolateHeatCycle(int cycleSize, int hottestPoint, int scale, int currentPoint)
{
    // Example:
    // cycleSize = 9
    // hottestPoint = 2
    // scale = 15 (which means [-15, 15])
    // currentPoint1 = 7
    // currentPoint2 = 1

    // Determine how many interval units away from the hottest we are; handle wrapping
    // Example: intervals = UMIN(7 - 2, 9 - 7 + 2) = UMIN(5, 4) = 4
    // Example: intervals = UMIN(2 - 1, 9 - 1 + 2) = UMIN(1, 10) = 1
    int intervals;
    if (currentPoint <= hottestPoint) 
        intervals = UMIN(hottestPoint - currentPoint, currentPoint + cycleSize - hottestPoint);
    else
        intervals = UMIN(currentPoint - hottestPoint, cycleSize - currentPoint + hottestPoint);

    // Determine the maximum number away it is possible to be
    // Example: maxIntervals = 9 / 2 = 4; note that integer truncation is desired
    int maxIntervals(cycleSize / 2);

    // Adjust for scale
    // Example: scaledIntervals = 0 - (15 * 4) / 4 = -15
    // Example: scaledIntervals = 0 - (15 * 1) / 4 = -3
    int scaledIntervals(0 - ((scale * intervals) / maxIntervals));

    // Now recenter the scale to be 0-based
    // Example: result = -15 * 2 + 15 = -15, which is the max cold and is correct since 7 is as far from 2 as possible on a 9-cycle
    // Example: result = -3 * 2 + 15 = 9
    return (scaledIntervals * 2 + scale);
}

int calculate_seasonal_temperature_modifier()
{
    // Determine the warmest day of the year
    int totalDays(0);
    int warmestDay(0);
    for (unsigned int i(0); month_table[i].name != NULL; ++i)
    {
        if (month_table[i].warmest_day)
            warmestDay = totalDays;

        totalDays += month_table[i].num_days;
    }

    // Now calculate seasonal modifer based on how far from the warmest day we are
    return interpolateHeatCycle(totalDays, warmestDay, 12, time_info.day_year);
}

int calculate_daily_temperature_modifier(int cloudCover)
{
    // Warmest time of day is 15:00 (in real life it is 15:00 to 16:30, but good enough)
    int baseMod(interpolateHeatCycle(NUM_HOURS_DAY, 15, 6, time_info.hour));

    // Cloud cover reduces the effect of the daytime adjustment, moderating temperatures throughout day and night
    // The reduction is linear from 0-50% (with 0-100% cloud cover)
    return (baseMod * (100 - (cloudCover / 2)) / 100);
}

int calculate_base_temperature(int temperatureIndex)
{
    switch (URANGE(TEMP_BLISTERING, temperatureIndex, TEMP_FRIGID))
    {
        case TEMP_BLISTERING:   return 15; 
        case TEMP_HOT:          return 5;
        case TEMP_TEMPERATE:    return 0;
        case TEMP_COOL:         return -5;
        case TEMP_FRIGID:       return -15;
    }

    bug("Impossible result in calculate_base_temperature", 0);
    return 0;    
}

void weather_update( void )
{
    extern const int rev_dir[];
    char outbuf[MAX_STRING_LENGTH];
    AREA_DATA *pArea;
    ALINK_DATA *alink;
    DESCRIPTOR_DATA *d;
    int i, sChance, tChance, wEff, oEff, aCount, oCount, old_precip;
    bool tBool, wStr;
    int num_dir[6], wind_dir[6], winds[6];
    int base_temp_mod = 0, base_precip_mod = 0, base_windmag_mod = 0;
    AFFECT_DATA *paf;
    bool hasMonsoon(false);
    bool hasIcestorm(false);
    bool hasTyphoon(false);
    bool hasElectricalStorm(false);
    bool hasMagicStorm(false);

    /* Step 1: Determine temperature */
    const season_type & season(season_table[time_info.season]);
    for (pArea = area_first; pArea; pArea = pArea->next)
    {
	/* Step 0.5: Scan for command weather affects */

	base_temp_mod = 0;
	base_precip_mod = 0;
	base_windmag_mod = 0;

    // Note that some of these weather modifiers look backwards, but are correct	
	for (paf = pArea->affected; paf; paf = paf->next)
    {
	    if (paf->type == gsn_commandweather)
        {
		    switch (paf->modifier)
    		{
	    	    case WCOM_DRIER: base_precip_mod--; break;
		        case WCOM_WETTER: base_precip_mod++; break;
		        case WCOM_CALMER: base_windmag_mod++; break;
    		    case WCOM_WINDIER: base_windmag_mod--; break;
	    	    case WCOM_COOLER: base_temp_mod++; break;
		        case WCOM_WARMER: base_temp_mod--; break;
		    }

            continue;
        }
       
        // Monsoons make it a lot wetter, somewhat windier, and a little cooler 
        if (paf->type == gsn_monsoon)
        {
            hasMagicStorm = true;
            hasMonsoon = true;
            base_precip_mod += 5;
            base_windmag_mod -= 2;
            base_temp_mod++;
        }

        // Icestorms make it a lot colder
        if (paf->type == gsn_icestorm)
        {
            hasMagicStorm = true;
            hasIcestorm = true;
            base_temp_mod += 5;
        }

        // Typhoons make it a lot windier
        if (paf->type == gsn_typhoon)
        {
            hasMagicStorm = true;
            hasTyphoon = true;
            base_windmag_mod -= 5;
        }

        // Register electrical storms
        if (paf->type == gsn_electricalstorm)
        {
            hasMagicStorm = true;
            hasElectricalStorm = true;
        }
    }
			
	/* Step 1: Determine temperature */
    pArea->w_new.temperature = calculate_base_temperature(pArea->base_temp + base_temp_mod);
	pArea->w_new.temperature += calculate_seasonal_temperature_modifier();
    pArea->w_new.temperature += calculate_daily_temperature_modifier(pArea->w_cur.cloud_cover);

    // Examine linked areas
	aCount = 0;
	wEff = 0;
    oEff = 0;
    oCount = 0;
	for (i = 0; i < 6; i++)
	{
	    num_dir[i] = 0;
	    wind_dir[i] = 0;
	}

    int areaTempMod(0);
    int averageTemp(0);
	for (alink = alink_first; alink; alink = alink->next)
    {
        AREA_DATA * other;
	    if (alink->a1 == pArea && alink->a2->w_cur.wind_dir == alink->dir1) other = alink->a2;
        else if (alink->a2 == pArea && alink->a1->w_cur.wind_dir == alink->dir2) other = alink->a1;
        else continue;

        // Found an area which blows towards this area, so add in the stats
        ++aCount;
		wEff += other->w_cur.wind_mag;
        areaTempMod += ((other->w_cur.temperature - pArea->w_cur.temperature) * other->w_cur.wind_mag);
        averageTemp += other->w_cur.temperature;
		++num_dir[other->w_cur.wind_dir];
		wind_dir[other->w_cur.wind_dir] += other->w_cur.wind_mag;
		if (other->w_cur.wind_dir == rev_dir[pArea->w_cur.wind_dir])
		{
		    oEff += other->w_cur.wind_mag;
		    ++oCount;
		}
	}

    // Factor in the temperatures of the surrounding areas which are blowing into this one
	if (aCount == 0)
        averageTemp = pArea->w_cur.temperature;
    else
	{
	    wEff /= aCount;
        averageTemp /= aCount;
	    pArea->w_new.temperature += (areaTempMod / (150 * aCount));
	}

    // Add in a random factor, and make sure ice storms are not higher than a certain temperature
	pArea->w_new.temperature += number_range(-4, 4);
    if (hasIcestorm)
        pArea->w_new.temperature = UMIN(pArea->w_new.temperature, -8);
	
    if (oCount != 0)
        oEff /= oCount;

	for (i = 0; i < 6; i++)
    {
	    if (num_dir[i] > 0) winds[i] = (wind_dir[i] / num_dir[i]);
	    else winds[i] = 0;
    }

	/* Step 2: Generate wind fields */
	switch (URANGE(WMAG_GALE, pArea->base_wind_mag + base_windmag_mod, WMAG_DOLDRUMS))
	{
	    case WMAG_GALE:		pArea->w_new.wind_mag = 80;	break;
	    case WMAG_WINDY:	pArea->w_new.wind_mag = 50;	break;
	    case WMAG_NORMAL:	pArea->w_new.wind_mag = 30;	break;
	    case WMAG_STILL:    pArea->w_new.wind_mag = 15;	break;
	    case WMAG_DOLDRUMS: pArea->w_new.wind_mag = 0;  break;
	    default:            pArea->w_new.wind_mag = 50; break;
	}

    // Factor in temperature differential, existing storms, and seasonal adjustments
	pArea->w_new.wind_mag += abs(averageTemp - pArea->w_cur.temperature);
	pArea->w_new.wind_mag += (pArea->w_cur.storm_str / 2);
	pArea->w_new.wind_mag += season.wind_mod;

    // Factor in geography
	for (i = 0; geo_table[i].bit; i++)
    {
	    if (IS_SET(pArea->geography, geo_table[i].bit))
	    	pArea->w_new.wind_mag += geo_table[i].wind_mod;
    }

	pArea->w_new.wind_mag += (wEff / 2);
	pArea->w_new.wind_mag -= (pArea->w_cur.wind_mag / 2);
	pArea->w_new.wind_mag = UMAX(pArea->w_new.wind_mag, 0);

    // Guarantee a minimum wind strength with typhoon
    if (hasTyphoon)
        pArea->w_new.wind_mag = UMAX(pArea->w_new.wind_mag, 65);

	/* Step 2.5: Wind direction */
	sChance = number_percent();

	if (sChance < 25)       pArea->w_new.wind_dir = pArea->w_cur.wind_dir; // 25% chance of staying the same
	else if (sChance < 50)  pArea->w_new.wind_dir = pArea->base_wind_dir;  // 25% chance of reverting to base
	else if (sChance < 70)
    {
        // 20% chance of switching to the highest average direction of surrounding areas blowing into here
    	int j = 0;
	    for (i = 1; i < 6; i++)
        {
            // Find the largest average
		    if (winds[i] > winds[j])
		        j = i;
        }
	    
	    pArea->w_new.wind_dir = j;
	}
	else if (sChance < 95)
	{
        // 17% chance of switching 90 degrees to the stronger surrounding wind
        // 8% chance of switching 90 degrees to the weaker surrounding wind
        // If up/down, just use the base direction
	    if (pArea->base_wind_dir == 0 || pArea->base_wind_dir == 2)
            pArea->w_new.wind_dir = (winds[1] > winds[3] ? (sChance < 87 ? 1 : 3) : (sChance < 87 ? 3 : 1));
        else if (pArea->base_wind_dir == 1 || pArea->base_wind_dir == 3)
            pArea->w_new.wind_dir = (winds[0] > winds[2] ? (sChance < 87 ? 0 : 2) : (sChance < 87 ? 2 : 0));
	    else
		    pArea->w_new.wind_dir = pArea->base_wind_dir;
	}
	else // 5% chance of switching to the reverse direction of base
	    pArea->w_new.wind_dir = rev_dir[pArea->base_wind_dir];   


	/* Step 3: Precipitation Index */
	old_precip = pArea->w_new.precip_index;
	switch (URANGE(PRECIP_BARREN, pArea->base_precip + base_precip_mod, PRECIP_MONSOON))
	{
	    case PRECIP_BARREN:	    pArea->w_new.precip_index -= 2;	break;
	    case PRECIP_ARID:	    pArea->w_new.precip_index -= 1;	break;
	    case PRECIP_AVERAGE:	pArea->w_new.precip_index += 0;	break;
	    case PRECIP_WET:		pArea->w_new.precip_index += 1;	break;
	    case PRECIP_MONSOON:	pArea->w_new.precip_index += 2;	break;
	    default:                pArea->w_new.precip_index += 0;	break;
	}

    // Adjust for geography
	for (i = 0; geo_table[i].bit; i++)
    {
	    if (IS_SET(pArea->geography, geo_table[i].bit))
	    	pArea->w_new.precip_index += geo_table[i].precip_mod;
    }

	if (pArea->w_new.precip_index > 0)
    {
        // Wetter at night due to cooler air holding less moisture
	    if ((time_info.hour >= season_table[time_info.season].sun_up) && (time_info.hour < season_table[time_info.season].sun_down))
    		pArea->w_new.precip_index -= number_range(0, 1);
	    else
	    	pArea->w_new.precip_index += number_range(0, 1);
    }

    // Average with the old precip during a storm, if the new one is wetter
	if ((pArea->w_cur.storm_str > 0) && (pArea->w_new.precip_index > old_precip))
	    pArea->w_new.precip_index = (pArea->w_new.precip_index + old_precip) / 2;

	pArea->w_new.precip_index = UMAX(pArea->w_new.precip_index, 0);

	/* Step 4: Cloud Cover */	
	pArea->w_new.cloud_cover = 15 * pArea->w_new.precip_index;
	
    // Adjust for geography
	for (i = 0; geo_table[i].bit; i++)
    {
	    if (IS_SET(pArea->geography, geo_table[i].bit))
		    pArea->w_new.cloud_cover += geo_table[i].cloud_mod;
    }

	switch (URANGE(PRECIP_BARREN, pArea->base_precip + base_precip_mod, PRECIP_MONSOON))
	{
	    case PRECIP_BARREN:	    pArea->w_new.cloud_cover -= 50;	break;
	    case PRECIP_ARID:	    pArea->w_new.cloud_cover -= 30;	break;
	    case PRECIP_AVERAGE:	pArea->w_new.cloud_cover += 0;	break;
	    case PRECIP_WET:		pArea->w_new.cloud_cover += 15;	break;
	    case PRECIP_MONSOON:	pArea->w_new.cloud_cover += 30;	break;
	    default:                pArea->w_new.cloud_cover += 0;	break;
	}

    // Random factor
	pArea->w_new.cloud_cover += number_range(-20, 20);	
	pArea->w_new.cloud_cover = URANGE(0, pArea->w_new.cloud_cover, 100);

    // Consumption of the Silver Veil makes for complete cloud cover
	if (silver_state == SILVER_FINAL)
	    pArea->w_new.cloud_cover = 100;

	/* Step 5: Storm Generation */
	if (pArea->w_cur.storm_str <= 0)
    {
	int sBase = 0;

    // From 2pm until sundown there are increased odds
	if ((time_info.hour >= ((NUM_HOURS_DAY / 2) + 2)) && (time_info.hour < season_table[time_info.season].sun_down))
	    sBase += 4;

    // Adjust for geography
	for (i = 0; geo_table[i].bit; i++)
    {
	    if (IS_SET(pArea->geography, geo_table[i].bit))
	    	sBase += geo_table[i].storm_mod;
    }

    // Adjust for cloud cover and temperature differential
	sBase += (pArea->w_new.cloud_cover / 10);
	sBase += (abs(averageTemp - pArea->w_cur.temperature) / 2);

    // Adjust for windiness and a random factor
	sBase += (4 * UMIN(num_dir[0], num_dir[2]));
	sBase += (4 * UMIN(num_dir[1], num_dir[3]));
	sBase += (4 * UMIN(num_dir[4], num_dir[5]));
	sBase += number_range(-3, 3);

	/* The big moment: Is there a storm? */
	if (hasMagicStorm || number_percent() < (sBase * pArea->w_new.precip_index))
	{
        // Initialize
	    pArea->w_new.precip_type = -1;

        // Check cold weather for snow
	    if (pArea->w_new.temperature <= -5)
	    {
            // If the cold is extreme, the chance of precipitation is much reduced
    		if (pArea->w_new.temperature <= -30) sChance = 0;
    		else sChance = 20;
    		sChance += season.snow_chance;
	    	if (pArea->w_new.wind_mag >= 50)
		        sChance += 20;

            // Check for snow (type 0)
    		if (number_percent() < sChance)
	    	    pArea->w_new.precip_type = 0;
	    }

        // If no snow and the cold is not extreme, check for hail and rain
	    if (pArea->w_new.precip_type == -1 && pArea->w_new.temperature > -30)
        {
    		if (number_percent() < (pArea->w_new.wind_mag / 5))
	    	    pArea->w_new.precip_type = 1;   /* Hail */
		    else
		        pArea->w_new.precip_type = 2;   /* Rain */
        }

        // If a monsoon or electrical storm, overwrite any previous decision with rain
        if (hasMonsoon || hasElectricalStorm)
            pArea->w_new.precip_type = 2;

	    /* Determine Storm Strength */
	    pArea->w_new.storm_str = (pArea->w_new.precip_index / 6);
	    pArea->w_new.storm_str += season.storm_mod;
	    pArea->w_new.storm_str += (pArea->w_new.wind_mag / 3);

	    pArea->w_new.storm_str += (5 * UMIN(num_dir[0], num_dir[2]));
	    pArea->w_new.storm_str += (5 * UMIN(num_dir[1], num_dir[3]));
 	    pArea->w_new.storm_str += (5 * UMIN(num_dir[4], num_dir[5]));

	    pArea->w_new.storm_str += (abs(averageTemp - pArea->w_cur.temperature) / 2);
   
        // Adjust for geography 
 	    for (i = 0; geo_table[i].bit; i++)
        {
	    	if (IS_SET(pArea->geography, geo_table[i].bit))
		       pArea->w_new.cloud_cover += geo_table[i].sstr_mod;
        }

        // Adjust for precipitation
	    switch (URANGE(PRECIP_BARREN, pArea->base_precip + base_precip_mod, PRECIP_MONSOON))
	    {
		    case PRECIP_BARREN:     pArea->w_new.storm_str -= 75;   break;
    		case PRECIP_ARID:	    pArea->w_new.storm_str -= 30;   break;
	    	case PRECIP_AVERAGE:    pArea->w_new.storm_str += 0;    break;
		    case PRECIP_WET:	    pArea->w_new.storm_str += 15;   break;
    		case PRECIP_MONSOON:    pArea->w_new.storm_str += 20;   break;
	    	default:                pArea->w_new.storm_str += 0;    break;
	    }

        // Factor in some randomness
	    pArea->w_new.storm_str += number_range(-15, 15);

        // If a monsoon or electrical storm, require a minimum storm strength
        if (hasMonsoon) pArea->w_new.storm_str = UMAX(pArea->w_new.storm_str, 60);
        if (hasElectricalStorm) pArea->w_new.storm_str = UMAX(pArea->w_new.storm_str, 40);
	
	    /* Determine whether lightning is present, and strength */
	    if ((pArea->w_new.precip_type == 2) && (hasElectricalStorm || number_percent() < pArea->w_new.storm_str))   /* Lightning only in rain */
	    {	    
            // Adjust for temperature differential, season, storm strength, and randomness
    		pArea->w_new.lightning_str = abs(averageTemp - pArea->w_cur.temperature);
    		pArea->w_new.lightning_str += season.lightning_mod;
    		pArea->w_new.lightning_str += (pArea->w_new.storm_str / 2);
		    pArea->w_new.lightning_str += number_range(-15, 15);
    		pArea->w_new.lightning_str = URANGE(0, pArea->w_new.lightning_str, 100);

            // Require a minimum lightning strength in an electrical storm
            if (hasElectricalStorm)
                pArea->w_new.lightning_str = UMAX(pArea->w_new.lightning_str, 50);
	    }
	}
	}
	else // Storm currently exists
	{
        // Diminish the storm partially, unless in a magically-induced storm
        if (!hasMagicStorm)
        {
    	    pArea->w_new.precip_index -= (pArea->w_cur.storm_str / 15) + 1;
	        pArea->w_new.precip_index = UMAX(pArea->w_new.precip_index, 0);
        }

	    if (pArea->w_new.precip_index > 0)
	    {
            // Iterate the surrounding areas to see whether we spread our storm into them
	    	for (alink = alink_first; alink; alink = alink->next)
	    	{
                // Find areas we are blowing into from here
    		    AREA_DATA * vArea;
	    	    if ((alink->a1 == pArea) && (alink->dir2 == pArea->w_new.wind_dir)) vArea = alink->a2;
		        if ((alink->a2 == pArea) && (alink->dir1 == pArea->w_new.wind_dir)) vArea = alink->a1;
                else continue;

                // Determine chance based on our magnitude, reduced for the opposing wind
		        sChance = (pArea->w_new.wind_mag / 2);
		        sChance -= (oEff / 2);

                // Adjust for area
	    	    switch (vArea->base_precip)
	    	    {
			        case PRECIP_BARREN:     sChance -= 100; break;
        			case PRECIP_ARID:		sChance -= 60; 	break;
        			case PRECIP_AVERAGE:    sChance += 0;  	break;
        			case PRECIP_WET:    	sChance += 20; 	break;
        			case PRECIP_MONSOON:  	sChance += 40; 	break;
        			default: break;
		        }

                // Adjust for geography
		    	for (i = 0; geo_table[i].bit; i++)
                {
    			    if (IS_SET(vArea->geography, geo_table[i].bit))
	    		    	sChance += geo_table[i].storm_move;
                }

                // Add in some randomness and check
    		    sChance += number_range(-10, 10);
	    	    if (number_percent() < sChance)
		        {
                    // Storm is spreading, determine precip index randomly but bounded by our storm
			        sChance += number_range(-20, 20);
        			sChance = URANGE(0, sChance, pArea->w_new.precip_index);

                    // As the storm enters, it pulls strength from this one
		        	vArea->w_new.precip_index += sChance;
        			pArea->w_new.precip_index -= sChance;
		        	pArea->w_new.precip_index = UMAX(pArea->w_new.precip_index, 0);
			        ++vArea->storm_enter[pArea->w_new.wind_dir];

                    // Bail out if no strength left in the storm
	                if (pArea->w_new.precip_index <= 0)
	    	    	    break;
                }
		    }
	    }

        // If no strength left, cancel the storm
	    if (pArea->w_new.precip_index <= 0)
	    {
    		pArea->w_new.storm_str = 0;
	    	pArea->w_new.lightning_str = 0;
	    }
	}
    }

    /* Check for tornado formation, and apply weather effects */
    for (pArea = area_first; pArea; pArea = pArea->next)
    {
    	tChance = 0;
	    tBool = FALSE;

        // Tornados can only happen if two storms enter from opposite directions
    	if ((pArea->storm_enter[0] > 0) && (pArea->storm_enter[2] > 0)) tBool = TRUE;
	    if ((pArea->storm_enter[1] > 0) && (pArea->storm_enter[3] > 0)) tBool = TRUE;
    	if ((pArea->storm_enter[4] > 0) && (pArea->storm_enter[5] > 0)) tBool = TRUE;

	    if (tBool)
	    {
            // Adjust for the number of storms entering
	        for (i = 0; i < 6; i++)
		        tChance += (pArea->storm_enter[i] * 2);

	        tChance += (pArea->w_new.wind_mag / 20);
	        tChance += season.tornado_mod;
	    
    	    if (number_percent() < tChance)
        		pArea->w_new.tornado = TRUE;
    	}

	/** Send weather echoes **/

	wStr = TRUE;
	if (pArea->w_cur.storm_str != pArea->w_new.storm_str)
	{
	    if (pArea->w_cur.storm_str == 0)	/* Storm has now started */
	    {
		if (pArea->w_new.precip_type == 0)
		{
		    if (pArea->w_new.storm_str < 10)
			sprintf(outbuf, "A few snowflakes begin to fall from above");
		    else if (pArea->w_new.storm_str < 35)
			sprintf(outbuf, "A light snowfall begins to blow");
		    else if (pArea->w_new.storm_str < 65)
			sprintf(outbuf, "Snow begins to fall steadily from the sky");
		    else if (pArea->w_new.storm_str < 90)
			sprintf(outbuf, "A thick snowfall begins to blow from the sky");
		    else
			sprintf(outbuf, "A blizzard sweeps up, obscuring your vision");
		}
		else if (pArea->w_new.precip_type == 1)
		{
		    if (pArea->w_new.storm_str < 25)
			sprintf(outbuf, "Tiny bits of hail begin to fall down from the sky");
		    else if (pArea->w_new.storm_str < 50)
			sprintf(outbuf, "Small pieces of hail begin to rain down from above");
		    else if (pArea->w_new.storm_str < 75)
			sprintf(outbuf, "Large hailstones begin to hurtle down from above");
		    else
			sprintf(outbuf, "Huge pieces of ice begin to rain down from the sky");
		}
		else
		{
		    if (pArea->w_new.storm_str < 10)
		    	sprintf(outbuf, "A light mist of rain begins falling from the sky");
		    else if (pArea->w_new.storm_str < 35)
		    	sprintf(outbuf, "A light drizzle of rain begins to fall from above");
	 	    else if (pArea->w_new.storm_str < 65)
		    	sprintf(outbuf, "A steady flow of rain begins to fall from the sky");
		    else if (pArea->w_new.storm_str < 90)
		    	sprintf(outbuf, "A heavy downpour of rain begins to fall");
		    else
		    	sprintf(outbuf, "Torrential sheets of rain begin to fall from the sky");
	        }
	
		if (pArea->w_new.lightning_str > 0)
		    strcat(outbuf, ", accompanied by flashes of lightning.\n\r");
		else
		    strcat(outbuf, ".\n\r");

	    }
	    else
	    {
		if (pArea->w_cur.precip_type == 0)
		    sprintf(outbuf, "Snow stops falling around you.\n\r");
		else if (pArea->w_cur.precip_type == 1)
		    sprintf(outbuf, "Hailstones stop falling from the sky.\n\r");
		else
		    sprintf(outbuf, "The falling rain lets up.\n\r");
	    } 

	}
	else if ((pArea->w_cur.cloud_cover <= 10) && (pArea->w_new.cloud_cover > 10))
	    if (silver_state == SILVER_FINAL)
		sprintf(outbuf, "Dark clouds boil across the horizon, spreading across the heavens.\n\r");
	    else
	        sprintf(outbuf, "Clouds move into the area, obscuring the sky.\n\r");
	else if ((pArea->w_new.cloud_cover <= 10) && (pArea->w_cur.cloud_cover > 10))
	    sprintf(outbuf, "The clouds begin to disperse, clearing the sky.\n\r");
	else if ((pArea->w_cur.wind_mag <= 10) && (pArea->w_new.wind_mag > 10))
	    sprintf(outbuf, "The wind picks up%s\n\r",
		((pArea->w_new.wind_dir == 0) ? ", blowing from the south." : 
		 (pArea->w_new.wind_dir == 1) ? ", blowing from the west." :
		 (pArea->w_new.wind_dir == 2) ? ", blowing from the north." :
		 (pArea->w_new.wind_dir == 3) ? ", blowing from the east." :
		 "."));
	else if ((pArea->w_new.wind_mag <= 10) && (pArea->w_cur.wind_mag > 10))
	    sprintf(outbuf, "The gusting wind dies down.\n\r");
	else
	    wStr = FALSE;	

	if (wStr)
    {
	    for (d = descriptor_list; d; d = d->next)
        {
	        if ((d->connected == CON_PLAYING)
		    && (d->character->in_room)
    		&& (d->character->in_room->area == pArea)
	    	&& !IS_SET(d->character->nact, PLR_NOWEATHER)
		    && IS_OUTSIDE(d->character)
    		&& IS_AWAKE(d->character)
	    	&& !IS_SET(d->character->in_room->room_flags, ROOM_NOWEATHER))
            {
		        send_to_char(outbuf, d->character);

                // Send echoes for call upon wind
                AFFECT_DATA * call(get_affect(d->character, gsn_calluponwind));
                if (call != NULL)
                {
                    switch (call->modifier)
                    {
                        case Direction::North:
                            if (pArea->w_new.wind_dir == Direction::South && pArea->w_cur.wind_dir != Direction::South)
                                send_to_char("You feel empowered as the chill northern wind flows about you!\n", d->character);
                            else if (pArea->w_new.wind_dir != Direction::South && pArea->w_cur.wind_dir == Direction::South)
                                send_to_char("The chill power of the northern air departs from you as the winds change.\n", d->character);
                            break;

                        case Direction::East:
                            if (pArea->w_new.wind_dir == Direction::West && pArea->w_cur.wind_dir != Direction::West)
                                send_to_char("You feel empowered as the baleful eastern wind creeps about you!\n", d->character);
                            else if (pArea->w_new.wind_dir != Direction::West && pArea->w_cur.wind_dir == Direction::West)
                                send_to_char("The noxious power of the eastern draft departs from you as the winds change.\n", d->character);
                            break;
 
                         case Direction::South:
                            if (pArea->w_new.wind_dir == Direction::North && pArea->w_cur.wind_dir != Direction::North)
                                send_to_char("You feel empowered as the dry heat of the southern wind whips about you!\n", d->character);
                            else if (pArea->w_new.wind_dir != Direction::North && pArea->w_cur.wind_dir == Direction::North)
                                send_to_char("The dessicating power of the southern current departs from you as the winds change.\n", d->character);
                            break;

                         case Direction::West:
                            if (pArea->w_new.wind_dir == Direction::East && pArea->w_cur.wind_dir != Direction::East)
                                send_to_char("You feel empowered as the hale western wind gusts about you!\n", d->character);
                            else if (pArea->w_new.wind_dir != Direction::East && pArea->w_cur.wind_dir == Direction::East)
                                send_to_char("The vital power of the western breeze departs from you as the winds change.\n", d->character);
                            break;
                    }
                }
            }
        }
    }
	pArea->w_cur = pArea->w_new;
    }
}
