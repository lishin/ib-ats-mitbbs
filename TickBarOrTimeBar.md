maybe it's better to use tick bar than timebar like(1min Bar, 5min Bar) to scalp currency pairs.

# Possible reason to support it: #
1. tick bar graph does not (or less sensitive to) distinguish regular trading hours when liquidity is high to other trading hours when activity is thin

2. tick bar records more details in spike activities

## Technical concerns: ##
for IB API, there is no TRADES/VOLUME for currencies. and it's hard to track realtime bar with current function provided.

### Possible ways to track realtime 5 minute bar: ###
1. request current time when connected

2. figure out the next **EXACT** ending time, eg. if current time is 15:23:23, next **EXACT** ending time is 15:25:00

3. when server time is AT the exact ending time 15:25:00, make a request on the data, request historical data every 5 minutes

a. synchronize local and server time, check local time

b. keep requesting current time on server side


### Or ###
1. request current time when connected

2. request realtime bar, only **5 SEC** is supported currently

3. figure out the next **EXACT** ending time, eg. if current time is 15:23:23, next **EXACT** ending time is 15:25:00

4. request historical data, and convert to realtime bar and concatenate realtime bar to the 5 minutes bar


# Drawback: #
1. too many time manipulation for both approach

2. if using reqMktData there is no timestamp for each tick, if using reqRealtimeBar, 5 seconds might not be enough during spikes