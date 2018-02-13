#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <gl2d.h>

#include "ndsheaderbanner.h"

extern sNDSBannerExt ndsBanner;

typedef enum
{
	GL_FLIP_BOTH	= (1 << 3)
} GL_FLIP_MODE_XTRA;

// Subroutine function signatures arm9
u32 moduleParamsSignature[2]   = {0xDEC00621, 0x2106C0DE};

//
// Look in @data for @find and return the position of it.
//
u32 getOffset(u32* addr, size_t size, u32* find, size_t sizeofFind, int direction)
{
	u32* end = addr + size/sizeof(u32);

    //u32 result = 0;
	bool found = false;

	do {
		for(int i=0;i<(int)sizeofFind;i++) {
			if (addr[i] != find[i]) 
			{
				break;
			} else if(i==(int)sizeofFind-1) {
				found = true;
			}
		}
		if(!found) addr+=direction;
	} while (addr != end && !found);

	if (addr == end) {
		return NULL;
	}

	return (u32)addr;
}

/**
 * Get the title ID.
 * @param ndsFile DS ROM image.
 * @param buf Output buffer for title ID. (Must be at least 4 characters.)
 * @return 0 on success; non-zero on error.
 */
int grabTID(FILE* ndsFile, char *buf) {
	fseek(ndsFile, offsetof(sNDSHeadertitlecodeonly, gameCode), SEEK_SET);
	size_t read = fread(buf, 1, 4, ndsFile);
	return !(read == 4);
}

char arm9binary[0x100000];

/**
 * Get SDK version from an NDS file.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @return 0 on success; non-zero on error.
 */
u32 getSDKVersion(FILE* ndsFile) {
	sNDSHeaderExt NDSHeader;
	fseek(ndsFile, 0, SEEK_SET);
	fread(&NDSHeader, 1, sizeof(NDSHeader), ndsFile);
	
	fseek(ndsFile, NDSHeader.arm9romOffset, SEEK_SET);
	if(NDSHeader.arm9binarySize > 0x100000) NDSHeader.arm9binarySize = 0x100000;
	fread(&arm9binary, 1, NDSHeader.arm9binarySize, ndsFile);

	// Looking for moduleparams
	uint32_t moduleparams = getOffset((u32*)arm9binary, NDSHeader.arm9binarySize, (u32*)moduleParamsSignature, 2, 1);
	if(!moduleparams) {
		return 0;
	}

	return ((module_params_t*)(moduleparams - 0x1C))->sdk_version;
}

// bnriconframeseq[]
static u16 bnriconframeseq[39][64] = {0x0000};

// bnriconframenum[]
int bnriconframenumX[39] = {0};
int bnriconframenumY[39] = {0};
int bannerFlip[39] = {GL_FLIP_NONE};

// bnriconisDSi[]
bool bnriconisDSi[39] = {false};

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence(int iconnum) {
	for (int i = 0; i < 64; i++) {
		bnriconframeseq[iconnum][i] = ndsBanner.dsi_seq[i];
	}
}

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence(int iconnum) {
	for (int i = 0; i < 64; i++) {
		bnriconframeseq[iconnum][i] = 0x0000;
	}
}

static u16 bannerDelayNum[39] = {0x0000};
int currentbnriconframeseq[39] = {0};

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence(int iconnum) {
	if(bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]] == 0x0001
	&& bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]+1] == 0x0100) {
		// Do nothing if icon isn't animated
		bnriconframenumX[iconnum] = 0;
		bnriconframenumY[iconnum] = 0;
		bannerFlip[iconnum] = GL_FLIP_NONE;
	} else {
		u16 setframeseq = bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]];

		if((setframeseq >= 0x0000) && (setframeseq < 0x0100)) {
			bnriconframenumY[iconnum] = 0;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0100) && (setframeseq < 0x0200)) {
			bnriconframenumY[iconnum] = 1;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0200) && (setframeseq < 0x0300)) {
			bnriconframenumY[iconnum] = 2;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0300) && (setframeseq < 0x0400)) {
			bnriconframenumY[iconnum] = 3;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0400) && (setframeseq < 0x0500)) {
			bnriconframenumY[iconnum] = 4;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0500) && (setframeseq < 0x0600)) {
			bnriconframenumY[iconnum] = 5;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0600) && (setframeseq < 0x0700)) {
			bnriconframenumY[iconnum] = 6;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else if((setframeseq >= 0x0700) && (setframeseq < 0x0800)) {
			bnriconframenumY[iconnum] = 7;
			bannerFlip[iconnum] = GL_FLIP_NONE;
		} else // Check for horizontally flipped frames
		if((setframeseq >= 0x4000) && (setframeseq < 0x4100)) {
			bnriconframenumY[iconnum] = 0;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4100) && (setframeseq < 0x4200)) {
			bnriconframenumY[iconnum] = 1;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4200) && (setframeseq < 0x4300)) {
			bnriconframenumY[iconnum] = 2;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4300) && (setframeseq < 0x4400)) {
			bnriconframenumY[iconnum] = 3;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4400) && (setframeseq < 0x4500)) {
			bnriconframenumY[iconnum] = 4;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4500) && (setframeseq < 0x4600)) {
			bnriconframenumY[iconnum] = 5;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4600) && (setframeseq < 0x4700)) {
			bnriconframenumY[iconnum] = 6;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else if((setframeseq >= 0x4700) && (setframeseq < 0x4800)) {
			bnriconframenumY[iconnum] = 7;
			bannerFlip[iconnum] = GL_FLIP_H;
		} else // Check for vertically flipped frames
		if((setframeseq >= 0x8000) && (setframeseq < 0x8100)) {
			bnriconframenumY[iconnum] = 0;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8100) && (setframeseq < 0x8200)) {
			bnriconframenumY[iconnum] = 1;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8200) && (setframeseq < 0x8300)) {
			bnriconframenumY[iconnum] = 2;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8300) && (setframeseq < 0x8400)) {
			bnriconframenumY[iconnum] = 3;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8400) && (setframeseq < 0x8500)) {
			bnriconframenumY[iconnum] = 4;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8500) && (setframeseq < 0x8600)) {
			bnriconframenumY[iconnum] = 5;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8600) && (setframeseq < 0x8700)) {
			bnriconframenumY[iconnum] = 6;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else if((setframeseq >= 0x8700) && (setframeseq < 0x8800)) {
			bnriconframenumY[iconnum] = 7;
			bannerFlip[iconnum] = GL_FLIP_V;
		} else // Check for both horizontally & vertically flipped frames
		if((setframeseq >= 0xC000) && (setframeseq < 0xC100)) {
			bnriconframenumY[iconnum] = 0;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC100) && (setframeseq < 0xC200)) {
			bnriconframenumY[iconnum] = 1;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC200) && (setframeseq < 0xC300)) {
			bnriconframenumY[iconnum] = 2;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC300) && (setframeseq < 0xC400)) {
			bnriconframenumY[iconnum] = 3;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC400) && (setframeseq < 0xC500)) {
			bnriconframenumY[iconnum] = 4;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC500) && (setframeseq < 0xC600)) {
			bnriconframenumY[iconnum] = 5;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC600) && (setframeseq < 0xC700)) {
			bnriconframenumY[iconnum] = 6;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		} else if((setframeseq >= 0xC700) && (setframeseq < 0xC800)) {
			bnriconframenumY[iconnum] = 7;
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		}

		bannerDelayNum[iconnum]++;
		if(bannerDelayNum[iconnum] >= (setframeseq & 0x00FF)) {
			bannerDelayNum[iconnum] = 0x0000;
			currentbnriconframeseq[iconnum]++;
			if(bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]] == 0x0000) {
				currentbnriconframeseq[iconnum] = 0;	// Reset sequence
			}
		}

	}
}
