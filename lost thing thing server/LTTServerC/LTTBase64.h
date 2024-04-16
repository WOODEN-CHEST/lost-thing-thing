#pragma once

char* Base64_Decode(const char* data, size_t* decodedDataLength);

char* Base64_Encode(const unsigned char* data, size_t dataLength);