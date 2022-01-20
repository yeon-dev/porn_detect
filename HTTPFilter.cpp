#include "HTTPFilter.h"

void
hexdump(
	__in BYTE* p,
	__in DWORD len)
{
	DWORD address = 0;
	DWORD row = 0;
	DWORD nread = 0;

	std::cout << std::hex << std::setfill('0');
	while (1) {
		// Show the address
		std::cout << std::setw(8) << address;

		if (address >= len) break;
		nread = ((len - address) > 16) ? 16 : (len - address);

		// Show the hex codes
		for (DWORD i = 0; i < 16; i++)
		{
			if (i % 8 == 0) std::cout << ' ';
			if (i < nread)
				std::cout << ' ' << std::setw(2) << (DWORD)p[16 * row + i];
			else
				std::cout << "   ";
		}

		// Show printable characters
		std::cout << "  ";
		for (DWORD i = 0; i < nread; i++)
		{
			if (p[16 * row + i] < 32) std::cout << '.';
			else std::cout << p[16 * row + i];
		}

		std::cout << "\n";
		address += 16;
		row++;
	}
	std::cout << std::dec;
}

HTTPFilter::HTTPFilter() : all_devices(nullptr), selected_device(nullptr) { }
HTTPFilter::~HTTPFilter() {
	if (this->all_devices != nullptr) {
		try {
			pcap_freealldevs(this->all_devices);
		}
		catch (const std::exception& ex) {
			fprintf(stderr, "Exception occured with standard exception\n");
		}
		catch (...)  {
			fprintf(stderr, "Exception occured with unknown exception\n");
		}
	}
}

vector<double> HTTPFilter::capture_traffic(u_int timeout, string adapter) {
	vector<double> statistic_result;
	
	// main logic here
	pcap_if_t* device_ptr = nullptr;
	unordered_map<string, pcap_if_t*> devices = this->get_adapter_list();

	for (auto& iterate : devices) {
		cout << iterate.first << endl;
		if (iterate.first.find(adapter) != string::npos) {
			device_ptr = iterate.second;
			break;
		}
	}
	if (device_ptr == nullptr) {
		fprintf(stderr, "There is no adapter in device list, return zero size result\n");
		return statistic_result;
	}
	// traffic capture start here
	this->selected_device = device_ptr;
	vector<PCOLUMN_INFO> uri_rows = get_http_traffics();
	return statistic_result;
}

vector<PCOLUMN_INFO> HTTPFilter::get_http_traffics() {
	int res = 0;
	pcap_t* handle;
	char errbuf[PCAP_ERRBUF_SIZE] = { 0 };
	struct bpf_program fcode;

	char* dev_name = this->selected_device->name;
	handle = pcap_open(dev_name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, NULL, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Unable to open adapter, %s is not supported\n", dev_name);
		return {};
	}

	// packet filtering setting
	u_int netmask = 0x0;
	if (this->selected_device->addresses != NULL)
		netmask = ((struct sockaddr_in*)(this->selected_device->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		netmask = 0xffffff;

	if (pcap_compile(handle, &fcode, "tcp", 1, netmask) < 0 || pcap_setfilter(handle, &fcode) < 0) {
		fprintf(stderr, "Unable to compile the packet filter. Check the syntax\n");
		return {};
	}

	// release device list
	try {
		pcap_freealldevs(this->all_devices);
		this->all_devices = nullptr;
		this->selected_device = nullptr;
		dev_name = nullptr;
	}
	catch (...) {
		fprintf(stderr, "Unable to free device lists\n");
		return {};
	}

	// packet capture start
	struct pcap_pkthdr* header;
	const u_char* pkt_data;

	while ((res = pcap_next_ex(handle, &header, &pkt_data)) >= 0) {
		if (res == 0) {
			// Timeout elapsed
			continue;
		}
		Ethernet* eth_header = (Ethernet*)pkt_data;
		IPHeader* ip_header = (IPHeader*)(pkt_data + sizeof(Ethernet));
		TCPHeader* tcp_header = (TCPHeader*)((u_char *)ip_header + (ip_header->length & 0xf) * 4);
		char* pkt_body = (char*)((u_char*)tcp_header + sizeof(TCPHeader));

		// printEthernet(eth_header);
		// printIPHeader(ip_header);
		// printTCPHeader(tcp_header);
		if (this->check_http_method(pkt_body)) {
			// cout << pkt_body << endl;
			// parsing http header here
			vector<string> parsedHeaders = this->parse_http_header(pkt_body);
			for (string& header : parsedHeaders) {
				size_t position;
				// cout << header << endl;
				if ((position = header.find("Host: ")) != string::npos && position == 0) {
					header.erase(0, position + 6);
					cout << header << endl;
				}
			}
			getchar();
		}
	}
}

bool HTTPFilter::check_http_method(const char* pkt_body) const {
	string methods[] = {"GET", "POST"};
	// HEAD나 PUT, DELETE 등은 일반적인 상황에서 사용자가 웹 사이트에 접속하기 위해서 요청되는 쿼리가 아님

	for (string& method : methods) {
		if (string(pkt_body).find(method) != string::npos) {
			return true;
		}
	}
	return false;
}

vector<string> HTTPFilter::parse_http_header(const char* pkt_body) {
	string packet_body(pkt_body);
	vector<string> splited;

	size_t pos = 0;
	while ((pos = packet_body.find("\x0d\x0a")) != string::npos) {
		// cout << packet_body.substr(0, pos) << endl;
		splited.push_back(packet_body.substr(0, pos));
		packet_body.erase(0, pos + 2);
	}
	return splited;
}

unordered_map<string, string> HTTPFilter::get_adapters_address() {
	return {};
}

void HTTPFilter::printEthernet(const Ethernet* pkt_data) const {
	cout << "******************************" << endl;
	int i = 0;
	cout << "Source MAC : ";
	for (i = 0; i < 6; i++) {
		cout << hex << (int)pkt_data->src_mac[i] << ':';
	}
	cout << endl;
	cout << "Destination MAC : ";
	for (i = 0; i < 6; i++) {
		cout << hex << (int)pkt_data->dest_mac[i] << ':';
	}
	cout << endl;
	cout << "******************************" << endl;
}

void HTTPFilter::printIPHeader(const IPHeader* pkt_data) const {
	cout << "******************************" << endl;
	u_int version = (int)pkt_data->version >> 4;
	u_int header_length = (int)(pkt_data->length & 0xf) * 4; // IPv4 version size

	u_int ttl = (u_int)pkt_data->ttl;
	u_int protocol = (u_int)pkt_data->protocol;

	u_char* source = (u_char*)&pkt_data->source;
	u_char* destination = (u_char*)&pkt_data->destination;

	int i;

	cout << dec;
	cout << "Version : " << version << endl;
	cout << "Header Size : " << header_length << endl;
	cout << "Time To Live : " << ttl << endl;
	cout << "Protocol : " << (protocol == 6 ? "TCP" : "Not TCP") << endl;
	cout << "Source IP : ";

	for (i = 0; i < 4; i++) {
		cout << (u_int)source[i] << '.';
	}
	cout << endl;
	cout << "Destination IP : ";
	for (i = 0; i < 4; i++) {
		cout << (u_int)destination[i] << '.';
	}
	cout << endl;
	cout << "******************************" << endl;
}

void HTTPFilter::printTCPHeader(const TCPHeader* pkt_data) const {
	u_short srcport = pkt_data->srcport;
	u_short dstport = pkt_data->dstport;

	cout << "******************************" << endl;
	cout << "Source Port : " << srcport << endl;
	cout << "Destination Port : " << dstport << endl;
	cout << "******************************" << endl;
}

void HTTPFilter::write_to_file(vector<double>& statistics) {
	// pass
}

unordered_map<string, pcap_if_t*> HTTPFilter::get_adapter_list() {
	char errbuf[PCAP_ERRBUF_SIZE] = { 0 };

	pcap_if_t* alldevs;
	pcap_if_t* d;

	unordered_map<string, pcap_if_t*> maps;

	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1) {
		fprintf(stderr, "Error in pcap_findalldevs_ex: %s\n", errbuf);
		exit(1);
	}

	this->all_devices = alldevs;
	for (d = alldevs; d; d = d->next) {
		// cout << d->description << endl;
		if (maps.find(d->description) == maps.end()) {
			maps.insert({ string(d->description), d });
		}
	}
	return maps;
}

vector<string> HTTPFilter::get_advanced_info(const pcap_if_t* target) {
	if (target == nullptr) {
		if (this->selected_device != nullptr) {
			// if selected_device is not nullptr
			target = this->selected_device;
		}
		else {
			return {};
		}
	}
	vector<string> result;

	for (pcap_addr_t* addr = target->addresses; addr; addr = addr->next) {
		switch (addr->addr->sa_family) {
		case AF_INET:
			if (addr->addr) {
			}
			break;
		case AF_INET6:
			break;
		default:
			result.push_back("Unknown");
			break;
		}
	}
}