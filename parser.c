
#include "glm_parser.h"

static int max_matrix_size = INIT_SENTENCE_LEN;

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
void resize_eisner_matrix(Sentence *sent)
{
	current_len = sent->word_list.size();
	if(current_len > max_matrix_size)
	{
		free_eisner_matrix(max_matrix_size);
		init_eisner_matrix(current_len);
		max_matrix_size = current_len;
	}
}


