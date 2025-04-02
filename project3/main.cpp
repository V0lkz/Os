/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <cassert>
#include <iostream>
#include <string.h>
#include <queue>
#include <vector>
#include <cstdlib>
#include <unordered_map>

using namespace std;
std::queue<int> free_frames;
std::queue<int> fifo_queue;
std::unordered_map<int, int> frame_page;
// Prototype for test program
typedef void (*program_f)(char *data, int length);
int (*policy)(struct page_table *) = nullptr;
// Number of physical frames
int npages;
int nframes;
int page_faults = 0;
// Pointer to disk for access from handlers
struct disk *disk = nullptr;

// Simple handler for pages == frames
void page_fault_handler_example(struct page_table *pt, int page)
{
    cout << "page fault on page #" << page << endl;

    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    // Map the page to the same frame number and set to read/write
    // TODO - Disable exit and enable page table update for example
    //exit(1);
    page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);


    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}

// TODO - Handler(s) and page eviction algorithms
int policy_rand(struct page_table *pt){
    return rand() % nframes;    
}

int policy_readonly_rand(struct page_table *pt){
    std::vector<int> read_only;
    std::vector<int> dirty;

    for(int frame = 0; frame < nframes; frame++){
        int page = frame_page[frame];
        int dummy_frame, bits;
        page_table_get_entry(pt, page,&dummy_frame, &bits);

        if(bits & PROT_WRITE){
            dirty.push_back(frame);
        }else{
            read_only.push_back(frame);
        }
    }

    if(!read_only.empty()){
        return read_only[rand() % read_only.size()];
    }else{
        return dirty[rand() % dirty.size()];
    }
}

int polic_FIFO(struct page_table *pt){
    int evicted_frame = fifo_queue.front();
    fifo_queue.pop();
    return evicted_frame;
}

void page_fault_handler(struct page_table *pt, int page){
    cout << "Before ----------------------------" << endl;
    page_table_print(pt);

    int frame, bits;
    char *phys_mem = page_table_get_physmem(pt);
    page_table_get_entry(pt, page, &frame, &bits);
    // if the page is being access for the first time
    if(bits == 0){
        page_faults++;

        int use_frame;

        if(!free_frames.empty()){               //checks for available frame
            use_frame = free_frames.front();
            free_frames.pop();
        }else{
            int evicted_page = frame_page[policy(pt)];
            int evicted_frame, evicted_bits;

            page_table_get_entry(pt, evicted_page, &evicted_frame, &evicted_bits);

            //if the evicted is dirt write to disk
            if(evicted_bits & PROT_WRITE){
                disk_write(disk, evicted_page, phys_mem + evicted_frame * PAGE_SIZE);
            }
            
            //reset the mapping of the evicted page
            page_table_set_entry(pt, evicted_page, 0, 0);
            frame_page.erase(evicted_frame);
            use_frame = evicted_frame;
        }
        char *phys_mem = page_table_get_physmem(pt);
        disk_read(disk, page, phys_mem + use_frame * PAGE_SIZE);
        page_table_set_entry(pt, page, use_frame, PROT_READ);
        frame_page[use_frame] = page;

        if(policy == polic_FIFO){
            fifo.queue.push(use_frame);
        }
    }
    //Page fault if the application attempts to write to read-only file
    else if((bits & PROT_READ) && !(bits & PROT_WRITE)){
        page_table_set_entry(pt,page,frame,PROT_READ | PROT_WRITE);
    }

    cout << "After ----------------------------" << endl;
    page_table_print(pt);
}



int main(int argc, char *argv[])
{
    // Check argument count
    if (argc != 5)
    {
        cerr << "Usage: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>" << endl;
        exit(1);
    }

    // Parse command line arguments
    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *algorithm = argv[3];
    const char *program_name = argv[4];

    // Validate the algorithm specified
    if(strcmp(algorithm, "rand") == 0){
        policy = policy_rand;
    }else if(strcmp(algorithm, "fifio") == 0){
        policy = polic_FIFO;
    }else if(strcmp(algorithm, "readonly_rand") == 0){
        policy = policy_readonly_rand;
    }else{
        cerr << "ERROR: Unknown algorithm: " << algorithm << endl;
        exit(1);
    }

    // Validate the program specified
    program_f program = NULL;
    if (!strcmp(program_name, "sort"))
    {
        if (nframes < 2)
        {
            cerr << "ERROR: nFrames >= 2 for sort program" << endl;
            exit(1);
        }

        program = sort_program;
    }
    else if (!strcmp(program_name, "scan"))
    {
        program = scan_program;
    }
    else if (!strcmp(program_name, "focus"))
    {
        program = focus_program;
    }
    else
    {
        cerr << "ERROR: Unknown program: " << program_name << endl;
        exit(1);
    }

    // TODO - Any init needed

    for (int i = 0; i < nframes; ++i) {
        free_frames.push(i);
    }
    
    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk)
    {
        cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << endl;
        return 1;
    }

    // Create a page table
    struct page_table *pt = page_table_create(npages, nframes, page_fault_handler /* TODO - Replace with your handler(s)*/);
    if (!pt)
    {
        cerr << "ERROR: Couldn't create page table: " << strerror(errno) << endl;
        return 1;
    }

    // Run the specified program
    char *virtmem = page_table_get_virtmem(pt);
    program(virtmem, npages * PAGE_SIZE);

    // Clean up the page table and disk
    page_table_delete(pt);
    disk_close(disk);

    return 0;
}
