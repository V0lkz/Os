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
#include <deque>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>

#include "disk.h"
#include "page_table.h"
#include "program.h"

#ifdef DEBUG    // DEBUG ON
// Print to stderr
#define PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
// Print page table
inline void PT_PRINT(const char *str, struct page_table *pt) {
    std::cout << str << " ----------------------------" << std::endl;
    page_table_print(pt);
    std::cout << "----------------------------------" << std::endl;
}
#else    // DEBUG OFF
#define PRINT(...)
#define PT_PRINT(...)
#endif

using namespace std;

// Contains free frames
std::queue<int> free_frames;
// Contains allocated frames
std::deque<int> used_frames;
// Map from PM frame to VM page
int *frame_mapping;

// Clock Policy
// Inital clock hand position
int clock_hand = 0;
// Tracks use bits for each frame
int *use_bits;

// Prototype for test program
typedef void (*program_f)(char *data, int length);
int (*policy)(struct page_table *) = nullptr;

// Number of virtual pages
int npages;
// Number of physical frames
int nframes;
// Page fault counter
int page_faults = 0;
// Disk read counter
int num_reads = 0;
// Disk write counter
int num_writes = 0;

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

// Page eviction policies

// Chooses random frame
int policy_rand(struct page_table *pt) {
    return rand() % nframes;
}

// Chooses first (oldest) frame
int policy_fifo(struct page_table *pt) {
    int evicted_frame = used_frames.front();
    used_frames.pop_front();
    return evicted_frame;
}

// Chooses last (newest) frame
int policy_lifo(struct page_table *pt) {
    int evicted_frame = used_frames.back();
    used_frames.pop_back();
    return evicted_frame;
}

// Chooses a random read-only frame
int policy_rd_rand(struct page_table *pt) {
    std::vector<int> rdonly_frames;
    std::vector<int> dirty_frames;

    // Iteratre through physical memory frames
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

// Choose a random write frame
int policy_wr_rand(struct page_table *pt) {
    return 0;
}

// Newest frame that was written to
int policy_mrw(struct page_table *pt) {
    for (auto iter = used_frames.end(); iter != used_frames.begin(); iter--) {
        int frame, bits;
        int page = frame_mapping[(*iter)];
        page_table_get_entry(pt, page, &frame, &bits);
        // Check page protections
        if (bits & PROT_WRITE) {
            used_frames.erase(iter);
            return frame;
        }
    }
    // Otherwise fifo
    return policy_fifo(pt);
}

int policy_clock(struct page_table *pt) {
    // Search for a frame with unset use bit
    while (true) {
        // Get frame and page pointed by clock hand
        int frame = clock_hand;
        int page = frame_mapping[frame];

        // Get corresponding page table entry
        int dummy_frame, bits;
        page_table_get_entry(pt, page, &dummy_frame, &bits);

        // Check if use bit is unset
        if (use_bits[frame] == 0) {
            return frame;
        }
        // Otherwise clear the use bit
        else {
            use_bits[frame] = 0;
            clock_hand = (clock_hand + 1) % nframes;
        }
    }
}

// Page fault handler
void page_fault_handler(struct page_table *pt, int page) {
    PT_PRINT("Before", pt);

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
            PRINT("Policy: Evicted page %d/frame %d\n", evicted_page, evicted_frame);
            // Write frame to disk if the frame is dirty
            if (evicted_bits & PROT_WRITE) {
                disk_write(disk, evicted_page, phys_mem + evicted_frame * PAGE_SIZE);
                num_writes++;
            }
            // Set the new frame to the evicted frame
            page_table_set_entry(pt, evicted_page, 0, 0);
            new_frame = evicted_frame;
        }
        // Read frame from disk into physical memory and update page table
        disk_read(disk, page, phys_mem + new_frame * PAGE_SIZE);
        page_table_set_entry(pt, page, new_frame, PROT_READ);
        // Update policy variables
        frame_mapping[new_frame] = page;
        use_bits[new_frame] = 1;
        used_frames.push_back(new_frame);
        // Update counters
        page_faults++;
        num_reads++;
        PRINT("Handler: Added page %d/frame %d into page table\n", page, new_frame);
    }
    // Check if read-only page was written to
    else if ((bits & PROT_READ) && !(bits & PROT_WRITE)) {
        page_table_set_entry(pt, page, frame, PROT_READ | PROT_WRITE);
        use_bits[frame] = 1;
        PRINT("Handler: PROT_WRITE added to page %d/frame %d\n", page, frame);
    }

    PT_PRINT("After", pt);
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
        policy = policy_fifo;
    } else if (strcmp(algorithm, "lifo") == 0) {
        policy = policy_lifo;
    } else if (strcmp(algorithm, "rd_rand") == 0) {
        policy = policy_rd_rand;
    } else if (strcmp(algorithm, "mrw") == 0) {
        policy = policy_mrw;
    } else if (strcmp(algorithm, "clock") == 0) {
        policy = policy_clock;
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

    // Initialize free frames queue
    for (int i = 0; i < nframes; ++i) {
        free_frames.push(i);
    }
    // Initialize frame mapping and use bits
    frame_mapping = new int[nframes];
    use_bits = new int[nframes];

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
    cout << "Page faults: " << page_faults << " || Disk reads: " << num_reads
         << " || Disk writes: " << num_writes << endl;

    return 0;
}
