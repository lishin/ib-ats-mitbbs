### I use two flags ###

1. HAS\_POSITION which is private member of client class as I want to compare different setting at the same time by using multiple process. current position is indistinguishable if it's not designed as instant specific.

2. m\_modified which is also a private member due to similar reason.

### logic flow: ###

1. initially, HAS\_POSITION = false, m\_modified = false.

2. order filled on buy/sell signal, HAS\_POSITION changed to true immediately after submitting MKT parent order, m\_modified changed to false.

3. parent MKT order filled message received by OrderStatus update, modify stop loss price or profit taking price and change m\_modified to true.

4. if child order filled or trailing order filled(since it's not an attached child order, I use negative order number to identify trailing orders), change HAS\_POSITION to false