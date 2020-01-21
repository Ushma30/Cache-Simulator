#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

typedef enum {dm, sa} cache_mapping_type;
typedef enum {uc, sc} cache_organization_type;
typedef enum {instruction, data} instruction_type;

typedef struct {
    uint32_t address;
    instruction_type inst_or_data;
} trace_memory_read;

typedef struct {
    int valid;
    int tag;
} cache_block; 

typedef struct {
    cache_block *block_array;
} cache; 

//Global variables declarations:
uint32_t blockSize;
uint32_t cacheSize;
uint32_t maxNumberOfBlocks = 0; //the real number of blocks in cache
int indx; //index
int indexLen; //index length
int offsetLen; //Offset Length
int hit_counter = 0;	//hit counter
cache_mapping_type cache_mapping_v;
cache_organization_type cache_org_v;

/* Unified Cache variables */
cache cache_v;
int unified_cache_counter = 0;
int unified_hit_counter = 0;

/* Split Cache variables */
// variables for Instruction Cache:
cache instruct_cache_v;
int instruct_cache_counter = 0;
int instruct_hit_counter = 0;

// variables for Data Cache:
cache data_cache_v;
int data_cache_counter = 0;
int data_hit_counter = 0;


//**********************************************************************
// Function Name: cache_setup 										   *
// Description: creates a cache of the desired size provided as input  *
// Input: Integer and poiner to a structure							   *
// Return: void 											   		   *
//**********************************************************************
void cache_setup (int max_block_num, cache *cache_size) {
    cache_size->block_array = malloc(max_block_num * (blockSize));

    for (int i= 0; i< max_block_num; i++) {
        cache_size->block_array[i].valid = 0;
        cache_size->block_array[i].tag = 0;
    }
}

//**********************************************************************
// Function Name: read_file 							   *
// Description: Reads the type of instruction executed in the trace    *
// Input: file						  								   *
// Return: structure of trace_memory_read					   		   *
//**********************************************************************
trace_memory_read read_file(FILE *file) {
    
    /* Data read from the file*/
    char max_size[1000000];
    int* op;
    char* max_size_string = max_size;
    trace_memory_read access;

	if (fgets(max_size,1000000, file)!=NULL) {

        /* Get the fetch type */
        op = strtok(max_size_string, " \n");
        if (strcmp(op,"2") == 0) {
            access.inst_or_data = instruction;
        } 
		if (strcmp(op,"0") == 0) {
            access.inst_or_data = data;
        } 
		if (strcmp(op,"1") == 0) {
            access.inst_or_data = data;
        } 
        
        /* Get the fetch type */        
        op = strtok(NULL, " \n");
        access.address = (uint32_t)strtol(op, NULL, 16);

        return access;
    }
}

//**********************************************************************
// Function Name: do_direct_map 									   *
// Description: Function to perform direct mapping in cache 		   *
// Input: structure, int and pointer								   *
// Return: void 													   *
//**********************************************************************
void do_direct_map (trace_memory_read access_v, int index_size, cache * fill_cache) {
    int remove_offset = (access_v.address >> ((int)log2(blockSize))) % (maxNumberOfBlocks);         // removing the offset bits
    int tagAddressLength = (access_v.address >> (((int)log2(blockSize)) + index_size));             // calculate size of the tag
    int fill_cache_counter = 0;                                                                     // initializing various counters
    int fill_hit_counter = 0;

    for (int i= 0; i < maxNumberOfBlocks; i++) {                                                    // search the cache for data
        if (i == remove_offset) {
            fill_cache_counter++;
            if (tagAddressLength == fill_cache->block_array[i].tag) {                               // tag is found in the cache 
                if (fill_cache->block_array[i].valid == 1) {                                     	// valid bit is set equal to 1
                    fill_hit_counter++;                                                             // there's a hit
                } else {                                                                            // valid bit is not equal to 1
                    fill_cache->block_array[i].tag = tagAddressLength;                              // update the tag
                    fill_cache->block_array[i].valid = 1;                                           // change valid bit to 1
                }
            } else {                                                                                // tag is not found in the cache
                fill_cache->block_array[i].tag = tagAddressLength;                                  // update the tag
                fill_cache->block_array[i].valid = 1;                                               // changing the valid bit to 1
            }
        }
    }

    /* Updating the counters: */
    if (fill_cache == &cache_v) {                                                                    // increase the counters for unified cache
        unified_cache_counter += fill_cache_counter;
        unified_hit_counter += fill_hit_counter;
    }
    else if (fill_cache == &instruct_cache_v) {                                                      // increase the counter for instruction cache in split cache
        instruct_cache_counter += fill_cache_counter;
        instruct_hit_counter += fill_hit_counter; 
    } else {                                                                                        // increase the counter for data cache in split cache
        data_cache_counter += fill_cache_counter;
        data_hit_counter += fill_hit_counter;
    }
}

int main(int argc, char *argv[]) {

	if  (argc != 5 ) { 
        printf("Enter the command as per the below instructions:\nCommand: ./Cache_Simulator [Cache Size: 32K] [Block Size: 8|32|128] [Cache Mapping: dm|sa] [Cache Organization: uc|sc]\n");
        exit(0);
    }
    else {

		//info about trace.din:
		//file size is 832477 entries
		//2: instrunction fetch
		//0: data read
		//1: data write
		//Data initialization:
		//preparing i/o files
		FILE *file;
		char *mode = "r";
		FILE *pfout;

		/* Read user inputs */
		/* Set Cache size and Block size */
		cacheSize = atoi(argv[1])*1024;
		blockSize = atoi(argv[2]);

		/* Set Cache Mapping */
		if (strcmp(argv[3], "dm") == 0) {
            cache_mapping_v = dm;
        }
        else if (strcmp(argv[3], "sa") == 0) {
        	cache_mapping_v = sa;
        }

        /* Set Cache Organization */
        if (strcmp(argv[4], "uc") == 0) {
            cache_org_v = uc;
        }
        else if (strcmp(argv[4], "sc") == 0) {
        	cache_org_v = sc;
        }
		
        //opening file for reading
		file = fopen("trace.din",mode);
		if (file == NULL) {
			printf("Can't open input file\n");
			return(0);
		}
		//opening file for writing - used for debugging
		pfout = fopen("out.txt","w");

		if (cache_mapping_v == dm) {
			if (cache_org_v == uc) {
				maxNumberOfBlocks = (int)(cacheSize / blockSize);
				cache_setup(maxNumberOfBlocks, &cache_v);
			}
		else {
			maxNumberOfBlocks = (int)((cacheSize / 2) / blockSize);
			cache_setup(maxNumberOfBlocks, &instruct_cache_v);
			cache_setup(maxNumberOfBlocks, &data_cache_v);
		}
		}

		//long long requiredTag;

		trace_memory_read fetch_type;
		/*Loop through the entire trace file*/
    	while(1) {
        fetch_type = read_file(file);
        if (fetch_type.address == 0)
            break;
			
		if (cache_mapping_v == dm) {
            if (cache_org_v == uc) {
                indexLen = (int)((float)log(maxNumberOfBlocks)/log(2));
                do_direct_map (fetch_type, indexLen, &cache_v);
            } else if (cache_org_v == sc){            
                indexLen = (int)((float)log(maxNumberOfBlocks)/log(2)) - 2;	//4 way associativity
                if (fetch_type.inst_or_data == instruction) {
                    do_direct_map (fetch_type, indexLen, &instruct_cache_v);
                } else {                                                                    
                    do_direct_map (fetch_type, indexLen, &data_cache_v);
                }
            }
    	}
	}


		/* Print the hit and miss rates */

    if (cache_org_v == uc) {
        printf("Unified Cache Fetches: %d\n", unified_cache_counter);
        printf("Unified Cache Hits: %d\n", unified_hit_counter);
        printf("Unified Hit Rate: %1.3f\n", ((double)unified_hit_counter)/unified_cache_counter);
		printf("Unified Cache Misses: %d\n", (unified_cache_counter)-(unified_hit_counter));
        printf("Unified Miss rate: %1.3f\n", (1-((double)unified_hit_counter)/unified_cache_counter));
    } else {                                                                                  // cache_org == sc
        printf("Instruction Cache Fetches: %d\n", instruct_cache_counter);
        printf("Instruction Cache Hits: %d\n", instruct_hit_counter);
        printf("Instruction Hit Rate: %1.3f\n", ((double)instruct_hit_counter)/instruct_cache_counter);
		printf("Instruction Cache Misses: %d\n", (instruct_cache_counter)-(instruct_hit_counter));
        printf("Instruction Miss Rate: %1.3f\n\n", (1.000-((double)instruct_hit_counter)/instruct_cache_counter));
        printf("Data Cache Fetches: %d\n", data_cache_counter);
        printf("Data Cache Hits: %d\n", data_hit_counter);
        printf("Data Hit Rate: %1.3f\n", ((double)data_hit_counter)/data_cache_counter);
		printf("Data Cache Misses: %d\n", (data_cache_counter)-(data_hit_counter));
        printf("Data Miss rate: %1.3f\n", (1.000-((double)data_hit_counter)/data_cache_counter));
    }

    fclose(file);

		
	}
	return 0;
}
//end of file :)
