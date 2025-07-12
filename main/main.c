#include <stdio.h>
#include "../detection.h"
#include "../processing.h"

// Signature can be saved statically in both BRIEF and SIFT formats.
// The maximum number of signatures is defined by MAX_SIGNATURES.
// static Signature_BRIEF signatures_BRIEF[MAX_SIGNATURES] = {0};
// static Signature_SIFT signatures_SIFT[MAX_SIGNATURES] = {0};

// The app allows for images saved both statically and dynamically.
// However static images still needs to be stored as arrays of pointers.
static uint8_t image_array[SIZE_X * SIZE_Y] = {0};
static uint8_t * image[SIZE_Y] = {0};

static uint8_t scaled_array[(SIZE_X * SIZE_Y)>>1] = {0};
static uint8_t * scaled[SIZE_Y>>1] = {0};

static uint8_t interest_array[(SIZE_X * SIZE_Y)>>1] = {0};
static uint8_t * interest[SIZE_Y>>1] = {0};

//void app_main(void){
int main(void){

    // Initialize the signatures arrays
    for(int i = 0; i < SIZE_Y>>1; i++) {
        image[i<<1] = &image_array[(i<<1) * SIZE_X];
        image[(i<<1) + 1] = &image_array[((i<<1)+1) * SIZE_X];
        scaled[i] = &scaled_array[i * (SIZE_X >> 1)];
        interest[i] = &interest_array[i * (SIZE_X >> 1)];
    }
    
    // init_button();
    // init_camera();

    // At the beginning of the program kernel weights have to be initialized.
    // They can be reinitialized if needed.
    init_Laplacian_kernels(1.0, 0.5);

    // This function unlocks the maximum number of Laplacian kernels.
    // The number can be changes at any time and it doesn't need kernel reinitialization.
    unlock_Laplacian_kernels(3);

    int signature_length = 0;

    /*
    *
    * FUNCTIONS TO CAPTURE AN IMAGE
    * 
    */
    
    // Detect interest points in the image using SIFT algorithm with unlocked Laplacian kernels.

    //get_interest_points_SIFT(image, scaled, interest);


    // Harris corner detection can be used instead of SIFT.

    //harris_corner(image, scaled, interest);
    

    // Sving signatures in image with SIFT fomrat.

    // save_signature_SIFT(image, scaled, interest, signatures_SIFT, &signature_length);


    // Saving signatures in image with BRIEF format.

    //save_signature_BRIEF(image, scaled, interest, signatures_BRIEF, &signature_length);
    

    // BRIEF also uses binary search trees to store signatures and they need initialization. 

    // Signature_BRIEF * heads[15] = {0};
    // int heads_length = 15;
    // init_BRIEF_BST(heads, signatures_BRIEF, heads_length, &signature_length);

    while(1){

        /*
        *
        * FUNCTIONS TO CAPTURE AN IMAGE
        * 
        */

        // Decide which function to use for interest point detection.
        // Ideally should be the same as the one used to save signatures.

        //get_interest_points_SIFT(image, scaled, interest);
        //harris_corner(image, scaled, interest);


        // Detect object using SIFT or BRIEF signatures.
        //Bounding_Box box = get_box_matches_BRIEF(image, scaled, interest, heads, heads_length, signatures_BRIEF, signature_length);
        //Bounding_Box box = get_box_matches_SIFT(image, scaled, interest, signatures_SIFT, signature_length);

        
        if(box.id >= 0){
            // Object detected, print the bounding box information.
            printf("Object detected with ID: %d at coordinates: (%d, %d) to (%d, %d)\n", 
                box.id, box.minY, box.minX, box.maxY, box.maxX);
        } else {
            // No object detected, print a message.
            printf("No object detected.\n");
        }
    }

}
