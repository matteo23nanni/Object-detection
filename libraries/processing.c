#include "processing.h"


typedef struct _Kernel {
    int8_t window[KERNEL_MAX_SIZE][KERNEL_MAX_SIZE];
    uint8_t size;
} Kernel;

Kernel kernels[NUMBER_OF_KERNELS];

int number_of_kernels = 0;

int convolve(uint8_t ** img, int y, int x, int index){

    int pad = kernels[index].size>>1;
    int sum = 0;
    int ny, nx;
    for (int ky = 0; ky < kernels[index].size; ky++) {
        for (int kx = 0; kx < kernels[index].size; kx++) {

            ny = y + ky - pad;
            nx = x + kx - pad;
            sum += img[ny][nx] * kernels[index].window[ky][kx];
        
        }
    }

    return sum;
}

void scale_down(uint8_t ** image, uint8_t ** scaled){

    int sizeX = SIZE_X>>1;
    int sizeY = SIZE_Y>>1;
    for (int y = 0; y < sizeY; y++){
        for(int x = 0; x < sizeX; x++){
            scaled[y][x] = (image[(y<<1) + 1][(x<<1) + 1] 
                       + image[(y<<1) + 1][ x<<1     ] 
                       + image[ y<<1     ][(x<<1) + 1] 
                       + image[ y<<1     ][ x<<1     ]) >> 2;
        }
    }
}

int is_edge_response(uint8_t ** img, int y, int x) {
    int padding = EDGE_AREA>>1;

    long long sumXX = 0, sumYY = 0, sumXY = 0;

    for (int ky = -padding; ky <= padding; ky++) {
        for (int kx = -padding; kx <= padding; kx++) {
            long long dx = (img[y + ky][x + kx + 1] - img[y + ky][x + kx - 1])>>1;
            long long dy = (img[y + kx + 1][x + kx] - img[y - ky - 1][x + kx])>>1;
            sumXX += dx * dx;
            sumYY += dy * dy;
            sumXY += dx * dy;
        }
    }

    long long det = (sumXX * sumYY) - (sumXY * sumXY);
    if (det <= 0) return 0;
    long long trace = sumXX + sumYY;
    long long ratio = det - (((trace * trace)<<2)/100);

    return ratio > 0 && (ratio>>10) > HARRIS_THRESHOLD_SIFT;
}

void non_max_suppression_signature(uint8_t ** img, uint8_t ** interest) {

    int sizeY = SIZE_Y>>1;
    int sizeX = SIZE_X>>1;

    int pad = KERNEL_NON_MAX_SIZE>>1;

    int Y = sizeY - (SIGNATURE_PADDING>>1);
    int X = sizeX - (SIGNATURE_PADDING>>1);

    for (int i = (SIGNATURE_PADDING>>1); i < Y; i++) {
        for (int j = (SIGNATURE_PADDING>>1); j < X; j++) {

            if (interest[i][j]){
                if(is_edge_response(img, i, j)){
                    int current = interest[i][j];
                    int max = current;

                    for (int ky = -pad; ky <= pad && max == current; ky++) {
                        for (int kx = -pad; kx <= pad && max == current; kx++) {
                            if (interest[i + ky][j + kx] > max) {
                                max = interest[i + ky][j + kx];
                            }
                        }
                    }

                    // Keep the value only if it's a local maximum
                    if ( max==current) {
                        for (int ky = -pad; ky <= pad; ky++) {
                            for (int kx = -pad; kx <= pad; kx++) { 
                                interest[i + ky][j + kx] = BINARY_ZERO;
                            }
                        }
                        interest[i][j] = BINARY_ONE;

                    } else {
                        interest[i][j] = BINARY_ZERO;
                    }
                } else {
                    interest[i][j] = BINARY_ZERO;
                }
            }
        }
    }
}


int init_Laplacian_kernels(float initial_sigma, float sigma_increment){

    if (initial_sigma <= 0 || sigma_increment <= 0) {
        return 0; // Invalid parameters
    }

    float sigma = initial_sigma;

    for(int n = 0; n < NUMBER_OF_KERNELS; n++){
        int size = INITIAL_KERNEL_SIZE + 2 * n;  // Kernel size increases with n
        int center = size >> 1;  // Kernel center
        float sigma2 = sigma * sigma;
        float sigma4 = sigma2 * sigma2;
        float factor = -100.0 / (3.14 * sigma4);  // Scale factor for ints
        float sum = 0.0;  

        // Compute floating-point kernel first
        float float_kernel[size][size];
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                int x = i - center;
                int y = j - center;
                float r2 = x * x + y * y;
                float_kernel[i][j] = factor * (1 - r2 / (2 * sigma2)) * exp(-r2 / (2 * sigma2));
                sum += float_kernel[i][j];
            }
        }

        // Convert to ints and normalize
        int int_sum = 0;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                kernels[n].window[i][j] = (int) round(float_kernel[i][j]); // Convert to int
                int_sum += kernels[n].window[i][j];
            }
        }

        // Adjust sum to zero (normalization step)
        int mean = int_sum / (size * size);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                kernels[n].window[i][j] -= mean;
            }
        }
        kernels[n].size = size;  // Store the size of the kernel
        sigma += sigma_increment;
    }

    return 1; // Successfully initialized kernels
}

int unlock_Laplacian_kernels(int n){
    if( n < 1 || n > NUMBER_OF_KERNELS){
        return 0; // Invalid kernel index
    }
    number_of_kernels = n;
    return 1; // Successfully unlocked kernels
}

// This function detects interest points in the image using the SIFT algorithm with unlocked Laplacian kernels.
int get_interest_points_SIFT(uint8_t ** image, uint8_t ** scaled, uint8_t ** interest){


    if (number_of_kernels < 1) {
        return 0; // No kernels unlocked
    }
    int sizeY = SIZE_Y>>1;
    int sizeX = SIZE_X>>1;

    for(int i = 0; i< sizeY; i++){
        memset(interest[i], 0, sizeX);
    }
 
    scale_down(image, scaled);

    for(int y = (SIGNATURE_PADDING>>1); y < sizeY - (SIGNATURE_PADDING>>1); y++){
        for(int x = (SIGNATURE_PADDING>>1); x < sizeX -(SIGNATURE_PADDING>>1) ; x++){

            int current = abs(convolve(image, y<<1, x<<1, 0));

            for(int i = 0; i < number_of_kernels; i++){

                int conv = abs(convolve(scaled, y, x, i));
                current = (current < conv)? conv : current;

            }


            if (current > INTEREST_THRESHOLD_INT){
                // Scale down the interest value to retain only significant responses
                // Cramping is stull applied to avoid overflow
                current>>=4;
                interest[y][x] = (current > 255)? 255 : current;
            }
        }
    }

    non_max_suppression_signature(scaled, interest);
    return 1; // Successfully found interest points
}

void non_max_suppression_harris(uint8_t ** interest) {

    int sizeY = SIZE_Y>>1;
    int sizeX = SIZE_X>>1;

    int pad = KERNEL_NON_MAX_SIZE>>1;

    int Y = sizeY - (SIGNATURE_PADDING>>1);
    int X = sizeX - (SIGNATURE_PADDING>>1);

    for (int i = (SIGNATURE_PADDING>>1); i < Y; i++) {
        for (int j = (SIGNATURE_PADDING>>1); j < X; j++) {

            if (interest[i][j]){
                if(interest[i][j] > HARRIS_THRESHOLD){
                    int current = interest[i][j];
                    int max = current;

                    for (int ky = -pad; ky <= pad && max == current; ky++) {
                        for (int kx = -pad; kx <= pad && max == current; kx++) {
                            if (interest[i + ky][j + kx] > max) {
                                max = interest[i + ky][j + kx];
                            }
                        }
                    }

        // Keep the value only if it's a local maximum
                    if ( max==current) {
                        for (int ky = -pad; ky <= pad; ky++) {
                            for (int kx = -pad; kx <= pad; kx++) { 
                                interest[i + ky][j + kx] = BINARY_ZERO;
                            }
                        }
                        interest[i][j] = BINARY_ONE;

                    } else {
                        interest[i][j] = BINARY_ZERO;
                    }
                } else {
                    interest[i][j] = BINARY_ZERO;
                }
            }
        }
    }
}

// Used to detect interest points in the image using the Harris corner detection algorithm.
void harris_corner(uint8_t ** image, uint8_t ** scaled, uint8_t ** interest){
    
    int sizeY = SIZE_Y>>1;
    int sizeX = SIZE_X>>1;

    for(int i = 0; i< sizeY; i++){
        memset(interest[i], 0, sizeX);
    }
 
    scale_down(image, scaled);

    int padding = HARRIS_KERNEL_SIZE>>1;

    int maxY = (SIZE_Y-SIGNATURE_PADDING)>>1, maxX = (SIZE_X-SIGNATURE_PADDING)>>1;

    for(int y = SIGNATURE_PADDING>>1; y < maxY; y++){
        for(int x = SIGNATURE_PADDING>>1; x < maxX; x++){
            int sumXX = 0, sumYY = 0, sumXY = 0;
            
            for (int ky = -padding; ky <= padding; ky++) {
                for (int kx = -padding; kx <= padding; kx++) {
                    int dx = (image[(y<<1) + ky][(x<<1) + kx + 1] - image[(y<<1) + ky][(x<<1) + kx - 1])>>1;
                    int dy = (image[(y<<1) + kx + 1][(x<<1) + kx] - image[(y<<1) - ky - 1][(x<<1) + kx])>>1;
                    sumXX += dx * dx;
                    sumYY += dy * dy;
                    sumXY += dx * dy;
                }
            }
            int det = (sumXX * sumYY) - (sumXY * sumXY);
            
            if (det <= 0){
                interest[y][x] = 0;
            } else {
                int trace = sumXX + sumYY;
                
                long long ratio = (long long)det - ((((long long)trace * (long long)trace)<<2)/100);
                if (ratio < 0){
                    interest[y][x] = 0;
                } else {
                    // Scale down the interest value to retain only significant responses
                    // Cramping is still applied to avoid overflow
                    ratio >>= 19;
                    interest[y][x] = (ratio > 255)? 255 : ratio;
                }
            }
        }
    }
    non_max_suppression_harris(interest);
}