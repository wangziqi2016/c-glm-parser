
#include "glm_parser.h"

static vector<Section> section_list;
static string data_root_path;

// Scans from the first chaarcter till trailing '\0'
// return true if all characters encountered are one of:
// '\t', ' ', '\n'
static bool is_empty_line(char *line)
{
    while(*line != '\0')
    {
        if(*line != '\n' && *line != ' ' && *line != '\t')
            return false;
        else 
            line++;
    }

    return true;
}

// Load sentence from files
static void load_data_from_file(SectionFile *sf_p)
{
    char line_buffer[LINE_BUFFER_MAX];
    char word_str[WORD_MAX];
    char pos_str[POS_MAX];
    char five_gram_word_str[WORD_MAX];
    
    int father_index;
    int state = STATE_FINISHED;
    
    string *filename_p = &sf_p->filename;

    FILE *fp = fopen(filename_p->c_str(), "r");
    if(fp == NULL) ERROR("Open file %s fails!", filename_p->c_str());

    Sentence st;
    // These two are implied but not in the file
    st.word_list.push_back(string("__ROOT__"));
    st.pos_list.push_back(string("ROOT"));
    // Starts from 1 because ROOT is implied
    int current_index = 1;
    //DEBUG("%s", sf_p->filename.c_str());
    while(!feof(fp))
    {   
        fgets(line_buffer, LINE_BUFFER_MAX, fp);
        if(is_empty_line(line_buffer))
        {
            if(state == STATE_FINISHED) continue;
            
            state = STATE_FINISHED;
            
            sf_p->sentence_list.push_back(st);
            st = Sentence();
            st.word_list.push_back(string("__ROOT__"));
            st.pos_list.push_back(string("ROOT"));
            current_index = 1;
        }
        else
        {
            state = STATE_PROCESSING;
            
            // Extract word, pos and father index
            sscanf(line_buffer, "%s %s %d", word_str, pos_str, &father_index);
            // Extract five gram using format string hach
            sscanf(line_buffer, "%5s", five_gram_word_str);
            
            st.word_list.push_back(string(word_str));
            st.pos_list.push_back(string(pos_str));
            st.five_gram_word_list.push_back(string(five_gram_word_str));
            //DEBUG("%s#####", five_gram_word_str);
            st.gold_edge_list.push_back(Edge(current_index, father_index));
            
            current_index++;
            
            //DEBUG("%s %s %d", word_str, pos_str, father_index);
        }
    }
    
    if(state == STATE_PROCESSING) sf_p->sentence_list.push_back(st);

    fclose(fp);
}

// Read from static global: section_list
static void load_all_sections()
{
    for(int i = 0;i < section_list.size();i++)
    {
        Section *sect_p = &section_list[i];
        for(int j = 0;j < sect_p->file_list.size();j++)
        {
            SectionFile *sf_p =  &(sect_p->file_list[j]);
            
            load_data_from_file(sf_p);
        } 
    }
    
    return;
}

// Construct a section, and fill its file_list with SectionFile instances
// path must have a trailing '/'. This is ensured by its caller
static Section probe_file_in_dir(string path)
{
    DIR *d;
    Section s;

    /* Open the current directory. */
    d = opendir(path.c_str());

    if(!d) ERROR("Not a valid path to search file: %s", path.c_str());
    
    while (1) 
    {
        struct dirent *entry;
        
        entry = readdir(d);
        if(!entry) break;
        
        SectionFile sf;
        string file_name_no_path(entry->d_name);
        // We use full path to make file opening easier
        sf.filename = path + file_name_no_path;
        
        // We do not read . and ..
        if(file_name_no_path == string(".") || 
           file_name_no_path == string("..")) 
            continue;
        
        s.file_list.push_back(sf);
    }
    /* Close the directory. */
    if (closedir(d)) ERROR("Could not close directory: %s", path.c_str());

    return s;
}

// Enumerate all folders, select those whose id is in sections
// root_path: with or without a trailing '/'
static void build_section_list(vector<int> *sections, string root_path)
{
    char section_path[SECTION_PATH_MAX];
    // Keep this var as a global for later use
    data_root_path = root_path;
    
    for(int i = 0;i < sections->size();i++)
    {
        int section_id = (*sections)[i];
        
        // We assume sections are from 00 to 99 - longer section ID will 
        // induce an error
        if(section_id < 0 || section_id > 99) ERROR("Section id %d may be correct"
            " but not supported", section_id);
            
        sprintf(section_path, "%s/%02d/", root_path.c_str(), section_id);

        Section s = probe_file_in_dir(string(section_path));
        s.section_id = section_id;
        
        section_list.push_back(s);
    }
    
    return;
}

// start = a, end = b, return a vector containing a, a+1, .. , b
// Error otherwise
static void section_range(vector<int> *sect_range, int start, int end)
{
    if(start > end) ERROR("Start section could not be larger than end section!", 
                          0);
    
    for(int i = start;i <= end;i++) sect_range->push_back(i);
    
    return;
}

static vector<int> section_range(int start, int end)
{
    vector<int> sect_range;
    
    if(start > end) ERROR("Start section could not be larger than end section!", 
                          0);
    
    for(int i = start;i <= end;i++) sect_range.push_back(i);
    
    return sect_range;
}

/////////////////////////////////////////////////////////
// This is the main procedure exposed to other modules
void load(int start, int end, string root_path)
{
    vector<int> v = section_range(start, end);
    build_section_list(&v, root_path);
    
    load_all_sections();
    
    return;
}

static Section *get_next_section(Context *ctx)
{
    if(ctx->current_section + 1 < section_list.size())
    {
        return &section_list[++ctx->current_section];   
    }
    
    return NULL;
}

static SectionFile *get_next_section_file(Context *ctx)
{
    Section *s_p = &section_list[ctx->current_section];
    while(s_p != NULL) // Until we run out of all sections
    {
        // If we still have file in current section, just retrieve it
        // and return after increment counter
        if(ctx->current_file + 1 < s_p->file_list.size())
        {
            return &s_p->file_list[++ctx->current_file];   
        }
        
        // If we have run out of files in one section, just
        // step to the next section and prepare for the next testing
        ctx->current_file = -1;
        s_p = get_next_section(ctx);
    }

    return NULL;
}

// Returns NULL if we have already reached the end
Sentence *get_next_sentence(Context *ctx)
{
    Section *s_p = &section_list[ctx->current_section];
    SectionFile *sf_p = &s_p->file_list[ctx->current_file];
    
    while(sf_p != NULL)
    {
        if(ctx->current_sentence + 1 < sf_p->sentence_list.size())
        {
            return &sf_p->sentence_list[++ctx->current_sentence];   
        }
        
        ctx->current_sentence = -1;
        sf_p = get_next_section_file(ctx);
    }
    
    return NULL;
}

///////////////////////////////////////////////////
// Profiling functions

static int get_sentence_count()
{
    int total = 0;
    for(int i = 0;i < section_list.size();i++)
    {
        Section *s_p = &section_list[i];
        
        for(int j = 0;j < s_p->file_list.size();j++)
        {
            SectionFile *sf_p = &s_p->file_list[j];
            total += sf_p->sentence_list.size();
        }
    }   
    
    return total;
}

int test()
{
    vector<int> v = section_range(0, 24);
    
    build_section_list(&v, string("D:/c-glm-parser/penn-wsj-deps/"));
    for(int i = 0;i < section_list.size();i++)
    {
        //printf("Section id: %d\n", section_list[i].section_id);
        for(int j = 0;j < section_list[i].file_list.size();j++) 0;
            //printf("\tFile name: %s\n", section_list[i].file_list[j].filename.c_str());   
    }
    
    load_all_sections();
    DEBUG("Load, complete", 0);
    Context ctx;
    int count = 0;
    while(get_next_sentence(&ctx) != NULL) 
    {
        //DEBUG("%d %d %d", ctx.current_section, ctx.current_file, 
        //                  ctx.current_sentence);
        //getchar();
        count++;
    }
    
    DEBUG("Finished, all = %d %d", get_sentence_count(), count);
    getchar();
    
    return 0;
}
