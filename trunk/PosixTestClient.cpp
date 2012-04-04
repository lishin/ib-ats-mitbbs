#include "PosixTestClient.h"
#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include <vector>
#include <algorithm>
//#include <functional>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
using namespace std;

ofstream logfile0, logfile1, logfile2, logfile3;

bool BIDIRECTION = false;
// use MKT ENTRY OR STP ENTRY
bool USEMKTENTRY = true;
// trailing order could only be attach to stop-limit and limit order, 
// but there is no limitation in regular trading hours -- this is wrong
// alternative way is to submit a limit order with limit price greater than ask for buy, (less than bid for short sell
//bool USETRAIL = false;
bool GOLONG = false;
bool GOSHORT = false;

// reqHistorical only accept "YYYYMMDD HH:MM:SS"
char curTime[17] = "";
char curDate[8] = "";

const int totalQuantity = 1000000;
const int PING_DEADLINE = 10; // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

const double sl = 0.00010;
const double tp = 0.00010;
double trail = 0.0010;
const double tick = 0.0001;

const double initial_min = 1.0000;
const double initial_max = 2.0000;
vector<double> open, low, high, last;
vector<double> ema10, ema20, ema50;
int cnt = 0;

//const static int ticks = 50;
double bid, ask, h, l;

const size_t NUM_OF_BARS = 0;
// should I assign default value here?
int NUM_OF_TICKS = 10;
double avgPrice = 0.0;

///////////////////////////////////////////////////////////
// member funcs
PosixTestClient::PosixTestClient(int clientId, bool tflag=false)
	: m_pClient(new EPosixClientSocket(this))
    , m_clientId(clientId)
	, m_state(ST_CONNECT)
    //better to set up initial value to be false
    , m_noposition(false)
    , m_modified(false)
	, m_sleepDeadline(0)
    , m_contract(Contract())
	, m_orderId(0)
    , HAS_POSITION(0)
    , USETRAIL(tflag)
{
    m_contract.symbol = "EUR";
    m_contract.secType = "CASH";
    m_contract.exchange = "IDEALPRO";
    m_contract.currency = "USD";
}

PosixTestClient::~PosixTestClient()
{
}

bool PosixTestClient::connect(const char *host, unsigned int port)
{
	// trying to connect
	printf( "Connecting to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, m_clientId);

	bool bRes = m_pClient->eConnect( host, port, m_clientId);

	if (bRes) {
		printf( "Connected to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, m_clientId);
	}
	else
		printf( "Cannot connect to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, m_clientId);

	return bRes;
}

void PosixTestClient::disconnect() const
{
	m_pClient->eDisconnect();
    if(logfile1.is_open()){
        logfile1.close();
    }

    if(logfile2.is_open()){
        logfile2.close();
    }

    if(logfile3.is_open()){
        logfile3.close();
    }

	printf ( "Disconnected\n");
}

bool PosixTestClient::isConnected() const
{
	return m_pClient->isConnected();
}

void PosixTestClient::processMessages()
{
	fd_set readSet, writeSet, errorSet;

	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	time_t now = time(NULL);

	switch (m_state) {
        case ST_CHECK:
            initialCheck();
            break;
	}

	if( m_sleepDeadline > 0) {
		// initialize timeout with m_sleepDeadline - now
		tval.tv_sec = m_sleepDeadline - now;
	}

	if( m_pClient->fd() >= 0 ) {

		FD_ZERO( &readSet);
		errorSet = writeSet = readSet;

		FD_SET( m_pClient->fd(), &readSet);

		if( !m_pClient->isOutBufferEmpty())
			FD_SET( m_pClient->fd(), &writeSet);

		FD_CLR( m_pClient->fd(), &errorSet);

		int ret = select( m_pClient->fd() + 1, &readSet, &writeSet, &errorSet, &tval);

		if( ret == 0) { // timeout
			return;
		}

		if( ret < 0) {	// error
			disconnect();
			return;
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &errorSet)) {
			// error on socket
			m_pClient->onError();
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &writeSet)) {
			// socket is ready for writing
			m_pClient->onSend();
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &readSet)) {
			// socket is ready for reading
			m_pClient->onReceive();
		}
	}
}

void PosixTestClient::reqCurrentTime()
{
    //printf("Requesting Current Time\n");

	// set ping deadline to "now + n seconds"
    //m_sleepDeadline = time( NULL) + PING_DEADLINE;

    //m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}

void PosixTestClient::placeOrder(const IBString &action)
{
    return;
}

void PosixTestClient::cancelOrder()
{
	printf("Client %d: Cancelling Order %ld\n", m_clientId, m_orderId);

	m_state = ST_CANCELORDER_ACK;

	m_pClient->cancelOrder(m_orderId);
    printf("Client %d: cancel order function called!", m_clientId);
}

// possible duplicate messages
void PosixTestClient::orderStatus(OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
    Order order_stoploss, order_profittaking, order_trailing;
    //OrderId stoplossId, profittakingId;
    printf("Client %d: Order: id=%ld, parentId=%d, status=%s, filled = %d\n", m_clientId, orderId, parentId, status.c_str(), filled);
    
    // what if Filled message is after attached child order status message?


    if(!USETRAIL){
        if(parentId!=0 && status=="Filled"){
            HAS_POSITION = 0;
            avgPrice = 0.0;
        }
        if(parentId==0 && status=="Filled"){
            avgPrice = avgFillPrice;
        }
    }
    else {
        if(parentId==0 && status=="Filled" && orderId<0){
            HAS_POSITION = 0;
        }
    }

    // STP order is PreSummitted, LMT order is Submitted
    // modified means 
    // 1. modify stop price and profit taking price for child orders
    // 2. attach a trailing stop order for trailing stop
    if(!m_modified){
        if(!USETRAIL){
            if(parentId!=0 && status != "Filled"){
            //cout << "Average filled price is :" << avgPrice << endl;
            // assume we don't trade negative quote products here
                if(avgPrice > 0.0){
                    if(!m_modified && USEMKTENTRY){

                        // try to change the stop order into trailing stop order 
                        if(parentId%2==0){
                            order_stoploss.action = "SELL";
                            order_profittaking.action = "SELL";
                            order_stoploss.auxPrice = avgPrice - sl;
                            order_profittaking.lmtPrice = avgPrice + tp;
                        }

                        if(parentId%2!=0){
                            order_stoploss.action = "BUY";
                            order_profittaking.action = "BUY";
                            order_stoploss.auxPrice = avgPrice + sl;
                            order_profittaking.lmtPrice = avgPrice - tp;
                        }
                    
                        order_stoploss.orderType = "STP";
                        order_profittaking.orderType = "LMT";
                        order_stoploss.totalQuantity = totalQuantity;
                        order_profittaking.totalQuantity = totalQuantity;

                        m_pClient->placeOrder(parentId + 1, m_contract, order_stoploss);
                        m_pClient->placeOrder(parentId + 2, m_contract, order_profittaking);
                        m_modified = true;
                    }
                }
            }
        }

        else {
            order_trailing.totalQuantity = totalQuantity;
            order_trailing.orderType = "TRAIL";
            if(orderId%2==0){
                order_trailing.action = "SELL";
                order_trailing.auxPrice = trail;
            }

            if(orderId%2!=0){
                order_trailing.action = "BUY";
                order_trailing.auxPrice = trail; 
            }
            // use negative orderId to distinguish MKT order and TRAIL order
            m_pClient->placeOrder(m_orderId, m_contract, order_trailing);
            m_orderId = m_orderId + 1; 
            // here modified means protected by trailing stop order
            m_modified = true;
        }
    }
}

void PosixTestClient::barRecord(){
    m_pClient->reqCurrentTime();
    // how to check if the message has been returned?
    //while(1){
    //if(string(curTime) != "") break;
    //}

    if(m_contract.secType == "CASH"){
        m_pClient->reqHistoricalData(5, m_contract, curTime, "2 D", "5 mins", "MIDPOINT", 0, 1);
    }
    
    else {
        m_pClient->reqHistoricalData(5, m_contract, curTime, "2 D", "5 mins", "TRADES", 0, 1);
    }
}

void PosixTestClient::tickRecord(){
    // no last/trade value for currency in tick mode
    m_pClient->reqCurrentTime();
    m_pClient->reqMktData(1, m_contract, "225", false);
    /*  
    Contract contract;
    contract.symbol = "ES";
    contract.secType = "FUT";
    contract.exchange = "GLOBEX";
    contract.currency = "USD";
    contract.expiry = "20120615";
    m_pClient->reqMktData(2, contract, "225", false);
    */
}
 
// this is called automatically on connection, should mannually maintain valid orderId
void PosixTestClient::nextValidId(OrderId orderId)
{
	m_orderId = orderId;
}

void PosixTestClient::currentTime(long time)
{
    time_t t = (time_t)time;
    if(strftime(curTime, 100, "%Y%m%d %T", localtime(&t)) && strftime(curDate, 100, "%Y%m%d", localtime(&t))){
        cout << "Client " << m_clientId << ": The current date/time is: " << curTime << endl;
        //cout << "The current date is: " << curDate << endl;
        //logfile1.open(("log/"+string(curDate)+"_1.txt").c_str(), ios::out|ios::app);
        //logfile2.open(("log/"+string(curDate)+"_2.txt").c_str(), ios::out|ios::app);
        stringstream s;
        string ss;
        s << m_clientId;
        ss = s.str();

        logfile1.open(("log/client"+ss+"_"+string(curDate)+"_tick.txt").c_str());
        logfile2.open(("log/client"+ss+"_"+string(curDate)+"_bar.txt").c_str());
        logfile3.open(("log/client"+ss+"_"+string(curDate)+"_trades.txt").c_str());
    }

    /* for 5 minutes bar
    if((curTime[13] == '5' or curTime[13] == '0') && curTime[15] == '0' && curTime[16] == '0'){
        cout << "The current data/time is: " << curTime << endl;
    }
    */
}

void PosixTestClient::error(const int id, const int errorCode, const IBString errorString)
{
    printf("Error id=%d, errorCode=%d, msg=%s\n", id, errorCode, errorString.c_str());

	if( id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
		disconnect();
}

void PosixTestClient::tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute) {
    double s_sp, l_sp;
    time_t t;
    Order s_order_parent, s_order_stoploss, s_order_profittaking;
    Order l_order_parent, l_order_stoploss, l_order_profittaking;

    stringstream oca;
    size_t sz;
    OrderId parentId;

    s_order_parent.totalQuantity = totalQuantity;
    l_order_parent.totalQuantity = totalQuantity;

    if(field == 1){
        cout << fixed;
        // assume no exception for now() function
        t = time(NULL);
        strftime(curTime, 100, "%Y%m%d %T", localtime(&t));
        //printf("Tick %d:\tTickerId = %d, TickType = %d, price = %f\n", cnt, static_cast<int>(tickerId), field, price);
        //cout << "Client " << m_clientId << ":" << curTime << "," << price << "," << cnt << "\n";
        logfile1 << curTime << "," << price << "," << cnt << endl;
        bid = price;
        ++cnt;
        if(cnt == 1){
            open.push_back(bid);
            h = bid;
            l = bid;
        }

        else {
            if(bid > h) h = bid;
            if(bid < l) l = bid;
        }

        if(cnt == NUM_OF_TICKS){
            low.push_back(l);
            high.push_back(h);
            last.push_back(bid);
            cnt = 0;
            //logfile1 << "Open\tLow\tHigh\tLast\n";
            t = time(NULL);
            strftime(curTime, 100, "%Y%m%d %T", localtime(&t));
            cout << "Client " << m_clientId << ":" << curTime << "," << open.back() << "," << low.back() << "," << high.back() << "," << last.back();
            logfile2 << curTime << "," << setprecision(5) << open.back() << "," << low.back() << "," << high.back() << "," << last.back();
            sz = open.size();
            
            if(sz >= 10){
                if(sz == 10)
                    ema10.push_back(accumulate(last.begin(), last.end(), 0.0)/10);
                else
                    ema10.push_back(ema10.back() + (2.0/11.0)*(last.back()-ema10.back()));
                cout << "|" << setprecision(5) << ema10.back();
                logfile2 << "\t" << ema10.back();
            }

            if(sz >= 20){
                if(sz == 20)
                    ema20.push_back(accumulate(last.begin(), last.end(), 0.0)/20);
                else
                    ema20.push_back(ema20.back() + (2.0/21.0)*(last.back()-ema20.back()));
                cout << "|" << setprecision(5) << ema20.back();
                logfile2 << "," << ema20.back();
            }

            cout << "\n";
            logfile2 << endl;

            /* ================================================================================================================= */
            // this part is the strategy
            /* 
            if(sz > NUM_OF_BARS && !HAS_POSITION){
                if(low[sz-1]>=low[sz-2] && high[sz-1]>high[sz-2]) GOLONG = true;
                if(low[sz-1]<low[sz-2] && high[sz-1]<=high[sz-2]) GOSHORT = true;
            }
            */
            
            // ema10 empty should be redudant condition
            if(ema20.size()>2 && ema10.size()>2 && !HAS_POSITION){
                if(ema10[ema10.size()-2] <= ema20[ema20.size()-2] && ema10[ema10.size()-1] > ema20[ema20.size()-1]) GOLONG = true;
                if(ema10[ema10.size()-2] >= ema20[ema20.size()-2] && ema10[ema10.size()-1] < ema20[ema20.size()-1]) GOSHORT = true;
            }

            //GOSHORT = true;
            //if(!ema10.empty() && !HAS_POSITION){
            //if(ema10.back() < price) GOLONG = true;
            //if(ema10.back() > price) GOSHORT = true;
            //}

            /* ================================================================================================================= */
            // place orders
            if(!HAS_POSITION){
                if(open.size()>=NUM_OF_BARS){
                //three bar
                    s_sp = *min_element(low.end()-NUM_OF_BARS, low.end()) - tick;
                    
                    s_order_parent.totalQuantity = totalQuantity;
                    s_order_parent.action = "SELL";
                    s_order_stoploss.action = "BUY";
                    s_order_profittaking.action = "BUY";
                    
                    if(USEMKTENTRY){
                        s_order_parent.orderType = "MKT";
                        s_order_stoploss.auxPrice = initial_max;
                        s_order_profittaking.lmtPrice = initial_min;
                    }
                    else{
                        s_order_parent.orderType = "STP";
                        s_order_parent.auxPrice = s_sp;
                        s_order_stoploss.auxPrice = s_sp + sl;
                        s_order_profittaking.lmtPrice = s_sp - tp;
                    }

                    //s_order_stoploss.orderType = "STP";
                    s_order_stoploss.orderType = "STP";
                    s_order_profittaking.orderType = "LMT";
                    s_order_stoploss.totalQuantity = s_order_parent.totalQuantity;
                    s_order_profittaking.totalQuantity =  s_order_parent.totalQuantity;

                    /* all or none is not supported in forex
                    s_order_parent.allOrNone = true;
                    s_order_stoploss.allOrNone = true;
                    s_order_profittaking.allOrNone = true;
                    */

                    s_order_parent.transmit = USETRAIL?true:false;
                    s_order_stoploss.transmit = false;
                    //s_order_profittaking.transmit = false;


                    // assume one tick spread, so stop price is equal to high_of_bid + 2 ticks
                    l_sp = *max_element(high.end()-NUM_OF_BARS, high.end()) + 2*tick;
                    
                    l_order_parent.totalQuantity = totalQuantity;
                    l_order_parent.action = "BUY";
                    l_order_stoploss.action = "SELL";
                    l_order_profittaking.action = "SELL";
                    
                    if(USEMKTENTRY){
                        l_order_parent.orderType = "MKT";
                        l_order_stoploss.auxPrice = initial_min;
                        l_order_profittaking.lmtPrice = initial_max;
                    }
                    else{
                        l_order_parent.orderType = "STP";
                        l_order_parent.auxPrice = l_sp;
                        l_order_stoploss.auxPrice = l_sp - sl;
                        l_order_profittaking.lmtPrice = l_sp + tp;
                    }

                    //l_order_stoploss.orderType = "STP";
                    l_order_stoploss.orderType = "STP";
                    l_order_profittaking.orderType = "LMT";
                    l_order_stoploss.totalQuantity = l_order_parent.totalQuantity;
                    l_order_profittaking.totalQuantity =  l_order_parent.totalQuantity;
                    

                    /* all or none is not supported in forex
                    l_order_parent.allOrNone = true;
                    l_order_stoploss.allOrNone = true;
                    l_order_profittaking.allOrNone = true;
                    */

                    l_order_parent.transmit = USETRAIL?true:false;
                    l_order_stoploss.transmit = false;
                    //l_order_profittaking.transmit = false;

                    if(BIDIRECTION){
                        oca << m_orderId;
                        s_order_parent.ocaGroup = oca.str();
                        l_order_parent.ocaGroup = oca.str();
                    }

                    if(GOLONG){
                        if(m_orderId%2!=0){
                            parentId = m_orderId + 1;
                            }
                        else{
                            parentId = m_orderId;
                        }

                        l_order_stoploss.parentId = parentId;
                        l_order_profittaking.parentId = parentId;
                        cout << "Client " << m_clientId << ": Placing a MKT long order, orderId " << parentId << ".\n";
                        m_pClient->placeOrder(parentId, m_contract, l_order_parent);	
                        if(!USETRAIL){
                            m_pClient->placeOrder(parentId + 1, m_contract, l_order_stoploss);
                            m_pClient->placeOrder(parentId + 2, m_contract, l_order_profittaking);
                        }
                        GOLONG = false;
                        HAS_POSITION = 1;
                        m_modified = false;
                        m_orderId = parentId + (USETRAIL?1:3);
                        cout << "Client " << m_clientId << ": New orderId is " << m_orderId << ".\n";
                    }
                    
                    if(GOSHORT){
                        if(m_orderId%2!=0){
                            parentId = m_orderId;
                            }
                        else{
                            parentId = m_orderId + 1;
                        }

                        s_order_stoploss.parentId = parentId;
                        s_order_profittaking.parentId = parentId;
                        
                        cout << "Client " << m_clientId << ": Placing a MKT short order, orderId " << parentId << ".\n";
                        m_pClient->placeOrder(parentId, m_contract, s_order_parent);	
                        if(!USETRAIL){
                            m_pClient->placeOrder(parentId + 1, m_contract, s_order_stoploss);
                            m_pClient->placeOrder(parentId + 2, m_contract, s_order_profittaking);
                        }
                        GOSHORT = false;
                        HAS_POSITION = -1;
                        m_modified = false;
                        m_orderId = parentId + (USETRAIL?1:3);
                        cout << "Client " << m_clientId << ": New orderId is " << m_orderId << ".\n";
                    }
                } 
            }
            else {
                //cout << "Client " << m_clientId << ": There are positions currently!" << endl;
            }
        }

        //if(HAS_POSITION>0 && price-avgPrice>sl){}
    }

    if(field == 2){
        ask = price;
        //printf("TickerId = %d, TickType = %d, price = %f\n", static_cast<int>(tickerId), field, price);
    }
}

void PosixTestClient::tickSize( TickerId tickerId, TickType field, int size) {}
void PosixTestClient::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
											 double optPrice, double pvDividend,
											 double gamma, double vega, double theta, double undPrice) {}
void PosixTestClient::tickGeneric(TickerId tickerId, TickType tickType, double value) {}
void PosixTestClient::tickString(TickerId tickerId, TickType tickType, const IBString& value) {}
void PosixTestClient::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
							   double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
void PosixTestClient::openOrder(OrderId orderId, const Contract &contract, const Order &order, const OrderState& ostate) {
    /*  
    if(contract == m_contract){
        //printf("Already has position!\n");
        printf("OrderId is %d, OrderState is %s\n", static_cast<int>(orderId), ostate.status.c_str());
        //cout << "OrderId is " << orderId << "OrderState is " << ostate << endl;
        //m_noposition = false;
        HAS_POSITION = true;
    }
    */
}

void PosixTestClient::openOrderEnd() {}
void PosixTestClient::winError( const IBString &str, int lastError) {}
void PosixTestClient::connectionClosed() {}
void PosixTestClient::updateAccountValue(const IBString& key, const IBString& val,
										  const IBString& currency, const IBString& accountName) {}
void PosixTestClient::updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName){
    //printf("testing updatePortfolio\n");
    //if(contract == m_contract && position!=0){
    //printf("Already has position!");
    //m_noposition = false;
    //}
}

void PosixTestClient::updateAccountTime(const IBString& timeStamp) {}
void PosixTestClient::accountDownloadEnd(const IBString& accountName) {}
void PosixTestClient::contractDetails( int reqId, const ContractDetails& contractDetails) {}
void PosixTestClient::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void PosixTestClient::contractDetailsEnd( int reqId) {}
void PosixTestClient::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void PosixTestClient::execDetailsEnd( int reqId) {}

void PosixTestClient::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {}
void PosixTestClient::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {}
void PosixTestClient::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void PosixTestClient::managedAccounts( const IBString& accountsList) {}
void PosixTestClient::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void PosixTestClient::historicalData(TickerId reqId, const IBString& date, double open, double high,
									  double low, double close, int volume, int barCount, double WAP, int hasGaps) {
    if(m_contract.secType == "CASH"){
        cout << reqId << ": " << date << "\t" << open << ", " << high << ", " << low << ", " << close << endl;
    }
    else {
        cout << reqId << ": " << date << "\t" << open << ", " << high << ", " << low << ", " << close << ", " << volume << ", " << barCount << ", " << WAP << ", " << hasGaps << endl; 
    }
}
void PosixTestClient::scannerParameters(const IBString &xml) {}
void PosixTestClient::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	   const IBString &distance, const IBString &benchmark, const IBString &projection,
	   const IBString &legsStr) {}
void PosixTestClient::scannerDataEnd(int reqId) {}
void PosixTestClient::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
								   long volume, double wap, int count) {}
void PosixTestClient::fundamentalData(TickerId reqId, const IBString& data) {}
void PosixTestClient::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
void PosixTestClient::tickSnapshotEnd(int reqId) {}
void PosixTestClient::marketDataType(TickerId reqId, int marketDataType) {}

void PosixTestClient::initialCheck(){
    m_pClient->reqAllOpenOrders();
}
