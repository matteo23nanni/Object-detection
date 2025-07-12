#include "constants.h"

#ifndef __DETECTION_H__
#define __DETECTION_H__

typedef struct _Bounding_Box{
    int id;
    int maxX, minX, 
        maxY, minY;
}  Bounding_Box;

typedef struct _Signature_SIFT{
    uint8_t signature[SIGNATURE_LENGTH];
    // ID of the object and match timestamp
    uint8_t id, match;
} Signature_SIFT;

typedef struct _Signature_BRIEF{
    uint64_t signature[LONG_INT_PER_SIGNATURE];

    // Same as above
    uint8_t id, match;

    // Binary search tree pointers for BRIEF signatures
    struct _Signature_BRIEF * left, * right;
} Signature_BRIEF;

// Global variables to keep track of the number of objects registered by SIFT and BRIEF algorithms.
extern int objects_IDs_SIFT;
extern int objects_IDs_BRIEF;

int save_signature_SIFT(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_SIFT * signatures, int * signature_length);

int save_signature_BRIEF(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_BRIEF * signatures, int * signature_length);

int init_BRIEF_BST(Signature_BRIEF ** heads, Signature_BRIEF * signatures, int heads_length,
    int * signature_lengths);

int init_SIFT_signatures(Signature_SIFT * signatures,
    int * signature_lengths);

Bounding_Box get_box_matches_SIFT(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_SIFT * signatures, int signature_length);

Bounding_Box get_box_matches_BRIEF(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_BRIEF ** heads, int heads_length, Signature_BRIEF * signatures, int signature_lengths);

#endif