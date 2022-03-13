#include "HexUtils.h"

char hexDigitToChar(char ch){
  switch(ch){
    case '0': return 0;
    case '1': return 1;
	case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
	case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
	case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
	case 'E': return 14;
    case 'F': return 15;
	default : return 0;
  }
}

const char charToHexDigitAlphabet[] = "0123456789ABCDEF";

char charToHexDigit(char ch){
	return charToHexDigitAlphabet[ch];
}

int hexStringToCharArray(char *output, char *input, int inputLen){
	int outputLen = 0;
	for(char i = 0; i <inputLen; i+=2){
		output[outputLen++] = hexDigitToChar(input[i])<<4 | hexDigitToChar(input[i + 1]);
	}
	return outputLen;
}

int charArrayToHexString(char *output, char *input, int inputLen){
	int outputLen = 0;
	for(char i = 0; i <inputLen; i++){
		output[outputLen++] = charToHexDigit(input[i]>>4 & 0x0F);
		output[outputLen++] = charToHexDigit(input[i] & 0x0F);
	}
	output[outputLen] = '\0';
	return outputLen;
}