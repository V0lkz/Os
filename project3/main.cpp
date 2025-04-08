/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include <string.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>

#include "disk.h"
#include "page_table.h"
#include "program.h"

using namespace std;

std::queue<int> free_frames;
std::queue<int> fifo_queue;
// Map from PM frame to VM page
int *frame_mapping;

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
void page_fault_handler_example(struct page_table *pt, int page) {
    cout << "page fault on page #" << page << endl;

    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    // Map the page to the same frame number and set to read/write
    page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);

    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}

// TODO - Handler(s) and page eviction algorithms
int policy_rand(struct page_table *pt) {
    return rand() % pt->nframes;
}

int policy_readonly_rand(struct page_table *pt) {
    std::vector<int> rdonly_frames;
    std::vector<int> dirty_frames;

    // Iteratre through physical memory frames
    int nframes = page_table_get_nframes(pt);
    for (int frame = 0; frame < nframes; frame++) {
        // Get page table entry
        int dummy_frame, bits;
        int page = frame_mapping[frame];
        page_table_get_entry(pt, page, &dummy_frame, &bits);
        // Check page protections
        if (bits & PROT_WRITE) {
            dirty_frames.push_back(frame);
        } else {
            rdonly_frames.push_back(frame);
        }
    }

    // Check if there are read-only frames
    if (!rdonly_frames.empty()) {
        return rdonly_frames[rand() % rdonly_frames.size()];
    } else {
        return dirty_frames[rand() % dirty_frames.size()];
    }
}

int policy_FIFO(struct page_table *pt) {
    int evicted_frame = fifo_queue.front();
    fifo_queue.pop();
    return evicted_frame;
}

void page_fault_handler(struct page_table *pt, int page) {
    cout << "Before ----------------------------" << endl;
    page_table_print(pt);

    // Get the corresponding page table entry
    int frame, bits;
    page_table_get_entry(pt, page, &frame, &bits);

    // Check if page is being accessed for the first time
    if (bits == 0) {
        char *phys_mem = page_table_get_physmem(pt);
        int new_frame;
        // Check if there are frame available in queue
        if (!free_frames.empty()) {
            new_frame = free_frames.front();
            free_frames.pop();
        }
        // Otherwise use frame eviction policy
        else {
            // Get evicted page table entry
            int evicted_frame, evicted_bits;
            int evicted_page = frame_mapping[policy(pt)];
            page_table_get_entry(pt, evicted_page, &evicted_frame, &evicted_bits);
            // Write frame to disk if the frame is dirty
            if (evicted_bits & PROT_WRITE) {
                // need corresponding page for write
                disk_write(disk, evicted_page, phys_mem + evicted_frame * PAGE_SIZE);
            }
            // Set the new frame to the evicted frame
            page_table_set_entry(pt, evicted_page, 0, 0);
            new_frame = evicted_frame;
        }
        // Read frame from disk into physical memory and update page table
        disk_read(disk, page, phys_mem + new_frame * PAGE_SIZE);
        page_table_set_entry(pt, page, new_frame, PROT_READ);
        // Update policy variables
        if (policy == policy_FIFO) {
            fifo_queue.push(new_frame);
        }
        // Update counter
        page_faults++;
    }
    // Check if read-only page was written to
    else if ((bits & PROT_READ) && !(bits & PROT_WRITE)) {
        page_table_set_entry(pt, page, frame, PROT_READ | PROT_WRITE);
    }

    cout << "After ----------------------------" << endl;
    page_table_print(pt);
}

int main(int argc, char *argv[]) {
    // Check argument count
    if (argc != 5) {
        cerr << "Usage: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>" << endl;
        exit(1);
    }

    // Parse command line arguments
    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *algorithm = argv[3];
    const char *program_name = argv[4];

    // Validate the algorithm specified
    if (strcmp(algorithm, "rand") == 0) {
        policy = policy_rand;
    } else if (strcmp(algorithm, "fifo") == 0) {
        policy = policy_FIFO;
    } else if (strcmp(algorithm, "readonly_rand") == 0) {
        policy = policy_readonly_rand;
    } else {
        cerr << "ERROR: Unknown algorithm: " << algorithm << endl;
        exit(1);
    }

    // Validate the program specified
    program_f program = NULL;
    if (!strcmp(program_name, "sort")) {
        if (nframes < 2) {
            cerr << "ERROR: nFrames >= 2 for sort program" << endl;
            exit(1);
        }
        program = sort_program;
    } else if (!strcmp(program_name, "scan")) {
        program = scan_program;
    } else if (!strcmp(program_name, "focus")) {
        program = focus_program;
    } else {
        cerr << "ERROR: Unknown program: " << program_name << endl;
        exit(1);
    }

    // TODO - Any init needed
    for (int i = 0; i < nframes; ++i) {
        free_frames.push(i);
    }
    frame_mapping = new int[nframes];

    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk) {
        cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << endl;
        return 1;
    }

    // Create a page table
    struct page_table *pt = page_table_create(npages, nframes, page_fault_handler);
    if (!pt) {
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
