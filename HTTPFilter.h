#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>

// #include <Windows.h>
#include <WinSock2.h>
#include <pcap.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

typedef struct Ethernet {
	u_char dest_mac[6];
	u_char src_mac[6];
	u_short type;			// IPv6 or IPv6
};

typedef struct IPHeader {
	union {
		u_char version;			// IPv4 or IPv6
		u_char length;			// IP Header Length
	};
	u_char diff_service_field;
	u_short total_length;
	u_short identification;
	u_short flags;
	u_char ttl;
	u_char protocol;
	u_short checksum;
	u_int source;
	u_int destination;
};

typedef struct TCPHeader {
	u_short srcport;
	u_short dstport;
	u_int seq;
	u_int ack_num;
	u_short flags;
	u_short window_size;
	u_short checksum;
	u_short urgent_pointer;
};

typedef struct ColumnInfo {
	u_int counter;
	u_int all_rows;
	string uri;
} COLUMN_INFO, *PCOLUMN_INFO;

class HTTPFilter
{
public:
	HTTPFilter();
	~HTTPFilter();
public:
	vector<double> capture_traffic(u_int timeout, string adapter);
	void write_to_file(vector<double>& statistics);
private:
	unordered_map<string, pcap_if_t*> get_adapter_list();
	vector<string> get_advanced_info(const pcap_if_t* target);
private:
	vector<PCOLUMN_INFO> get_http_traffics();
	unordered_map<string, string> get_adapters_address();
	bool check_http_method(const char* pkt_body) const;
	vector<string> parse_http_header(const char* pkt_body);
public:
	void printEthernet(const Ethernet* pkt_data) const;
	void printIPHeader(const IPHeader* pkt_data) const;
	void printTCPHeader(const TCPHeader* pkt_data) const;
private:
	pcap_if_t* all_devices;
	pcap_if_t* selected_device;
};

