
#include "glm_parser.h"

unordered_map<unsigned long, float> weight_vector;

float get_weight(unsigned long h)
{
    if(weight_vector.count(h) == 0) return 0.0;
    else return weight_vector.at(h);
}
