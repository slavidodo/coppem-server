
#ifndef NETWORKMESSAGE
#define NETWORKMESSAGE

#define NETWORKMESSAGE_MAXSIZE 24590
class NetworkMessage
{
public:
	typedef uint16_t MsgSize_t;
	// Headers:
	// 2 bytes for unencrypted message size
	// 4 bytes for checksum
	// 2 bytes for encrypted message size
	static const MsgSize_t INITIAL_BUFFER_POSITION = 8;
	enum { HEADER_LENGTH = 2 };
	enum { CHECKSUM_LENGTH = 4 };
	enum { XTEA_MULTIPLE = 8 };
	enum { MAX_BODY_LENGTH = NETWORKMESSAGE_MAXSIZE - HEADER_LENGTH - CHECKSUM_LENGTH - XTEA_MULTIPLE };
	enum { MAX_PROTOCOL_BODY_LENGTH = MAX_BODY_LENGTH - 10 };

	NetworkMessage() {
		reset();
	}

	void reset() {
		overrun = false;
		length = 0;
		position = INITIAL_BUFFER_POSITION;
	}

	// simply read functions for incoming message
	uint8_t getByte() {
		if (!canRead(1)) {
			return 0;
		}

		return buffer[position++];
	}

	uint8_t getPreviousByte() {
		return buffer[--position];
	}

	template<typename T>
	T get() {
		if (!canRead(sizeof(T))) {
			return 0;
		}

		T v;
		memcpy(&v, buffer + position, sizeof(T));
		position += sizeof(T);
		return v;
	}

	String getString(uint16_t stringLen = 0);

	// skips count unknown/unused bytes in an incoming message
	void skipBytes(int16_t count) {
		position += count;
	}

	// simply write functions for outgoing message
	void addByte(uint8_t value) {
		if (!canAdd(1)) {
			return;
		}

		buffer[position++] = value;
		length++;
	}

	template<typename T>
	void add(T value) {
		if (!canAdd(sizeof(T))) {
			return;
		}

		memcpy(buffer + position, &value, sizeof(T));
		position += sizeof(T);
		length += sizeof(T);
	}

	void addBytes(const char* bytes, size_t size);
	void addPaddingBytes(size_t n);

	void addString(const String& value);

	void addDouble(double value, uint8_t precision = 2);

	MsgSize_t getLength() const {
		return length;
	}

	void setLength(MsgSize_t newLength) {
		length = newLength;
	}

	MsgSize_t getBufferPosition() const {
		return position;
	}

	uint16_t getLengthHeader() const {
		return static_cast<uint16_t>(buffer[0] | buffer[1] << 8);
	}

	bool isOverrun() const {
		return overrun;
	}

	uint8_t* getBuffer() {
		return buffer;
	}

	const uint8_t* getBuffer() const {
		return buffer;
	}

	uint8_t* getBodyBuffer() {
		position = 2;
		return buffer + HEADER_LENGTH;
	}

	MsgSize_t getReadPosition() {
		return position;
	}

	void setReadPos(uint16_t pos) {
		position = pos;
	}

protected:
	inline bool canAdd(size_t size) const {
		return (size + position) < MAX_BODY_LENGTH;
	}

	inline bool canRead(int32_t size) {
		if ((position + size) > (length + 8) || size >= (NETWORKMESSAGE_MAXSIZE - position)) {
			overrun = true;
			return false;
		}
		return true;
	}

	MsgSize_t length;
	MsgSize_t position;
	bool overrun;

	uint8_t buffer[NETWORKMESSAGE_MAXSIZE];
};

#endif