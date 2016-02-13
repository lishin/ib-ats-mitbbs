for mitbbs, targeting forex scalping,

could be easily changed to futures and stocks, options are **VERY** different

This will save you time of writing an automatic trading system in IB, but I would **NOT** put my strategy in the file(actually, for forex scalping, I don't think I need any strategy, coin tossing is profitable enough). There is a 10-20 EMA cross demo currently.

I use tick chart which tracks BID(ASK should also work), if you trade futures, please use TRADES in tickPrice ewrapper function. **DO** modify sl/tp/trailing price for different product.

Usage:

to compile: make

to use: eu.exe -c 0 -t 10 -n 20

-c: clientId, IB API allows up to 9 clients at the same time.

-t: if this options is turned on, it means 10 pips for forex, remember I use 0.0001 factor in tickPrice function; otherwise, two child orders will be attached.

-n: 20 number of ticks per candle

I made minimum changes on the IB POSIX C++ example. tried multithreading for multiple clients(different client uses different sl/tp/trailing settings for comparing backtesting result).  I switched back to multiprocess.


Money management is currently **NOT** implemented.

Feel free to ask me questions.




