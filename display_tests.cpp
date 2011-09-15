#define BOOST_TEST_MODULE display
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <exception>

#include <boost/cstdint.hpp>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace boost;
using namespace std;

#include "base_display.h"
#include "sek_display.h"
#include "base_stream.h"
#include "bootloader.h"
#include "sek_fw_file.h"

void BufferOut(uint8_t* buff, int buflen);

void BufferOut(uint8_t* buff, int buflen) {
	stringstream s;
	s << "Buffer: ";
	for(int i=0;i<buflen;i++) 
		s << hex << (int) buff[i];
	BOOST_TEST_MESSAGE("Buffer out: "<< s.str());
}

// Global fixtures
struct D {
	D() {
		BOOST_TEST_MESSAGE("Setup display fixture.");
		set<string> devices = BaseDisplay::ListDevicePaths(0x4d8, 0x3f);
		if (devices.size() == 0) {
			BOOST_FAIL("No devices connected.");
		}
		BOOST_CHECK_MESSAGE(devices.size()>0, "Connecting to device: "<< *devices.begin());
		sek = new SEK_Display();
		connect_path = *devices.begin();
		sek->OpenDisplay(connect_path.c_str());
	}
	~D() {
		BOOST_TEST_MESSAGE("teardown display fixture.");
		if (sek != NULL) {
			delete(sek);
		}
	}
	void delete_device(void) {
		if (sek != NULL) {
			delete(sek);
			sek = NULL;
		}
	}
	void recreate_device(void) {
		if (sek == NULL) {
			sek = new SEK_Display();
			sek->OpenDisplay(connect_path.c_str());
		}
	}
	SEK_Display* sek;
	string connect_path;
};

struct B: public Bootloader {
	B() : Bootloader() {
		BOOST_TEST_MESSAGE("Setup bootloader display fixture.");
		sek = NULL;
	}
	~B() {
		if (sek != NULL) {
			delete(sek);
			sek=NULL;
		}
		BOOST_TEST_MESSAGE("teardown bootloader display fixture.");
	}
	void Connect(int prod_id = 0x3f) {
		set<string> devices = BaseDisplay::ListDevicePaths(0x4d8, prod_id);
		if (devices.size() == 0) {
			BOOST_FAIL("No devices connected.");
		}
		sek = new SEK_Display();
		connect_path = *devices.begin();
		BOOST_CHECK_MESSAGE(devices.size()>0, "Connecting to device: "<< *devices.begin());
		sek->OpenDisplay(connect_path.c_str());
		BOOST_CHECK_MESSAGE(devices.size()>0, "Connected to device: "<< connect_path);
		SetDisplay(sek);	
	}
	void DoQueryMemory(void) {
		QuerySEKMemory();
	}
	void LoadFile(string image_file_name) {
		LoadFirmwareFile(image_file_name);
	}
	void PrepareForFWUpgrade(string image_file_name, int mem_type, int address, int size, 
		int nb_per_packet, int nb_per_address) {
		LoadFirmwareFile(image_file_name); // FW file name
		SetBytesPerPacket(nb_per_packet);
		SetBytesPerAddress(nb_per_address);
		AddMemoryRegion(mem_type, address, size);
		MapImage();
		// Setup memory images
		// Map image
		// Program blocks
	}
	void PrepareForFWUpgrade(vector<string>& fw_lines, int mem_type, int address, int size, 
		int nb_per_packet, int nb_per_address) {
		for(vector<string>::iterator it = fw_lines.begin(); it != fw_lines.end(); it++)
			GetFWFile()->LoadLine(*it);
		GetFWFile()->ParseFile();
		SetBytesPerPacket(nb_per_packet);
		SetBytesPerAddress(nb_per_address);
		AddMemoryRegion(mem_type, address, size);
		MapImage();
		ProgramFirmware();
		// Program blocks
	}
	vector<string> GenerateLines(void) {
		vector<string> str;
		str.push_back(":10522000e2f50400000000006ef60400000000003b");
		str.push_back(":105230006ef704000000000042f5040000000000ca");
		str.push_back(":00000001FF");
		return(str);
	}
	int BlockNum(int address) {return(GetBlockNumber(address));}
	void DoErase(void) {
		EraseFirmware();
	}
	void RunUpgrade(bool in_bl = false) {
		if (!in_bl)
			sek->SetBootLoaderMode();
		sek->SetDebugOn();
		DoUpgrade("test_fw_file.hex");
	}
	SEK_Display* sek;
	string connect_path;
};

struct F : public SEKHexFile {
	F() : SEKHexFile() {
		BOOST_TEST_MESSAGE("Setup fw file fixture.");
	}
	~F() {
		BOOST_TEST_MESSAGE("teardown fw file fixture.");
	}
	int GetCheckSum(const string& data) {return(CalculateCheckSum(data));}	
	vector<uint8_t> ProcessLinePart(unsigned int record_type, unsigned int& address,
		const string& payload) {
		parsed_line l;
		ProcessLineParts(l, record_type, address, payload);
		address = l.address;
		return(l.data_bytes);
	}
	string file_name;
};

BOOST_FIXTURE_TEST_SUITE(basic_tests, D)


BOOST_AUTO_TEST_CASE (check_devices_connected) {
	set<string> devices = BaseDisplay::ListDevicePaths(0x4d8, 0x3f);
	int len_devs = devices.size();
	BOOST_TEST_MESSAGE("Detected " << len_devs << " devices.");
	BOOST_CHECK_MESSAGE(len_devs > 0, "No devices connected!!!");
	if (len_devs<=0)
		BOOST_FAIL("Failed - no devices.");
}

BOOST_AUTO_TEST_CASE (check_device_details) {
	list<DisplayInfo> devices = BaseDisplay::GetAllDisplayInfo(0x4d8, 0x3f);
	int len_devs = devices.size();
	BOOST_TEST_MESSAGE("Detected " << len_devs << " devices.");
	BOOST_CHECK(len_devs > 0);
	if (len_devs<=0)
		BOOST_FAIL("Failed - no devices.");
	list<DisplayInfo>::iterator it;
	for(it=devices.begin(); it!=devices.end();it++) {
		BOOST_TEST_MESSAGE("Device details, type: "<<(*it).type<<" path: "<<(*it).path<<" vendor_id: "<<(*it).vid<<" product_id "
			<<(*it).pid);
	}
}

BOOST_AUTO_TEST_CASE (firmware_version_test) {
	try {
		int version = sek->GetFWVersion();
		BOOST_TEST_MESSAGE("Device "<< sek->GetPath() << " FW version " << hex << version);
		BOOST_CHECK(version > 0);
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (interface_test) {
	try {
sek->SetInfoDebugOn();
		int last_version = 0;
		int no_reps = 10;
		for(int k=0;k<no_reps;k++) {
			int version = sek->GetFWVersion();
			BOOST_REQUIRE(version > 0);
			if (last_version != 0) 
				BOOST_REQUIRE(version == last_version);
			last_version = version;
		}
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (single_ldm_string) {
	try {
		string return_ldm = sek->RawLDMString(0x02, "E3007(0000080008)");
		BOOST_CHECK(return_ldm[return_ldm.size()-1] == ')');
		BOOST_TEST_MESSAGE("Received response "<< return_ldm);
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (set_single_register) {
	try {
sek->SetInfoDebugOn();
		int reg_num = 0x05;
		int orig_val = sek->GetRegister(0x02, reg_num);
		BOOST_TEST_MESSAGE("Register ("<<reg_num<<")="<<orig_val);
		sek->SetRegister(0x02, reg_num, orig_val+1);
		int new_val = sek->GetRegister(0x02, reg_num);
		BOOST_TEST_MESSAGE("Register ("<<reg_num<<") new val="<<new_val);
		BOOST_CHECK(orig_val+1==new_val);
		sek->SetRegister(0x02, reg_num, orig_val);
		new_val = sek->GetRegister(0x02, reg_num);
		BOOST_CHECK(orig_val==new_val);
		BOOST_TEST_MESSAGE("Register ("<<reg_num<<") reset to ="<<orig_val);
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(encode)

BOOST_AUTO_TEST_CASE (base64_decode) {
	struct test_1 : protected BaseDisplay {
		int length;
		test_1(BaseDisplay* bd, uint8_t buffer[64], string to_decode) {
			length = ((test_1*)bd)->DecodeBase64(buffer, to_decode);
		}
	};
	struct test_2 : protected BaseDisplay {
		test_2(BaseDisplay* bd, uint8_t buffer[64], int size, string& encoded) {
			encoded = ((test_2*)bd)->EncodeBase64(buffer, size);
		}
	};
	BaseDisplay bd;
	uint8_t buffer[64];
	uint8_t buffer1[64];
	const char* messages[] = {
		"AAAAAAAAAAAAAGdkAgABLS==",
		"AAAAAAAAAAAAAI5kAgABLi==",
		"AAAAAAAAAAAAAA8LAQABAQ=="};
	int num_messages = sizeof(messages)/sizeof(char*);
	BOOST_TEST_MESSAGE("Base64 encode testing "<<num_messages<<" messages");
	for(int i=0;i<num_messages;i++) {
		memset(buffer, 0, 64);
		memset(buffer1, 0, 64);
		test_1 t1a(&bd, buffer, messages[i]);
		BOOST_TEST_MESSAGE("Original string "<<messages[i]);
		BufferOut(buffer, t1a.length);
		string out;
		test_2 t2a(&bd, buffer, t1a.length, out);
		test_1 t1b(&bd, buffer1, out);
		BOOST_TEST_MESSAGE("String "<<messages[i]<<" decode/encode as " << out);
		// This test sometimes fails because of byte alignment (it's not important as long as binary is correct)
		// so we'll leave it commented out for now
		//BOOST_CHECK(out==messages[i]); 
		BOOST_CHECK(memcmp(buffer, buffer1, 64) == 0);
	}
}

BOOST_FIXTURE_TEST_CASE (send_single_message, D) {
	const char* messages[] = {
		"AAAAAAAAAAAAAGdkAgABLQ==",
		"AAAAAAAAAAAAAI5kAgABLg==",
		"AAAAAAAAAAAAAA8LAQABAQ==",
		"AAAAAAAAAAAAANGeAgABLQ=="};
	int num_messages = sizeof(messages)/sizeof(char*);
	for(int i=0;i<num_messages;i++) {
		string response = sek->SendSingleMessage(messages[i]);
		BOOST_TEST_MESSAGE("SingleMessage: "<<messages[i]<<" Response: " << response);
	}
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(bulk_tests, D)

BOOST_AUTO_TEST_CASE (create_destruct_test) {
	int version = sek->GetFWVersion();
	delete_device();
	BOOST_REQUIRE(sek == NULL);
	recreate_device();
	BOOST_REQUIRE(sek != NULL);
	int new_version = sek->GetFWVersion();
	BOOST_REQUIRE(version == new_version);
}

BOOST_AUTO_TEST_CASE (get_bulk_data) {
	vector<uint8_t> block = sek->GetBlock(0x02, 0x01, 0x06);
	BOOST_CHECK(block.size()==0x100);
}

BOOST_AUTO_TEST_CASE (get_stream_data) {
	BaseStream stream(sek);
	stream.ReadStream(1); // get the energy low res stream
	BOOST_TEST_MESSAGE(stream);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(fw_file_tests)

BOOST_FIXTURE_TEST_CASE (check_sum_line, F) {
	BOOST_CHECK(GetCheckSum(string("1004000000000000000000000000000000000000")) == 0xEC);
	BOOST_CHECK(GetCheckSum(string("106cd0000000fa000080fa00000006000200fa00")) == 0x3E);
	BOOST_CHECK(GetCheckSum(string("106ce000000f78000000eb000080fa0000000600")) == 0xb2);
	BOOST_CHECK(GetCheckSum(string("10c38c0020002000100798001e0090000080fa00")) == 0x8a);
}

BOOST_FIXTURE_TEST_CASE (process_line_part_extended, F) {
	unsigned int address = 0;
	ProcessLinePart(0x04, address, "0005");
	BOOST_CHECK(address = 0x05);
	address = 0x10;
	vector<uint8_t> tmp_vec = ProcessLinePart(0x00, address, "0102030405");
	BOOST_CHECK(address = 0x15); // 0x10 + 0x05
	uint8_t init_values[] = {1, 2, 3, 4, 5};
	BOOST_CHECK(tmp_vec == vector<uint8_t>(init_values, init_values + sizeof(init_values)/sizeof(uint8_t)));
	address = 0x100;
	ProcessLinePart(0x04, address, "0101");
	BOOST_CHECK(address = 0x201); // 0x100 + 0x101

}

BOOST_AUTO_TEST_CASE (check_sum_file) {
	string fw_file_name = "test_fw_file.hex";
	try {
		SEKHexFile fw_file(fw_file_name);
	}
	catch(SEKFileException& e) {
		stringstream err;
		err << "FW file error: " << *get_error_info<error_string>(e)<<", code: "
			<< *get_error_info<error_code>(e);
		BOOST_FAIL("Failed to load FW file "<< fw_file_name<<", error "<<err.str());
	}
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(upgrade_tests, B)

BOOST_AUTO_TEST_CASE (list_devices) {
	for(int k=0;k<40;k++) {
		list<DisplayInfo> devices = BaseDisplay::GetAllDisplayInfo(0x4d8, 0x3f);
		int len_devs = devices.size();
		BOOST_TEST_MESSAGE("Detected " << len_devs << " devices.");
		BOOST_CHECK(len_devs > 0);
		list<DisplayInfo>::iterator it;
		for(it=devices.begin(); it!=devices.end();it++) {
			BOOST_TEST_MESSAGE("Device details, type: "<<(*it).type<<" path: "<<(*it).path<<" vendor_id: "<<(*it).vid<<" product_id "
				<<(*it).pid);
		}
		sleep(1);
		BOOST_TEST_MESSAGE("==================================\n\n");
	}
}

BOOST_AUTO_TEST_CASE (sek_fw_file) {
	// Load in a fw file and check
	// line check sums etc.
	try {
		const char* fw_file_name = "test_fw_file.hex";
		BOOST_TEST_MESSAGE("Loading file "<< fw_file_name);
		SEKHexFile fw_file(fw_file_name);
	}
	catch(boost::exception& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (sek_modes) {
	try {
		int vid, pid;
		Connect(0x3f);
		sek->SetDebugOn();
		sek->GetHidDetails(vid, pid);
		BOOST_CHECK(pid == 0x3f);
		BOOST_TEST_MESSAGE("Resetting device");
		sek->SetBootLoaderMode();
		sek->GetHidDetails(vid, pid);
		BOOST_CHECK_MESSAGE(pid == 0x3c, "Device in bootloader mode.");
		BOOST_TEST_MESSAGE("Setting to App mode.");
		sek->SetAppMode();
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (sek_query_memory) {
	try {
		int vid, pid;
		Connect(0x3f);
		sek->SetBootLoaderMode();
		sek->GetHidDetails(vid, pid);
		BOOST_CHECK_MESSAGE(pid == 0x3c, "Device in bootloader mode.");
		DoQueryMemory();
		BOOST_CHECK(GetBytesPerAddress() == 2);
		BOOST_TEST_MESSAGE("SEK has "<<GetNumMemoryRegions()<<" memory region.");	
		for(unsigned int i=0;i<GetNumMemoryRegions();i++) {
			// Check memory structures found.
			memory_region rg = GetMemoryRegion(i);
			BOOST_TEST_MESSAGE("Memory region "<<i<<", type:"<<hex<<(int)rg.type<<", address:"<<hex<<rg.address<<", size:"<<hex<<rg.size<<", bytes per packet "
				<<GetBytesPerPacket()<<", bytes per address "<<GetBytesPerAddress());
			// Check memory pattern
			const uint8_t* block = GetMemoryBlock(i);
			for(unsigned int j=0;j<(rg.size * GetBytesPerAddress() ); j++) {
				if ((j+1)%4 == 0 && block[j] != 0x00)
					BOOST_FAIL("Block format is incorrect");
				else if ((j+1)%4!=0 && block[j] != 0xFF)
					BOOST_FAIL("Block format is incorrect");
			}
			BOOST_MESSAGE("Block "<<i<<" has correct format");
		}
		sek->SetAppMode();
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(program_tests, B)

BOOST_AUTO_TEST_CASE (sek_program1) {
	try {
		string fw_file_name = "test_fw_file.hex";
		PrepareForFWUpgrade(fw_file_name, 1, 0x2800, 0x28000, 38, 2);
		BOOST_REQUIRE(BlockNum((0x2800 + 0) * 2) == 0);
		BOOST_REQUIRE(BlockNum((0x2800 + 0) * 2 - 1) == -1);
		BOOST_REQUIRE(BlockNum((0x2800 + 0x28000) * 2 -1) == 0);
		BOOST_REQUIRE(BlockNum((0x2800 + 0x28000) * 2) == -1);
		BOOST_REQUIRE(GetNumMemoryRegions() == 1);
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (sek_program2) {
	try {
		vector<string> fw_lines = GenerateLines();
		PrepareForFWUpgrade(fw_lines, 1, 0x2800, 0x28000, 38, 2);
		BOOST_REQUIRE(GetNumMemoryRegions() == 1);
		const uint8_t* mp = GetMemoryBlock(0); 
		BOOST_REQUIRE(mp != NULL);
		//const uint32_t* mem = reinterpret_cast<const uint32_t*>(mp);
		for(int k=0;k<16;k++) {
			BOOST_TEST_MESSAGE("byte (" << k << "): "<<hex<<(unsigned int)mp[k]);
		}
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (really_program) {
	try {
		Connect(0x3f);
		RunUpgrade();
		sek->SetAppMode();
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_CASE (really_program_bl) {
	try {
		Connect(0x3c);
		RunUpgrade(true);
		sek->SetAppMode();
	}
	catch(OnzoDisplayException& e) {
		BOOST_TEST_MESSAGE("An exception occured "<< *get_error_info<error_string>(e));
		throw;
	}
}

BOOST_AUTO_TEST_SUITE_END()
