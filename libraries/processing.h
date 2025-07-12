#include "constants.h"

#ifndef __PROCESSING_H__
#define __PROCESSING_H__

int init_Laplacian_kernels(float initial_sigma, float sigma_increment);

int unlock_Laplacian_kernels(int n);

int get_interest_points_SIFT(uint8_t ** image, uint8_t ** scaled, uint8_t ** interest);

void harris_corner(uint8_t ** image, uint8_t ** scaled, uint8_t ** interest); 

#endif 