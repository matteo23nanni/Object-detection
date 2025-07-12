#include "detection.h"


int objects_IDs_SIFT = 0;
int objects_IDs_BRIEF = 0;

// Timestamp for the current frame.
// The variable is used to avoid a single signature to be matched multiple times in a single frame.
uint8_t time = 0;

// Compares two magnitude/ gradients vectors and returns the angle sector of their sum.
uint8_t get_angle_sector(int xMagnitude, int yMagnitude){
    int xCoords = (xMagnitude * COS22_INT - yMagnitude * SIN22_INT);
    int yCoords = (xMagnitude * SIN22_INT + yMagnitude * COS22_INT);

    if (xCoords > 0)
    {
        if (yCoords > 0)
        {
            if (xCoords > yCoords)
            {
                return 0;
            }
            else
            {
            return 1;
            }
        }
        else
        {
            if (xCoords > -yCoords)
            {
                return 7;
            }
            else
            {
                return 6;
            }
        }
    }
    else
    {
        if (yCoords > 0)
        {
            if (-xCoords > yCoords)
            {
                return 3;
            }
            else
            {
                return 2;
            }
        }
        else
        {
            if (xCoords < yCoords)
            {
                return 4;
            }
            else
            {
                return 5;
            }
        }
    }
}

// Calculates the magnitude of the area around a point in the image.
uint8_t area_magnitude(uint8_t ** img, int y, int x){

    unsigned int Xmomentum = 0, Ymomentum = 0; 
    for(uint8_t i = 0; i < SECTOR_SIZE; i++){
        int xp = x + circle_sectors[i][1], yp = y + circle_sectors[i][0],
            xn = x - 1 - circle_sectors[i][1], yn = y - 1 - circle_sectors[i][0];
        Xmomentum += xp*(img[yp][xp] + img[yn][xp]) + xn*(img[yp][xn] + img[yn][xn]);
        Ymomentum += yp*(img[yp][xp] + img[yp][xn]) + yn*(img[yn][xp] + img[yn][xn]);
    }

    return get_angle_sector(Xmomentum, Ymomentum);
}

// Function use to navigate through the binary search tree of signatures in BRIEF format.
int left_or_right(uint64_t * v1, uint64_t * v2, int i){

    uint8_t distance = 0;
    uint8_t shift = i & 31;
    int xor = 65535;
    if (shift > 3){
        shift -= 3;
        xor &= ((v1[1]>>(shift << 4)) ^ (v2[1]>>(shift << 4)));
    } else {
        xor &= ((v1[0]>>(shift << 4)) ^ (v2[0]>>(shift << 4)));
    }
    
    while (xor){
        distance += xor & 1;
        xor>>=1;
    }

    return distance < LEFT_OR_RIGHT_THRESHOLD;
}

// Populates the non normalized SIFT signature with the magnitudes of the gradients in the image.
void add_magnitude(int py, int px, uint8_t ** img ,uint8_t shift,
    unsigned int * signature, unsigned int sectors[ORIENTATIONS]){
   
   int Ymagnitude = ((int)img[ py - 1 ][ px     ] - (int)img[ py + 1 ][ px     ])/2;
   int Xmagnitude = ((int)img[ py     ][ px - 1 ] - (int)img[ py     ][ px + 1 ])/2;
   int magnitude = (abs(Xmagnitude) + abs(Ymagnitude));

   uint8_t sector = get_angle_sector(Xmagnitude, Ymagnitude);
   sectors[sector] += magnitude;

   signature[ORIENTATIONS * shift + sector]+=magnitude;
}

// Corrects the orientation of the SIFT signature and normalizes it to fit in 8 bits
// by shifting its content. Then it normalizes it.
void correct_orientation_and_normalize(unsigned int * signature,
    uint8_t * final_signature, uint8_t shift){

   unsigned int max = 0;
   unsigned int min = __INT_MAX__;

   for(uint8_t i = 0; i < SIGNATURE_LENGTH; i++){
       max = (max<signature[i])? signature[i] : max;
       min = (min>signature[i])? signature[i] : min;
   }

   if(min==max) return;

   for(uint8_t i = 0; i < INTERNAL_SECTORS; i++){
       for(uint8_t j = 0; j < ORIENTATIONS; j++){
           final_signature[(((i + shift) & 7) << 3) + ((j + shift) & 7)] =
               ((signature[(i << 3) + j] - min) * 255) / (max - min) ;
       }
   }

   for(uint8_t i = 0; i < EXTERN_SECTORS; i++){
       for(uint8_t j = 0; j < ORIENTATIONS; j++){
           final_signature[INTERNAL_SIGNATURE_SIZE + (((i + shift) & 7) << 3) + ((j + shift) & 7)] =
               ((signature[INTERNAL_SIGNATURE_SIZE + (i << 3) + j] - min) * 255) / (max - min) ;
       }
   }
}


int max_index(int * arr, int size){
    int max = 0;
    for(int i = 0; i < size; i++){
        if(arr[i] > arr[max]){
            max = i;
        }
    }
    return max;
}

void get_signature_SIFT(uint8_t ** img, int y, int x, 
    uint8_t * final_signature){

        unsigned int signature[SIGNATURE_LENGTH] =  {0};

        unsigned int sectors[ORIENTATIONS] = {0};
    
        for(uint8_t i = 0; i < INTERNAL_SECTOR_SIZE; i++){

            for(uint8_t j = 0; j < INTERNAL_SECTORS; j++){
                uint8_t mod = j & 3;
                uint8_t index = mod == 1 || mod == 2;
                add_magnitude(y -(j > 1 && j < 6) + internal_circle_sectors[index][i][0], x - (j > 3) 
                + internal_circle_sectors[index][i][1], img, j, signature, sectors); //00
            }
    
        }
    
        for(uint8_t i = 0; i < EXTERN_SECTOR_SIZE; i++){

            for(uint8_t j = 0; j < EXTERN_SECTORS; j++){
                uint8_t mod = j & 3;
                uint8_t index = mod == 1 || mod == 2;
                add_magnitude(y -(j > 1 && j < 6) + internal_circle_sectors[index][i][0], x - (j > 3) 
                + internal_circle_sectors[index][i][1], img, INTERNAL_SECTORS + j, signature, sectors); //00
            }

    
        }
    
        correct_orientation_and_normalize(signature, final_signature, (ORIENTATIONS - max_index(sectors, ORIENTATIONS)) & 7);
}

int save_signature_SIFT(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_SIFT * signatures, int * signature_length){

    for(int y = 0; y < SIZE_Y>>1; y++){
        for(int x = 0; x < SIZE_X>>1; x++){
            if(interest[y][x]){
                if(*signature_length < MAX_SIGNATURES){
                    get_signature_SIFT(image, y<<1, x<<1, signatures[*signature_length].signature);
                    signatures[*signature_length].id = objects_IDs_SIFT;
                    signatures[*signature_length].match = 0;
                    
                    (*signature_length)++;
                    get_signature_SIFT(scaled, y, x, signatures[*signature_length].signature);
                    signatures[*signature_length].id = objects_IDs_SIFT;
                    signatures[*signature_length].match = 0;

                    (*signature_length)++;
                } else {
                    return 0; // Not enough space for signatures
                }
            }
        }
    }
    objects_IDs_SIFT++;
    return 1; // Successfully saved signatures
}


// Corrects the coordinates of the BRIEF signature offsets based on the shift value.
void correct_coordinates(int8_t coordinates[2], int8_t shift){
    if(shift>>2){
        coordinates[1] = -coordinates[1];
        coordinates[0] = -coordinates[0];
    }
    shift-=4;
    if(shift>>1){
        coordinates[1] = -coordinates[0];
        coordinates[0] = -coordinates[1];
    }
    shift-=2;
    if(shift){
        coordinates[1] = (SIN_COS45_256 * (coordinates[1] - coordinates[0]))>>8;
        coordinates[0] = (SIN_COS45_256 * (coordinates[1] + coordinates[0]))>>8;
    }

}

int compute_distance_BRIEF(uint64_t * v1, uint64_t * v2){
    
    int res = 0;

    for(uint8_t n  = 0; n < LONG_INT_PER_SIGNATURE; n++){

        uint64_t xor = v1[n] ^ v2[n]; 

        while(xor){
            res += xor & 1;
            xor>>=1;
        }
    }

    return res;
}

int compute_distance_SIFT(uint8_t * v1, uint8_t * v2){
    int res = 0;
    for(uint8_t i = 0; i < SIGNATURE_LENGTH; i++){
        res += abs((int)v1[i] - (int)v2[i]);
    }

    return res;
}

void get_signature_BRIEF(uint8_t ** img, int y, int x, 
    uint64_t final_signature[LONG_INT_PER_SIGNATURE]){

    uint8_t shift = area_magnitude(img, y, x);

    int size = sizeof(uint64_t)<<3;
    for(int i = 0; i < LONG_INT_PER_SIGNATURE; i++){
        for(int j = size * i; j < size * (i + 1); j++){
            int8_t coords1[]={ offsets[0][j][0], offsets[0][j][1]};
            int8_t coords2[]={ offsets[1][j][0], offsets[1][j][1]};

            correct_coordinates(coords1, shift);
            correct_coordinates(coords2, shift);

            final_signature[i] |= img[y + coords1[0]][ x + coords1[1]] > img[y + coords2[0]][ x + coords2[1]];
            final_signature[i] <<= 1;
        
        }
    }
}

// Signatures too close to each other are remnoved to avoid ambiguoius matches.
int not_close_BRIEF(Signature_BRIEF * node, Signature_BRIEF * signatures_BRIEF, int i, int size_BRIEF){

    for(; i < size_BRIEF; i++){
        if(compute_distance_BRIEF(node->signature, signatures_BRIEF[i].signature) < MIN_INTRA_SIGNATURE_DISTANCE_BRIEF){
            return 0;
        }
    }

    return 1;
}


int not_close_SIFT(Signature_SIFT * node, Signature_SIFT * signatures_SIFT, int i, int size_SIFT){

    for(; i < size_SIFT; i++){
        if(compute_distance_SIFT(node->signature, signatures_SIFT[i].signature) < MIN_INTRA_SIGNATURE_DISTANCE_SIFT){
            return 0;
        }
    }

    return 1;
}

// Saves the signatures of the interest points found in the image using the BRIEF algorithm.
int save_signature_BRIEF(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_BRIEF * signatures, int *signature_length){

    for(int y = 0; y < SIZE_Y>>1; y++){
        for(int x = 0; x < SIZE_X>>1; x++){
            if(interest[y][x]){
                if(*signature_length < MAX_SIGNATURES){
                    get_signature_BRIEF(image, y<<1, x<<1, signatures[*signature_length].signature);
                    signatures[*signature_length].id = objects_IDs_BRIEF;
                    signatures[*signature_length].match = 0;
                    signatures[*signature_length].left = signatures[*signature_length].right = NULL;
                    (*signature_length)++;
                    get_signature_BRIEF(scaled, y, x, signatures[*signature_length].signature);
                    signatures[*signature_length].id = objects_IDs_BRIEF;
                    signatures[*signature_length].match = 0;
                    signatures[*signature_length].left = signatures[*signature_length].right = NULL;
                    (*signature_length)++;
                } else {
                    return 0; // Not enough space for signatures
                }
            }
        }
    }
    objects_IDs_BRIEF++;
    return 1; // Successfully saved signatures
}

int insert_node_BRIEF(Signature_BRIEF * node, Signature_BRIEF** heads, int index){
    if (heads[index] == NULL){
        heads[index] = node;
        return 1;
    }

    Signature_BRIEF * prev = heads[index];

    for(short i = 0; i < MAX_SIGNATURES && prev != NULL; i++){
        
        if(left_or_right(node->signature, prev->signature, i)){
            if(prev->left == NULL){
                prev->left = node;
                return 1;
            }else{
                prev = prev->left;
            }
        } else {
            if(prev->right == NULL){
                prev->right = node;
                return 1;
            }else{
                prev = prev->right;
            }
        }
    }

    return 0;
    
}

// Initializes the binary search tree array for BRIEF signatures.
int init_BRIEF_BST(Signature_BRIEF ** heads, Signature_BRIEF * signatures, int heads_length,
    int * signature_lengths){
    
    for (short i = 0; i < *signature_lengths; i++){
        if(!not_close_BRIEF(&signatures[i],signatures, i + 1, *signature_lengths)){
            (*signature_lengths)--;
            signatures[i] = signatures[*signature_lengths];
            i--;
        }
    }

    int shift = (*signature_lengths) / heads_length;

    for(int j = 0; j < heads_length; j++){
        for(int i = 0; i < shift; i++){
            if(!insert_node_BRIEF(&signatures[shift * j + i], heads, j)){
                return 0;
            }
        }
    }

    return 1;
}

// Checks if an interest point is a match with BRIEF
int8_t find_neighbour_BRIEF(uint8_t ** img, int y, int x, Signature_BRIEF ** heads, int heads_length){

    uint64_t signature[LONG_INT_PER_SIGNATURE] = {0};
    get_signature_BRIEF(img, y, x, signature);
    
    Signature_BRIEF * prev;

    int best_distance = 255; int second_best_distance = 255;
    Signature_BRIEF * best_sig = NULL; Signature_BRIEF * second_best_sig = NULL;

    for(int j = 0; j < heads_length; j++){
        prev = heads[j];

        for(short i = 0; i < heads_length && prev != NULL; i++){
        
            if(prev != best_sig && prev != second_best_sig && prev->match != time){

                int distance = compute_distance_BRIEF(heads[i]->signature, signature);

                if(distance < second_best_distance){
                    if(distance < best_distance){
                        second_best_distance = best_distance;
                        second_best_sig = best_sig;

                        best_distance = distance;
                        best_sig = prev;
                    } else {
                        second_best_distance = distance;
                        second_best_sig = prev;
                    }
                }
            }
            
            prev = (left_or_right(prev->signature, signature, i))? prev->left : prev->right;
        }
    }
    
    if(best_distance > MAX_DISTANCE_BRIEF) return -1;

    if(((best_distance * 10) / second_best_distance) > MAX_LOWES_RATIO) return -1;

    best_sig->match = time;

    return best_sig->id;
}

// Checks if an interest point is a mathc with SIFT
int8_t find_neighbour_SIFT(uint8_t ** img, int y, int x, Signature_SIFT * signatures, int signature_length){

    uint8_t signature[SIGNATURE_LENGTH] = {0};
    get_signature_SIFT(img, y, x, signature);

    int best_distance = __INT_MAX__; int second_best_distance = __INT_MAX__;
    Signature_SIFT * best_sig = NULL; Signature_SIFT * second_best_sig = NULL;


    for(short i = 0; i < signature_length; i++){
            if(&signatures[i] != best_sig && &signatures[i] != second_best_sig && signatures[i].match != time){

                int distance = compute_distance_SIFT(signatures[i].signature, signature);

                if(distance < second_best_distance){
                    if(distance < best_distance){
                        second_best_distance = best_distance;
                        second_best_sig = best_sig;

                        best_distance = distance;
                        best_sig = &signatures[i];
                    } else {
                        second_best_distance = distance;
                        second_best_sig = &signatures[i];
                    }
                }
            }
        }
    
    if(best_distance > MAX_DISTANCE_SIFT) return -1;

    if(((best_distance * 10) / second_best_distance) > MAX_LOWES_RATIO) return -1;

    best_sig->match = time;

    return best_sig->id;
}

int max_element(int * arr, int size){
    int max = 0;
    for(int i = 0; i < size; i++){
        if(arr[i] > max){
            max = arr[i];
        }
    }
    return max;
}

void look_up_sector(uint8_t ** interest, int startY, int startX, int maxY, int maxX, int * objects, int * interest_points, int i){
    maxX += startX;
    maxY += startY;
    for(int y = startY; y < maxY; y++){
        for(int x = startX; x < maxX; x++){
            if(interest[y][x]){
                interest_points[i]++;
            }
            if(interest[y][x] > 1 ){
                objects[interest[y][x]-2]++;
            }
        }
    }
}

// Initializes the SIFT signatures by removing those that are too close to each other.
// This phase is not necessary for object recognition
int init_SIFT_signatures(Signature_SIFT * signatures,
    int * signature_lengths){
    
    for (short i = 0; i < * signature_lengths; i++){
        if(!not_close_SIFT(&signatures[i],signatures, i + 1, * signature_lengths)){
            (* signature_lengths)--;
            signatures[i] = signatures[* signature_lengths];
            i--;
        }
    }

    return 1;
}

// Checks for objects in a interest point map and returns a  bounding box
Bounding_Box lookup(uint8_t ** interest, int signature_length, int * objects){
    
    int maxY = (SIZE_Y - SIGNATURE_PADDING) >> 1;
    int maxX = (SIZE_X - SIGNATURE_PADDING) >> 1;

    int min_interest_points = signature_length/5;

    // It first checks if there are neough matches in the whole image.
    if(max_element(objects, MAX_OBJECTS) > min_interest_points){
        Bounding_Box box = {max_index(objects, MAX_OBJECTS), SIZE_X, 0, SIZE_Y, 0};
        return box;
    }

    int startX = 0;
    int startY = 0;

    int shiftY = maxY>>2;
    int shiftX = maxX>>2;

    maxX -= shiftX;
    maxY -= shiftY;

    // This loop reduces the size of the area to be searched for matches after each iteration.
    while( maxX > MIN_INTEREST_SIZE && maxY > MIN_INTEREST_SIZE){
        int matches[4] = {0};
        int objects_index[MAX_OBJECTS] = {0};
        int interest_points[4] = {0};

        min_interest_points *=9;
        min_interest_points >>= 4;

        // Each sector is placed in a corner of the previous area.
        // Each sector is labeled with a number from 0 to 3, where:
        // 0 => top left, 1 => top right, 2 => bottom left 3 => bottom right.
        // To get the coordinater of a sector we need to add a shift to the current coordinates.
        // Shift is added using booleans, in fact no shift is added to sector 0 (j & 1 == 0 && j > 1 == 0),
        // shiftY is added to sector 1 (j & 1 == 1 && j > 1 == 0),
        // shiftX is added to sector 2 (j & 1 == 0 && j > 1 == 1),
        // and both shifts are added to sector 3 (j & 1 == 1 && j > 1 == 1).
        for(int j = 0; j < 4; j++){

            memset(objects, 0, sizeof(objects));
            look_up_sector(interest, startY + shiftY * (j & 1), startX + shiftX * (j > 1), maxY, maxX, objects, interest_points, j);
            matches[j] = max_element(objects, MAX_OBJECTS);
            objects_index[j] = max_index(objects, MAX_OBJECTS);
            
        }

        // The area with the most matches is selected as the bounding box or for the next iteration.
        int max = max_index(matches, 4);
        if(matches[max] > min_interest_points){
            Bounding_Box box = {objects_index[max], (startX + shiftX * (max > 1) + maxX)<<1, 
                (startX + shiftX * (max > 1))<<1, (startY + shiftY * (max & 1) + maxY)<<1, (startY + shiftY * (max & 1))<<1};
            
            return box;
        }   

        // If no matches are found then the area is reduced by 25% in each direction.
        maxX -= maxX>>2;
        maxY -= maxY>>2;

        startY += (max & 1) * shiftY;
        startX += (max > 1) * shiftX;
        
        shiftX -= shiftX>>2;
        shiftY -= shiftY>>2;

    }

    // Default bounding box if no matches are found.
    Bounding_Box box = {-1, 0, 0, 0, 0};
    return box;
}

Bounding_Box get_box_matches_SIFT(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_SIFT * signatures, int signature_length){

    if(time == 255){
        time = 0;
        for(int i = 0; i < signature_length; i++){
            signatures[i].match = 0;
        }
    }

    int interest_points = 0;
    
    time++;

    int maxY = (SIZE_Y - SIGNATURE_PADDING) >> 1;
    int maxX = (SIZE_X - SIGNATURE_PADDING) >> 1;

    int objects[MAX_OBJECTS] = {0};

    int mathces_found = 0;

    for(int y = SIGNATURE_PADDING>>1; y < maxY; y++){
        for(int x = (SIGNATURE_PADDING>>1); x < maxX; x++){
            if(interest[y][x]){
                
                interest[y][x] = (uint8_t)(find_neighbour_SIFT(scaled, y, x, signatures, signature_length) + 2);

                if (interest[y][x] <= 1){
                    interest[y][x] = (uint8_t)(find_neighbour_SIFT(image, y<<1, x<<1, signatures, signature_length) + 2);
                }
                // If an interest point is matched then is marked in the interest map with the object ID + 2.
                // The +2 is so that in the interest map 0 => no interest point, 1 => interest point with no match,
                // and 2 => interest point with a match with object ID = value - 2.
                interest_points++;
                if(interest[y][x] > 1) {
                    mathces_found++;
                    objects[interest[y][x] - 2]++;
                }
            }
        }
    }

    return lookup(interest, signature_length, objects);
}

Bounding_Box get_box_matches_BRIEF(uint8_t ** image, uint8_t ** scaled,
    uint8_t ** interest, Signature_BRIEF ** heads, int heads_length, Signature_BRIEF * signatures, int signature_length){

    if(time == 255){
        time = 0;
        for(int i = 0; i < signature_length; i++){
            signatures[i].match = 0;
        }
    }

    int interest_points = 0;
    
    time++;

    int maxY = (SIZE_Y - SIGNATURE_PADDING) >> 1;
    int maxX = (SIZE_X - SIGNATURE_PADDING) >> 1;

    int objects[MAX_OBJECTS] = {0};

    int mathces_found = 0;

    for(int y = SIGNATURE_PADDING>>1; y < maxY; y++){
        for(int x = (SIGNATURE_PADDING>>1); x < maxX; x++){
            if(interest[y][x]){
                
                interest[y][x] = (uint8_t)(find_neighbour_BRIEF(scaled, y, x, heads, heads_length) + 2);

                if (interest[y][x] <= 1){
                    interest[y][x] = (uint8_t)(find_neighbour_BRIEF(image, y<<1, x<<1, heads, heads_length) + 2);
                }
                interest_points++;
                // Same as above
                if(interest[y][x] > 1) {
                    mathces_found++;
                    objects[interest[y][x] - 2]++;
                }
            }
        }
    }

    return lookup(interest, signature_length, objects);
}