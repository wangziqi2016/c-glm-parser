
#include "glm_parser.h"

static int max_matrix_size = INIT_SENTENCE_LEN;
float (*arc_weight)(Sentence *sent, int head_index, int dep_index) = get_first_order_feature_score;

// Eisner Matrix
static PP_EisnerNode ***e;

/*
    def init_eisner_matrix(self):
        self.e = <PP_EisnerNode***>malloc(self.n*sizeof(PP_EisnerNode**))
        cdef int i, j, k, l
        for i in range(self.n):
            self.e[i] = <P_EisnerNode***>calloc(self.n,sizeof(P_EisnerNode**))
            for j in range(self.n):
                self.e[i][j] = <P_EisnerNode**>calloc(2,sizeof(P_EisnerNode*))
                for k in range(2):
                    self.e[i][j][k] = <P_EisnerNode*>calloc(2,sizeof(P_EisnerNode))
                    for l in range(2):
                        self.e[i][j][k][l] = <P_EisnerNode>calloc(1,sizeof(EisnerNode))
        return

*/

// Must be called at least once to init a parsing matrix
void init_eisner_matrix(int n)
{	
	e = (PP_EisnerNode ***)malloc(sizeof(PP_EisnerNode **) * n);
	for(int i = 0;i < n;i++)
	{
		e[i] = (P_EisnerNode ***)malloc(sizeof(P_EisnerNode **) * n);
		for(int j = 0;j < n;j++)
		{
			e[i][j] = (P_EisnerNode **)malloc(sizeof(P_EisnerNode *) * 2);
			for(int k = 0;k < 2;k++)
			{
				e[i][j][k] = (P_EisnerNode *)malloc(sizeof(P_EisnerNode) * 2);
				for(int l = 0;l < 2;l++)
				{
					e[i][j][k][l] = (P_EisnerNode)malloc(sizeof(EisnerNode));
				}
			}
		}
	}
	
	return;
}

/*
	def delete_eisner_matrix(self):
        cdef int i, j, k
        for i in range(self.n):
            for j in range(self.n):
                for k in range(2):
                    for l in range(2):
                        free(self.e[i][j][k][l])
                    free(self.e[i][j][k])
                free(self.e[i][j])
            free(self.e[i])
*/
void free_eisner_matrix(int n)
{
	for(int i = 0;i < n;i++)
	{
		for(int j = 0;j < n;j++)
		{
			for(int k = 0;k < 2;k++)
			{
				for(int l = 0;l < 2;l++)
				{
					free(e[i][j][k][l]);
				}
				free(e[i][j][k]);
			}
			free(e[i][j]);
		}
		free(e[i]);
	}
	
	return;
}

// For efficiency consideration, we do not allocate and free memory everytime
// We only keep the largest size eisner matrix in memory, and if the target sentence
// is longer, we free the memory blocks and re-allocate
// It mush be called on before every parsing procedure begins
void resize_eisner_matrix(Sentence *sent)
{
	int current_len = sent->word_list.size();
	if(current_len > max_matrix_size)
	{
		free_eisner_matrix(max_matrix_size);
		init_eisner_matrix(current_len);
		max_matrix_size = current_len;
	}
	
	return;
}

// max_index is used to return a value
float combine_triangle(Sentence *sent, int head, int modifier, int *max_index_p)
{
	//assert(head != modifier)
	int s, t, q;
	
	if(head < modifier) s = head, t = modifier;
	else t = head, s = modifier;
	
	float edge_score = arc_weight(sent, head, modifier);
	int max_index = s;
	
	float max_score = e[s][s][1][0]->score + e[q + 1][t] [0][0]->score + edge_score;
	float current_score;
	
	for(q = s + 1;q < t;q++)
	{
		current_score = e[s][q][1][0]->score + e[q + 1][t][0][0]->score + edge_score;
		if(max_score < current_score)
		{
			max_score = current_score;
			max_index = q;
		}
	}
	
	*max_index_p = max_index;
	return max_score;
}

float combine_left(int s, int t, int *max_index_p)
{       
    int max_index = s;
    float max_score = e[s][s][0][0]->score + e[s][t][0][1]->score;

    float current_score;
    for(int q = s + 1;q < t;q++)
	{
    	current_score = e[s][q][0][0]->score + e[q][t][0][1]->score;
        if(max_score < current_score)
        {
            max_score = current_score;
            max_index = q;
        }
	}
	
	*max_index_p = max_index;
    return max_score; 
}

float combine_right(int s, int t, int *max_index_p)
{
    int max_index = s + 1;
    float max_score = e[s][s + 1][1][1]->score + e[s + 1][t][1][0]->score;
    
    float current_score;
    for(int q = s + 2;q <= t;q++)
    {
        current_score = e[s][q][1][1]->score + e[q][t][1][0]->score;
        if(max_score < current_score)
        {
            max_score = current_score;
            max_index = q;
        }
	}
	
    *max_index_p = max_index;
    return max_score;
}

inline void split_right_triangle(EdgeRecoveryNode *node, 
								 EdgeRecoveryNode *left, 
								 EdgeRecoveryNode *right)
{
    int q = e[node->s][node->t][1][0]->mid_index;
        
    *left = EdgeRecoveryNode(node->s, q, 1, 1);
    *right = EdgeRecoveryNode(q, node->t, 1, 0);

    return;
}

void split_left_triangle(EdgeRecoveryNode *node,
						 EdgeRecoveryNode *left,
						 EdgeRecoveryNode *right)
{
    int q = e[node->s][node->t][0][0]->mid_index;
        
    *left = EdgeRecoveryNode(node->s, q, 0, 0);
    *right = EdgeRecoveryNode(q, node->t, 0, 1);
    
	return;
}

static int edge_list_index = 0;
Edge edge_list[MAX_EDGE_LIST_SIZE];

void split_right_trapezoid(EdgeRecoveryNode *node,
						   EdgeRecoveryNode *left,
						   EdgeRecoveryNode *right)
{
    Edge ee(node->s, node->t);
    edge_list[edge_list_index++] = ee;

    int q = e[node->s][node->t][1][1]->mid_index;
    *left = EdgeRecoveryNode(node->s, q, 1, 0);
    *right = EdgeRecoveryNode(q + 1, node->t, 0, 0);
        
    return;
}

void split_left_trapezoid(EdgeRecoveryNode *node,
						   EdgeRecoveryNode *left,
						   EdgeRecoveryNode *right)
{
    Edge ee(node->t, node->s);
    edge_list[edge_list_index++] = ee;

    int q = e[node->s][node->t][0][1]->mid_index;
    *left = EdgeRecoveryNode(node->s, q, 1, 0);
    *right = EdgeRecoveryNode(q + 1, node->t, 0, 0);
        
    return;
}
