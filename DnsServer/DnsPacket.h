#ifndef _DNSPACKET_H_
#define _DNSPACKET_H_

typedef enum {
	NoError = 0,
	FormatError = 1,
	ServerError = 2,
	NameError = 3,
	NotImplemented = 4,
	Refused = 5
} DnsResponseCodes;

typedef struct DnsPacket
{
	~DnsPacket()
	{
		if (_data)
		{
			free(_data);
			_data = NULL;
		}

		_hostname_length = 0;
		_data_length = 0;
	}

public:
	int Parse(unsigned char* buf, uint32_t len)
	{
		if (buf == NULL || len == 0 || len < 20)
			return -1;

		_data_length = len;
		unsigned char* _new_ptr = NULL;

		if (_data != NULL)
			_new_ptr = (unsigned char*)realloc(_data, _data_length);
		else
			_new_ptr = (unsigned char*)malloc(_data_length);

		if (!_new_ptr)
			return -3;

		memcpy(_new_ptr, buf, len);

		//length + null byte
		_hostname_length = 0;
		char* ptr = (char*)(_new_ptr + 0x0C);
		_hostname_length = ptr[0];
		int skip = _hostname_length;
		ptr++;
		while (ptr[0] != 0x00 && skip != 0x00 && _hostname_length <= 512)
		{
			//if we already have data, add a byte for a '.'
			if (*(_new_ptr + 0x0C) != _hostname_length)
			{
				_hostname_length++;
			}
			ptr += skip;
			skip = ptr[0];
			_hostname_length += skip;
			ptr++;
		}

		//thats just too big, wtf
		if (_hostname_length >= 512)
		{
			_hostname_length = 0;
			free(_new_ptr);
			return -4;
		}

		//add null byte
		_hostname_length++;
		_data = _new_ptr;
		return 1;
	}

	void SetError(DnsResponseCodes responseCode)
	{
		IsResponse(1);
		ReturnCode(responseCode);
	}

	int AddResponse(unsigned char* data, uint16_t len)
	{
		if (data == NULL || len != 4 || _data == NULL || _data_length == 0)
			return -1;

		int _new_data_length = _data_length + (0x0C + len);
		unsigned char* _new_ptr = (unsigned char*)malloc(_new_data_length);
		if (_new_ptr == NULL)
			return -2;

		//copy data
		memcpy(_new_ptr, _data, _data_length);

		//copy request info to response info
		uint16_t requestType = RequestType();
		uint16_t requestClass = RequestClass();

		//response is to the hostname at 0x00C, name C
		_new_ptr[_data_length + 0] = 0xC0;
		_new_ptr[_data_length + 1] = 0x0C;
		_new_ptr[_data_length + 2] = requestType >> 8;
		_new_ptr[_data_length + 3] = requestType & 0xFF;
		_new_ptr[_data_length + 4] = requestClass >> 8;
		_new_ptr[_data_length + 5] = requestClass & 0xFF;

		//Add TTL of the hostname entry
		_new_ptr[_data_length + 6] = 0x00;
		_new_ptr[_data_length + 7] = 0x00;
		_new_ptr[_data_length + 8] = 0x00;
		_new_ptr[_data_length + 9] = 0x20;

		//response length
		_new_ptr[_data_length + 0x0A] = len >> 8;
		_new_ptr[_data_length + 0x0B] = len & 0xFF;

		//response
		for (int i = 0; i < len; i++)
		{
			_new_ptr[_data_length + 0x0C + i] = data[i];
		}

		//set data
		free(_data);
		_data_length = _new_data_length;
		_data = _new_ptr;

		//set as response
		IsResponse(1);
		//if the client asked for a recursive request we'll just say its available.
		if (IsRecursive())
			IsRecursiveAvailable(1);
		AwnserCount(AwnserCount() + 1);
		AdditionalRecordsCount(0);
		NameServerCount(0);
		ReturnCode(DnsResponseCodes::NoError);
		return (int)IsResponse();
	}

	int Build(unsigned char* buf, uint32_t len)
	{
		if (buf == NULL || len == 0 || len < _data_length || _data == NULL || _data_length == 0)
			return -1;

		memcpy(buf, _data, _data_length);
		return _data_length;
	}

	const uint16_t ID() const { return (_data[0] << 8) + _data[1]; }
	const uint32_t Size() const { return _data_length; }

	const char IsResponse() const { return (_data[2] & 0b10000000) > 0; }
	const void IsResponse(char isResponse) const { _data[2] = ((_data[2] & 0b01111111) | (isResponse > 0 ? 0b10000000 : 0)); }
	const unsigned char OpCode() const { return ((_data[2] & 0b01111000) >> 3) & 0xFF; }
	const char IsAuthoritative() const { return (_data[2] & 0b00000100) > 0; }
	const char IsTruncated() const { return (_data[2] & 0b00000010) > 0; }
	const char IsRecursive() const { return (_data[2] & 0b00000001) > 0; }
	const char IsRecursiveAvailable() const { return (_data[3] & 0b10000000) > 0; }
	const void IsRecursiveAvailable(char isRecursive) const { _data[3] = ((_data[3] & 0b01111111) | (isRecursive > 0 ? 0b10000000 : 0)); }
	const char Reserved() const { return (_data[3] & 0b01110000); }
	const char ReturnCode() const { return (_data[3] & 0b00001111); }
	const void ReturnCode(DnsResponseCodes returnCode) const { _data[3] = returnCode & 0b00001111; }

	const uint16_t RequestCount() const { return (_data[4] << 8) + _data[5]; }
	const uint16_t AwnserCount() const { return (_data[6] << 8) + _data[7]; }
	const void AwnserCount(uint16_t cnt) const { _data[6] = (cnt & 0xFF00) >> 8; _data[7] = cnt & 0xFF; }
	const uint16_t NameServerCount() const { return (_data[8] << 8) + _data[9]; }
	const void NameServerCount(uint16_t cnt) const { _data[8] = (cnt & 0xFF00) >> 8; _data[9] = cnt & 0xFF; }
	const uint16_t AdditionalRecordsCount() const { return (_data[0x0A] << 8) + _data[0x0B]; }
	const void AdditionalRecordsCount(uint16_t cnt) const { _data[0x0A] = (cnt & 0xFF00) >> 8; _data[0x0B] = cnt & 0xFF; }

	const int Hostname(char* dst, uint32_t maxCnt)
	{
		if (_hostname_length <= 0 || maxCnt <= _hostname_length)
			return -1;

		char* ptr = (char*)(_data + 0x0C);
		uint32_t pos = 0;
		uint32_t str_len = ptr[0];
		ptr++;
		while (ptr != 0x00 && str_len != 0x00 && pos + str_len <= _hostname_length)
		{
			//if we have data, add a '.'
			if (dst[0] != 0x00)
			{
				dst[pos] = '.';
				pos++;
			}
			memcpy(dst + pos, ptr, str_len);
			pos += str_len;
			ptr += str_len;
			str_len = ptr[0];
			ptr++;
		}

		if (pos != _hostname_length - 1)
			memset(dst, 0, _hostname_length);

		//null termination
		dst[pos] = 0x00;
		return pos;
	}
	const uint16_t RequestType() const { return (_data[0x0C + _hostname_length + 1] << 8) + _data[0x0D + _hostname_length + 1]; }
	const uint16_t RequestClass() const { return (_data[0x0E + _hostname_length + 1] << 8) + _data[0x0F + _hostname_length + 1]; }
private:
	uint32_t _data_length = 0;
	uint32_t _hostname_length = 0;
	unsigned char* _data = NULL;

} DnsPacket;

#endif