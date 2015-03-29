
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
    
    add_feature(0, 2, 0);
    add_feature(1, 1, 0);
    add_feature(2, 1, 1);
    
    add_feature(3, 2, 2);
    add_feature(4, 1, 2);
    add_feature(5, 1, 3);
    
    // Add five gram word feature
    if(sent->five_gram_flag[head_index] == true)
    {
    	string *word_i_5 = &sent->five_gram_word_list[head_index];
    	
    	feature_buffer[0] = (unsigned char *)word_i_5->c_str();
    	
    	add_feature(0, 2, 0);
    	add_feature(1, 1, 0);
    }
    
    if(sent->five_gram_flag[dep_index] == true)
    {
    	string *word_j_5 = &sent->five_gram_word_list[dep_index];
    	
    	feature_buffer[2] = (unsigned char *)word_j_5->c_str();
    	
    	add_feature(3, 2, 2);
    	add_feature(4, 1, 2);
	}
	
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
//         7       3        1   --> no word_i
//         10      3        0
//
//              xi-word xi-pos xj-pos *
//         9       3        0  --> no word_j
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
	
	// Add five gram word feature
	string *word_i_5 = &sent->five_gram_word_list[head_index];
    string *word_j_5 = &sent->five_gram_word_list[dep_index];
    
    feature_buffer[0] = (unsigned char *)word_i_5->c_str();
    feature_buffer[1] = (unsigned char *)pos_i->c_str();
    feature_buffer[2] = (unsigned char *)word_j_5->c_str();
    feature_buffer[3] = (unsigned char *)pos_j->c_str();
    
    bool word_i_flag = sent->five_gram_flag[head_index];
    bool word_j_flag = sent->five_gram_flag[dep_index];
    
    if(word_i_flag == true && word_j_flag == true)
    {
    	add_feature(6, 10, 0);
    	add_feature(10, 3, 0);
    	add_feature(7, 3, 1);
    
    	feature_buffer[2] = (unsigned char *)pos_j->c_str();
 		add_feature(9, 3, 0);
    
    	feature_buffer[1] = (unsigned char *)word_j_5->c_str();
    
    	add_feature(8, 3, 0);
		add_feature(11, 2, 0);  
	}
	else if(word_i_flag == true)
	{
		add_feature(6, 10, 0);
    	add_feature(10, 3, 0);
    
    	feature_buffer[2] = (unsigned char *)pos_j->c_str();
 		add_feature(9, 3, 0);
    
    	feature_buffer[1] = (unsigned char *)word_j_5->c_str();
    
    	add_feature(8, 3, 0);
		add_feature(11, 2, 0);  
	}
	else if(word_j_flag == true)
    {
    	add_feature(6, 10, 0);
    	add_feature(10, 3, 0);
    	add_feature(7, 3, 1);
    
    	feature_buffer[2] = (unsigned char *)pos_j->c_str();
    	feature_buffer[1] = (unsigned char *)word_j_5->c_str();
    
    	add_feature(8, 3, 0);
		add_feature(11, 2, 0);  
	}
    
    return score;
}

//        xi-pos xb-pos xj-pos  type = 12
//    type   num   offset
//     12     3      0
float get_in_between_feature_score(Sentence *sent, int head_index, int dep_index)
{
	unsigned long h;
    register float score = 0.0;
    int dir_dist = get_dir_and_dist(head_index, dep_index); 
	static const unsigned char *feature_buffer[3];
	
	string *pos_i = &sent->pos_list[head_index];
    string *pos_j = &sent->pos_list[dep_index];
	
	int start_index, end_index;
	if(head_index > dep_index) 
	{
		start_index = dep_index;
		end_index = head_index;	
	}
	else
	{
		start_index = head_index;
		end_index = dep_index;
	}
	
	feature_buffer[0] = (unsigned char *)pos_i->c_str();
	feature_buffer[2] = (unsigned char *)pos_j->c_str();
	
	for(int i = start_index;i < dep_index;i++)
	{
		string *pos_b = &sent->pos_list[i];
		feature_buffer[1] = (unsigned char *)pos_b->c_str();
		add_feature(12, 3, 0); 
	}
	
	return score;
} 
//        +------------------------------------+
//        | xi_pos, xi+1_pos, xj-1_pos, xj_pos | type = 13
//        | xi_pos, xi+1_pos,         , xj_pos | type = 14
//        | xi_pos,           xj-1_pos, xj_pos | type = 15
//        | xi-1_pos, xi_pos, xj-1_pos, xj_pos | type = 16
//        |           xi_pos, xj-1_pos, xj_pos | type = 17
//        | xi-1_pos, xi_pos,           xj_pos | type = 18
//        | xi_pos, xi+1_pos, xj_pos, xj+1_pos | type = 19
//        | xi_pos,           xj_pos, xj+1_pos | type = 20
//        | xi_pos, xi+1_pos, xj_pos           | type = 21
//        | xi-1_pos, xi_pos, xj_pos, xj+1_pos | type = 22
//        |           xi_pos, xj_pos, xj+1_pos | type = 23
//        | xi-1_pos, xi_pos, xj_pos           | type = 24
//        +------------------------------------+
//
float get_surrounding_feature_score(Sentence *sent, int head_index, int dep_index)
{
	unsigned long h;
    register float score = 0.0;
    int dir_dist = get_dir_and_dist(head_index, dep_index); 
	static const unsigned char *feature_buffer[4];
	int largest_index = sent->word_list.size() - 1;
	// When we are at the boundry of the sentence
	static string *null_pos = new string("_N_");
	
	string *pos_i = &sent->pos_list[head_index];
    string *pos_j = &sent->pos_list[dep_index];
    string *pos_i_plus, *pos_i_minus, *pos_j_plus, *pos_j_minus;
    if(head_index == largest_index) pos_i_plus = null_pos;
    else pos_i_plus = &sent->pos_list[head_index + 1];
    
    if(head_index == 0) pos_i_minus = null_pos;
    else pos_i_minus = &sent->word_list[head_index - 1];
    
    if(dep_index == largest_index) pos_j_plus = null_pos;
    else pos_j_plus = &sent->pos_list[dep_index + 1];
    
    if(dep_index == 0) pos_j_minus = null_pos;
    else pos_j_minus = &sent->word_list[dep_index - 1];
    
    feature_buffer[0] = (const unsigned char *)pos_i->c_str();
    feature_buffer[1] = (const unsigned char *)pos_i_plus->c_str();
    feature_buffer[2] = (const unsigned char *)pos_j_minus->c_str();
    feature_buffer[3] = (const unsigned char *)pos_j->c_str();
    
	//i i+1 j-1 j
    add_feature(13, 4, 0);
    
    feature_buffer[2] = (const unsigned char *)pos_j->c_str();
    
    // i i+1 j j
    add_feature(14, 3, 0);
    
    feature_buffer[1] = (const unsigned char *)pos_i->c_str();
    feature_buffer[2] = (const unsigned char *)pos_j_minus->c_str();
    
    // i i j-1 j
    add_feature(15, 3, 1);
    
    feature_buffer[0] = (const unsigned char *)pos_i_minus->c_str();
    
    // i-1 i j-1 j
    add_feature(16, 4, 0);
    add_feature(17, 3, 1);
    
    feature_buffer[2] = (const unsigned char *)pos_j->c_str();
    
    // i-1 i j j
	add_feature(18, 3, 0);
	
    feature_buffer[0] = (const unsigned char *)pos_i->c_str();
    feature_buffer[1] = (const unsigned char *)pos_i_plus->c_str();
    feature_buffer[3] = (const unsigned char *)pos_j_plus->c_str();
    
    // i i+1 j j+1
    add_feature(19, 4, 0);
    
    feature_buffer[1] = (const unsigned char *)pos_i->c_str();
    
    // i i j j+1
    add_feature(20, 3, 1);
    
    feature_buffer[1] = (const unsigned char *)pos_i_plus->c_str();
    
    // i i+1 j j+1
    add_feature(21, 3, 0);
    
    feature_buffer[0] = (const unsigned char *)pos_i_minus->c_str();
    feature_buffer[1] = (const unsigned char *)pos_i->c_str();
    
    // i-1 i j j+1
    add_feature(22, 4, 0);
    add_feature(23, 3, 1);
    add_feature(24, 3, 0);
    
    return score;
}

float get_first_order_feature_score(Sentence *sent, int head_index, int dep_index)
{
	float score = 0.0;
	
	score += get_unigram_feature_score(sent, head_index, dep_index);
	score += get_bigram_feature_score(sent, head_index, dep_index);	
	score += get_in_between_feature_score(sent, head_index, dep_index);
	score += get_surrounding_feature_score(sent, head_index, dep_index);
	
	return score;
} 

///////////////////////////////////////////////////////////////////////
// Second order feature

///////////////////////////////////////////////////////////////////////
// Test code

int test()
{
    const char *test_array[] = {"zxcvbnm", "asdfghjkl", "a", ""};
    unsigned long h;
    clock_t start = clock();
    for(unsigned int i = 0;i < (unsigned int)1000000;i++)
    {
        h = hash_feature(17, 1, (const unsigned char **)test_array + 2);
    }
    clock_t end = clock();
    
    DEBUG("%f", (float)(end - start) / 1000000.0);
    DEBUG("%ld, %lx", h, h);
    
    return 0;
}
