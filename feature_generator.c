
#include "glm_parser.h"


// Direction and distance is "packed" into a single value - 
// bucketed distance is left shifted 1 bit, and ORed with direction
static int get_dir_and_dist(int head_index, int dep_index)
{
    // On 64-bit machine this is the same
    unsigned long dist, dir;
    if(head_index > dep_index) 
    {
        dir = 0;
        dist = head_index - dep_index;   
    }
    else
    {
        dir = 1;
        dist = dep_index - head_index;
    }
    
    // We bucket distance into 1 2 3 4 5 and 10
    // But in order to achieve binary compactness, 10 is represented
    // using 6, such that we only needs 4 bits to encode dist + dir
    if(dist > 5)
    {
        if(dist < 10) dist = 5;
        else dist = 6; // dist = 10 or = 6 does not make any difference
    }
    
    // It takes 4 LSBs to store dir + dist
    return (dist << 1) | dir;
}

//////////////////////////////////////////////////////////////////////////////
// Acknowledgement
// http://www.cs.hmc.edu/~geoff/classes/hmc.cs070.200101/homework10/hashfuncs.html
//////////////////////////////////////////////////////////////////////////////

#ifndef _32BIT_ONLY

static unsigned long hash_feature_64bit(int type, int num, const char *str_pp[])
{
    int i = 0;
    unsigned long h = (unsigned long)0x0000000000000000;
    unsigned long high_order;
    
    while(num-- > 0)
    {
        const char *str_p = str_pp[i];
        
        while(*str_p != '\0')
        {
            high_order = h & _64BIT_HIGH_FIVE_BITS;
            h <<= 3;
            h ^= (high_order >> _64BIT_LOW_BIT_NUM);
            h ^= (unsigned long)(*str_p);
            
            str_p++;
        }
        
        i++;
    }
    
    // Add type into hashing
    h <<= TYPE_HASH_BIT;
    h ^= (unsigned long)(type);
    
    return h;
}

#endif

static unsigned long hash_feature_32bit(int type, int num, const char *str_pp[])
{
    int i = 0;
    unsigned long h = (unsigned long)0x00000000;
    unsigned long high_order;
    
    while(num-- > 0)
    {
        const char *str_p = str_pp[i];
        
        while(*str_p != '\0')
        {
            high_order = h & _32BIT_HIGH_FIVE_BITS;
            h <<= 3;
            h ^= (high_order >> _32BIT_LOW_BIT_NUM);
            h ^= (unsigned long)(*str_p);
            
            str_p++;
        }
        
        i++;
    }
    
    // Add type into hashing
    h <<= TYPE_HASH_BIT;
    h ^= (unsigned long)type;
    
    return h;
}

// Callback function to calculate features
static unsigned long (*hash_feature)(int, int, const char *[]);

static void determine_word_length()
{
    if(sizeof(unsigned long) == 4) hash_feature = hash_feature_32bit;
    #ifndef _32BIT_ONLY
    else if(sizeof(unsigned long) == 8) hash_feature = hash_feature_64bit;
    #endif
    else ERROR("Unable to determine word length = %d!", sizeof(unsigned long));
    
    return;
}

///////////////////////////////////////////////////////////////////////
// Feature generator

//            +-----------------+
//            | xi-word, xi-pos | type = 0
//            | xi-word         | type = 1
//            | xi-pos          | type = 2
//            | xj-word, xj-pos | type = 3
//            | xj-word         | type = 4
//            | xj-pos          | type = 5
//            +-----------------+ 
float get_unigram_feature_score(Sentence *sent, int head_index, int dep_index)
{
    unsigned long h;
    float score = 0.0;
    int dir_dist; 
	static const char *feature_buffer[2];
    
    string *word_i = &sent->word_list[head_index];
    string *pos_i = &sent->pos_list[head_index];
    string *word_j = &sent->word_list[dep_index];
    string *pos_j = &sent->pos_list[dep_index];
    
    dir_dist = get_dir_and_dist(head_index, dep_index);
    
    feature_buffer[0] = word_i->c_str();
    feature_buffer[1] = pos_i->c_str();
    h = hash_feature(0, 2, feature_buffer);
    score += get_weight(h);
    h = hash_feature((0 << 4) ^ dir_dist, 2, feature_buffer);
    score += get_weight(h);
    
    h = hash_feature(1, 1, feature_buffer);
    score += get_weight(h);
    h = hash_feature((1 << 4) ^ dir_dist, 1, feature_buffer);
    score += get_weight(h);
    
    h = hash_feature(2, 1, feature_buffer + 1);
    score += get_weight(h);
    h = hash_feature((2 << 4) ^ dir_dist, 1, feature_buffer + 1);
    score += get_weight(h);
    
    feature_buffer[0] = word_j->c_str();
    feature_buffer[1] = pos_j->c_str();
    h = hash_feature(3, 2, feature_buffer);
    score += get_weight(h);
    h = hash_feature((3 << 4) ^ dir_dist, 2, feature_buffer);
    score += get_weight(h);
    
    h = hash_feature(4, 1, feature_buffer);
    score += get_weight(h);
    h = hash_feature((4 << 4) ^ dir_dist, 1, feature_buffer);
    score += get_weight(h);
    
    h = hash_feature(5, 1, feature_buffer + 1);
    score += get_weight(h);
    h = hash_feature((5 << 4) ^ dir_dist, 1, feature_buffer + 1);
    score += get_weight(h);
    
    return score;
}

//            +----------------------------------+
//            | xi-word, xi-pos, xj-word, xj-pos | type = 6
//            | xi-pos, xj-word, xj-pos          | type = 7
//            | xi-word, xj-word, xj-pos         | type = 8
//            | xi-word, xi-pos, xj-pos          | type = 9
//            | xi-word, xi-pos, xj-word         | type = 10
//            | xi-word, xj-word                 | type = 11
//            | xi-pos, xj-pos                   | type = 12
//            +----------------------------------+
float get_bigram_feature_score(Sentence *sent, int head_index, int dep_index)
{
    unsigned long h;
    float score = 0.0;
    int dir_dist; 
	static const char *feature_buffer[4];
    
    string *word_i = &sent->word_list[head_index];
    string *pos_i = &sent->pos_list[head_index];
    string *word_j = &sent->word_list[dep_index];
    string *pos_j = &sent->pos_list[dep_index];
    
    dir_dist = get_dir_and_dist(head_index, dep_index);
    
    feature_buffer[0] = word_i->c_str();
    feature_buffer[1] = pos_i->c_str();
    feature_buffer[2] = word_j->c_str();
    feature_buffer[3] = pos_j->c_str();
    
    h = hash_feature(6, 4, feature_buffer);
    score += get_weight(h);
    h = hash_feature((6 << 4) ^ dir_dist, 4, feature_buffer);
    score += get_weight(h);
}

///////////////////////////////////////////////////////////////////////
// Test code

#include <time.h>

int main()
{
    determine_word_length();
    const char *test_array[] = {"Thissssssss", "issfersdf", "Asdfrwed", "Tetdfdgweasd"};
    unsigned long h;
    clock_t start = clock();
    for(unsigned int i = 0;i < (unsigned int)1000000;i++)
    {
        h = hash_feature(17, 4, test_array);
    }
    clock_t end = clock();
    
    DEBUG("%f", (float)(end - start) / 1000000.0);
    DEBUG("%ld, %lx", h, h);
    
    return 0;
}
