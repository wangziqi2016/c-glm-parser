
#include "glm_parser.h"

// Direction and distance is "packed" into a single value - 
// bucketed distance is left shifted 1 bit, and ORed with direction
static int get_dir_and_dist(int head_index, int dep_index)
{
    // On 64-bit machine this is the same
    int dist, dir;
    if(head_index > dep_index) 
    {
        dir = (int)0x00000000;
        dist = head_index - dep_index;   
    }
    else
    {
        dir = (int)0x00000001;
        dist = dep_index - head_index;
    }
    
    if(dist > 5)
    {
        if(dist < 10) dist = 5;
        else dist = 10;
    }
    
    return (dist << 1) | dir;
}

#ifndef _32BIT_ONLY

static unsigned long hash_feature_64bit(int type, int num, char *str_pp[])
{
    int i = 0;
    unsigned long h = (unsigned long)0x0000000000000000;
    unsigned long high_order;
    
    while(num-- > 0)
    {
        char *str_p = str_pp[i];
        
        while(*str_p != '\0')
        {
            high_order = h & _64BIT_HIGH_FIVE_BITS;
            h <<= 3;
            h ^= (high_order >> _64BIT_LOW_BIT_NUM);
            h ^= (unsigned long)(*str_p);
            
            h *= 0x1234567890ABCDEF;
            
            str_p++;
        }
        
        i++;
    }
    
    // Add type into hashing
    h <<= TYPE_HASH_BIT;
    h ^= (unsigned long)type;
    
    return h;
}

#endif

static unsigned long hash_feature_32bit(int type, int num, char *str_pp[])
{
    int i = 0;
    unsigned long h = (unsigned long)0x00000000;
    unsigned long high_order;
    
    while(num-- > 0)
    {
        char *str_p = str_pp[i];
        
        while(*str_p != '\0')
        {
            high_order = h & _32BIT_HIGH_FIVE_BITS;
            h <<= 3;
            h ^= (high_order >> _32BIT_LOW_BIT_NUM);
            h ^= (unsigned long)(*str_p);
            
            h *= 0x12345678;
            
            str_p++;
        }
        
        i++;
    }
    
    // Add type into hashing
    h <<= TYPE_HASH_BIT;
    h ^= (unsigned long)type;
    
    return h;
}

static unsigned long (*hash_feature)(int, int, char *[]);

static void determine_word_length()
{
    if(sizeof(unsigned long) == 4) hash_feature = hash_feature_32bit;
    #ifndef _32BIT_ONLY
    else if(sizeof(unsigned long) == 8) hash_feature = hash_feature_64bit;
    #endif
    else ERROR("Unable to determine word length = %d!", sizeof(unsigned long));
    
    return;
}

#include <time.h>

int main()
{
    determine_word_length();
    char *test_array[] = {"Thissssssss", "issfersdf", "Asdfrwed", "Tetdfdgweasd"};
    unsigned long h;
    clock_t start = clock();
    for(unsigned int i = 0;i < (unsigned int)1000000;i++)
    {
        h = hash_feature(17, 4, test_array);
    }
    clock_t end = clock();
    
    DEBUG("%f", (float)(end - start) / 1000000.0);
    DEBUG("%ld, %lx", h, h);
    
    getchar();
    
    return 0;
}
