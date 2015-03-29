
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

static unsigned long hash_feature(unsigned long type, int num, const unsigned char *str_pp[])
{
	int i = 0;
    unsigned long h = (unsigned long)0x00000000;
    unsigned long high_order;
    
    while(num-- > 0)
    {
        const unsigned char *str_p = str_pp[i];
        
        while(*str_p != '\0')
        {
            h = h * HASH_MULTIPLIER + (unsigned long)*str_p;
            
            str_p++;
        }
        
        i++;
    }
    
    // Add type into hashing
    h = h * HASH_MULTIPLIER + type;
    
    return h;
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
//              xi-word xi-pos xj-word xj-pos
//        type    num   offset
//         0       2      0
//         1       1      0
//         2       1      1
//         3       2      2
//         4       1      2
//         5       1      3
float get_unigram_feature_score(Sentence *sent, int head_index, int dep_index)
{
    unsigned long h;
    register float score = 0.0;
    int dir_dist; 
	static const unsigned char *feature_buffer[2];
    
    string *word_i = &sent->word_list[head_index];
    string *pos_i = &sent->pos_list[head_index];
    string *word_j = &sent->word_list[dep_index];
    string *pos_j = &sent->pos_list[dep_index];
    
    dir_dist = get_dir_and_dist(head_index, dep_index);
    
    feature_buffer[0] = (unsigned char *)word_i->c_str();
    feature_buffer[1] = (unsigned char *)pos_i->c_str();
    
    add_feature(0, 2, 0);
    add_feature(1, 1, 0);
    add_feature(2, 1, 1);
    
    add_feature(3, 2, 2);
    add_feature(4, 1, 2);
    add_feature(5, 1, 3);
    
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
// Memory layout:
//              xi-word xi-pos xj-word xj-pos
//        type    num     offset
//         6       4        0
//         7       3        1
//         10      3        0
//
//              xi-word xi-pos xj-pos *
//         9       3        0
//         12      2        1
//              
//              xi-word xj-word xj-pos *
//         8       3        0
//         11      2        0

float get_bigram_feature_score(Sentence *sent, int head_index, int dep_index)
{
    unsigned long h;
    register float score = 0.0;
    int dir_dist; 
	static const unsigned char *feature_buffer[4];
    
    string *word_i = &sent->word_list[head_index];
    string *pos_i = &sent->pos_list[head_index];
    string *word_j = &sent->word_list[dep_index];
    string *pos_j = &sent->pos_list[dep_index];
    
    dir_dist = get_dir_and_dist(head_index, dep_index);
    
    feature_buffer[0] = (unsigned char *)word_i->c_str();
    feature_buffer[1] = (unsigned char *)pos_i->c_str();
    feature_buffer[2] = (unsigned char *)word_j->c_str();
    feature_buffer[3] = (unsigned char *)pos_j->c_str();
    
    add_feature(6, 10, 0);
    add_feature(7, 3, 1);
    add_feature(10, 3, 0);
    
    feature_buffer[2] = (unsigned char *)pos_j->c_str();
    
    add_feature(9, 3, 0);
    add_feature(12, 2, 1);
    
    feature_buffer[1] = (unsigned char *)word_j->c_str();
    
    add_feature(8, 3, 0);
	add_feature(11, 2, 0);    
    
    return score;
}

///////////////////////////////////////////////////////////////////////
// Test code

int main()
{
    const char *test_array[] = {"issasdasd", "we23eqds", "Asdfrwed", "Tetdfdgwas"};
    unsigned long h;
    clock_t start = clock();
    for(unsigned int i = 0;i < (unsigned int)1000000;i++)
    {
        h = hash_feature(17, 4, (const unsigned char **)test_array);
    }
    clock_t end = clock();
    
    DEBUG("%f", (float)(end - start) / 1000000.0);
    DEBUG("%ld, %lx", h, h);
    
    return 0;
}
