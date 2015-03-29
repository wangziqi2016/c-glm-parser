
#ifndef _GLM_PARSER_H
#define _GLM_PARSER_H

#include <vector>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <unordered_map>


using namespace std;

#define ERROR(s, args...) { \
        fprintf(stderr, "ERROR @ file %s line %d\n", __FILE__, __LINE__); \
        fprintf(stderr, s, args); \
        exit(-1); \
        putchar('\n'); }
        
#define DEBUG(s, args...) { \
        fprintf(stderr, "DEBUG @ file %s line %d\n", __FILE__, __LINE__); \
        fprintf(stderr, s, args); \
        putchar('\n'); }

// Maximum line length from file
#define LINE_BUFFER_MAX 512
#define SECTION_PATH_MAX 1024
#define WORD_MAX 128
#define POS_MAX 16

// State machine used to parse input file
#define STATE_FINISHED 0
#define STATE_PROCESSING 1

// Used by hash function to compute hash value on
// machines with different word size
#define _32BIT_HIGH_FIVE_BITS 0xF8000000
#define _32BIT_LOW_BIT_NUM 27
#define _64BIT_HIGH_FIVE_BITS 0xF800000000000000L
#define _64BIT_LOW_BIT_NUM 59

// Initial eisner matrix size
#define INIT_SENTENCE_LEN 100

struct EisnerNode
{
	float score;
	int mid_index;	
};

struct EdgeRecoveryNode
{
	int s, t, orientation, shape;	
};

typedef EisnerNode *P_EisnerNode;
typedef EisnerNode **PP_EisnerNode;


// How many bits do we leave for type, dir and dist information
#define HASH_MULTIPLIER 2897

struct Edge
{
    int head_index;
    int dep_index;
    
    Edge(int hi, int di) 
    {
        head_index = hi; 
        dep_index = di;
    }
};

struct Sentence
{
    vector<string> word_list;
    vector<string> pos_list;
    vector<string> five_gram_word_list;
    vector<bool> five_gram_flag;

    vector<Edge> gold_edge_list;
};

struct SectionFile
{
    string filename; // File name, no path
    
    vector<Sentence> sentence_list;
};

struct Section
{
    int section_id;
    
    vector<SectionFile> file_list;
};

struct Context
{
    int current_section;    // Index into section_list
    int current_file;       // Index into Section.file_list
    int current_sentence;   // Index into SectionFile.sentence_list
    int total_sentence;     // Total number of sentences processed
    
    float start_time;       // Used for time accoutning
    float end_time;         // Same as above
    
    Context()
    {
        current_section = current_file = 0;
        current_sentence = -1; 
        total_sentence = 0;   
        start_time = end_time = 0.0;
    }
};

struct Feature
{
    string *word ;  
};

///////////////////// Function Dealaration
extern unordered_map<unsigned long, float> weight_vector;

inline float get_weight(unsigned long h)
{
	// To save space, just falsefully return 0.0. Do not add new entry here
    if(weight_vector.count(h) == 0) return 0.0;
    else return weight_vector.at(h);
}

inline unsigned long pack_type_dir_dist(unsigned long type, unsigned long dir_dist)
{
	return (type << 4) | dir_dist;
}

// Assumes: feature_buffer, score, h and dir_dist has already been defined
#define add_feature(type, num, offset) { \
	h = hash_feature(type, num, feature_buffer + offset); \
    score += get_weight(h); \
    h = hash_feature(pack_type_dir_dist(type, dir_dist), num, feature_buffer + offset); \
    score += h; }

#endif
