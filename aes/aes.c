#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "..\file-header\fileheader.h"

#define BLOCK_SIZE 16

#ifdef TEST_DEF

void printKey(uc *key, int length) {
    int i, j;
    for (i = 0; i < length; ++i) {
        printf("w(%d): ", i);
        for (j = 0; j < 4; ++j) {
            printf("%x%x", key[i*4+j] >> 4, key[i*4+j] & 0x0f);
        }
        printf("\n");
    }
}

void printBlock(uc *block) {
    int i, j;
    printf("Print block:\n");
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            printf("%x%x ", block[i*4+j] >> 4, block[i*4+j] & 0x0f);
        }
        printf("\n");
    }
}
#endif // TEST_DEF

uc Sbox[256] =   {
//0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, // 0
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, // 1
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, // 2
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, // 3
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, // 4
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, // 5
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, // 6
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, // 7
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, // 8
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, // 9
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, // A
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, // B
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, // C
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, // D
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, // E
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16  // F
};

uc rSbox[256] =
{
//0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, // 0
0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, // 1
0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, // 2
0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25, // 3
0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, // 4
0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, // 5
0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06, // 6
0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, // 7
0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73, // 8
0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, // 9
0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, // A
0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, // B
0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f, // C
0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, // D
0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61, // E
0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d  // F
};

uc Rcon[255] = {
//0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, // 0
0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, // 1
0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, // 2
0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, // 3
0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, // 4
0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, // 5
0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, // 6
0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, // 7
0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, // 8
0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, // 9
0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, // A
0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, // B
0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, // C
0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, // D
0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, // E
0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb        // F
};

uc getSbox(uc element) {
    return Sbox[element];
}

uc getrSbox(uc element) {
    return rSbox[element];
}

uc getRcon(int rowNum) {
    return Rcon[(rowNum)/4];
}

void transformRow(uc *row, int rowNum, uc *transformedRow) {
    transformedRow[0] = getSbox(row[1]) ^ getRcon(rowNum);
    transformedRow[1] = getSbox(row[2]);
    transformedRow[2] = getSbox(row[3]);
    transformedRow[3] = getSbox(row[0]);
}

void expandKey(uc *key, uc *expandedKey) {
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            expandedKey[i*4 + j] = key[i*4 + j];
        }
    }
    for (int i = 4; i < 44; ++i) {
        if (i % 4 == 0) {
            uc *row = expandedKey + (i - 1)*4;
            uc transformedRow[4];
            transformRow(row, i, transformedRow);
            for (int j = 0; j < 4; ++j) {
                expandedKey[i*4 + j] = expandedKey[(i-4)*4 + j] ^ transformedRow[j];
            }
        } else {
            for (int j = 0; j < 4; ++j) {
                expandedKey[i*4 + j] = expandedKey[(i-4)*4 + j] ^ expandedKey[(i-1)*4 + j];
            }
        }
    }
}

void getRoundKeys(uc *key, uc roundKeys[11][16]) {
    uc expandedKey[176];
    expandKey(key, expandedKey);
    for (int k = 0; k < 11; ++k) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                roundKeys[k][4*j+i] = expandedKey[k*16+4*i+j];
            }
        }
    }
}

// -------------- ENCRYPTION ---------------------

void byteSub(uc *state) {
    for (int i = 0; i < 16; ++i) {
        state[i] = getSbox(state[i]);
    }
}

void shift(uc *row, int numRow) {
    for (int i = 0; i < numRow; ++i) {
        uc temp = row[0];
        for (int j = 0; j < 4; ++j) {
            row[j] = row[j+1];
        }
        row[3] = temp;
    }
}

void shiftRow(uc *state) {
    for (int i = 0; i < 4; i++)
        shift(state+i*4, i);
}

uc galoisFieldMult(uc a, uc b){
    uc r = 0, xor = 0;
    while(b) {
        if (b & 0x01)
            r ^= a;
        xor = a & 0x80;
        a <<= 1;
        if (xor)
            a ^= 0x1b;
        b >>= 1;
    }
    return r;
}

void mixColumn(uc *state) {
    uc column[4];
    uc coef[4][4] = { {2, 3, 1, 1},
                      {1, 2, 3, 1},
                      {1, 1, 2, 3},
                      {3, 1, 1, 2}};
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i)
            column[i] = state[i*4+j];
        for (int i = 0; i < 4; ++i) {
            state[i*4+j] = 0;
            for (int k = 0; k < 4; ++k)
                state[i*4+j] ^= galoisFieldMult(column[k], coef[i][k]);
        }
    }
}

void addRoundKey(uc *state, uc *roundKey) {
    for (int i = 0; i < 16; ++i) {
        state[i] ^= roundKey[i];
    }
}

void applyRound(uc *state, uc *roundKey) {
    byteSub(state);
    shiftRow(state);
    mixColumn(state);
    addRoundKey(state, roundKey);
}

void encryptBlockRoundKeys(uc *state, uc roundKeys[11][16]) {
    addRoundKey(state, roundKeys[0]);
    for (int i = 1; i <= 9; ++i)
        applyRound(state, roundKeys[i]);
    byteSub(state);
    shiftRow(state);
    addRoundKey(state, roundKeys[10]);
}

void encryptBlock(uc *state, uc *key) {
    uc roundKeys[11][16];
    getRoundKeys(key, roundKeys);
    encryptBlockRoundKeys(state, roundKeys);
}

// -------------- DECRYPTION ---------------------

void invByteSub(uc *state) {
    for (int i = 0; i < 16; ++i) {
        state[i] = getrSbox(state[i]);
    }
}

void invShift(uc *row, int numRow) {
    for (int i = 0; i < numRow; ++i) {
        uc temp = row[3];
        for (int j = 3; j > 0; --j) {
            row[j] = row[j-1];
        }
        row[0] = temp;
    }
}

void invShiftRow(uc *state) {
    for (int i = 0; i < 4; i++)
        invShift(state+i*4, i);
}

void invMixColumn(uc *state) {
    uc column[4];
    uc coef[4][4] = { {14, 11, 13,  9},
                      { 9, 14, 11, 13},
                      {13,  9, 14, 11},
                      {11, 13,  9, 14}};
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i)
            column[i] = state[i*4+j];
        for (int i = 0; i < 4; ++i) {
            state[i*4+j] = 0;
            for (int k = 0; k < 4; ++k)
                state[i*4+j] ^= galoisFieldMult(column[k], coef[i][k]);
        }
    }
}

void getInvRoundKeys(uc *key, uc invRoundKeys[11][16]) {
    uc expandedKey[176];
    expandKey(key, expandedKey);
    for (int k = 0; k < 11; ++k) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                invRoundKeys[k][4*j+i] = expandedKey[k*16+4*i+j];
            }
        }
    }
    for (int k = 1; k < 10; ++k) {
        invMixColumn(invRoundKeys[k]);
    }
}

void invAddRoundKey(uc *state, uc *roundKey) {
    for (int i = 0; i < 16; ++i) {
        state[i] ^= roundKey[i];
    }
}

void applyInvRound(uc *state, uc *roundKey) {
    invMixColumn(state);
    invAddRoundKey(state, roundKey);
    invByteSub(state);
    invShiftRow(state);
}

void decryptBlockRoundKeys(uc *state, uc invRoundKeys[11][16]) {
    addRoundKey(state, invRoundKeys[10]);
    invByteSub(state);
    invShiftRow(state);
    for (int i = 9; i >= 1; --i)
        applyInvRound(state, invRoundKeys[i]);
    addRoundKey(state, invRoundKeys[0]);
}

void decryptBlock(uc *state, uc *key) {
    uc invRoundKeys[11][16];
    getInvRoundKeys(key, invRoundKeys);
    decryptBlockRoundKeys(state, invRoundKeys);
}

// -------------- FILE ENCRYPTION ---------------------

void encryptFileECB(char *name, uc* key)
{
	FILE *in, *out;
	fileheader_t head;
	char *oname = malloc(strlen(name) + 3);
	uc state[BLOCK_SIZE];
	int i = 0;

    uc roundKeys[11][16];
    getRoundKeys(key, roundKeys);

	in = fopen(name, "rb");
	FILE_CHECK(in);

	strcpy(oname, name);
	oname[strlen(name) - 4] = '\0';
	strcat(oname, "_c.txt");

	out = fopen(oname, "wb");
	FILE_CHECK(out);
	free(oname);

	head = headerCreate(in, name);

	// headerPrint(&head);

    uc *p = &head;

	for (int i = 0; i < sizeof(head)/BLOCK_SIZE; i++) {
        encryptBlockRoundKeys(p, roundKeys);
        fwrite(p, sizeof(uc), BLOCK_SIZE, out);
        p += BLOCK_SIZE;
	}

    while ((i = fread(state, sizeof(uc), BLOCK_SIZE, in)))
    {
        for (; i<BLOCK_SIZE; i++)
            state[i] = 0;
        encryptBlockRoundKeys(state, roundKeys);
        fwrite(state, sizeof(uc), BLOCK_SIZE, out);
    }

	fclose(in);
	fclose(out);
	return;
}

int decryptFileECB(char *name, uc* key)
{
	FILE *in, *out;
	fileheader_t header;
	char *oname = malloc(strlen(name) + 3);
	uc state[BLOCK_SIZE];
	int i = 0;

    uc invRoundKeys[11][16];
    getInvRoundKeys(key, invRoundKeys);

	in = fopen(name, "rb");
	FILE_CHECK(in);

	uc *p = &header;
	for (int i = 0; i < sizeof(header)/BLOCK_SIZE; i++) {
	    fread(p, sizeof(uc), BLOCK_SIZE, in);
        decryptBlockRoundKeys(p, invRoundKeys);
        p += BLOCK_SIZE;
	}

    // headerPrint(&header);

    uint64_t len = header.byteLength;
    /*
    out = fopen(header.fileName, "rb");
    if (out == NULL) {

    }*/
	strcpy(oname, name);
	oname[strlen(name) - 4] = '\0';
	strcat(oname, "_c.txt");

	out = fopen(oname, "wb");
	FILE_CHECK(out);


	while ((i = fread(state, sizeof(uc), BLOCK_SIZE, in)))
    {
        decryptBlockRoundKeys(state, invRoundKeys);
        fwrite(state, sizeof(uc), len > BLOCK_SIZE ? BLOCK_SIZE : len, out);
        len -= BLOCK_SIZE;
    }

	fclose(in);
	fclose(out);

	in = fopen(oname, "rb");
	FILE_CHECK(in);
	free(oname);

    i = headerCheck(in, &header);

    fclose(in);

	return i;
}


#ifdef TEST_DEF
void test1() {
    uc key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    uc state[] = {0x32, 0x88, 0x31, 0xe0, 0x43, 0x5a, 0x31, 0x37, 0xf6, 0x30, 0x98, 0x07, 0xa8, 0x8d, 0xa2, 0x34};

    encryptBlock(state, key);
    printBlock(state);

    decryptBlock(state, key);
    printBlock(state);
}
#endif // TEST_DEF

int main() {
    uc key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    char name[10] = "test.txt";
    encryptFileECB(name, key);
    char name2[15] = "test_c.txt";
    printf("status = %d\n", decryptFileECB(name2, key));
}
