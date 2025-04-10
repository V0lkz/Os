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

// Contains free frames
std::queue<int> free_frames;
// Contains allocated pages in fifo order
std::deque<int> used_pages;
// Contains dirty pages in fifo order
std::deque<int> dirty_pages;

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
    std::cout << "page fault on page #" << page << std::endl;

    // Print the page table contents
    std::cout << "Before ---------------------------" << std::endl;
    page_table_print(pt);
    std::cout << "----------------------------------" << std::endl;

    // Map the page to the same frame number and set to read/write
    page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);

    // Print the page table contents
    std::cout << "After ----------------------------" << std::endl;
    page_table_print(pt);
    std::cout << "----------------------------------" << std::endl;
}

// Removes a page from a container
template <typename Container>
void remove_page(Container &cont, int page) {
    for (auto iter = cont.begin(); iter != cont.end(); iter++) {
        if ((*iter) == page) {
            cont.erase(iter);
            return;
        }
    }
    std::cerr << "Page does not exist in container" << std::endl;
    abort();
}

// Page eviction policies

// Chooses random page
int policy_rand(struct page_table *pt) {
    int index = rand() % used_pages.size();
    int evicted_page = used_pages[index];
    used_pages.erase(used_pages.begin() + index);
    return evicted_page;
}

// Chooses first (oldest) page
int policy_fifo(struct page_table *pt) {
    int evicted_page = used_pages.front();
    used_pages.pop_front();
    return evicted_page;
}

// Chooses last (newest) page
int policy_lifo(struct page_table *pt) {
    int evicted_page = used_pages.back();
    used_pages.pop_back();
    return evicted_page;
}

// Chooses a random read-only page
int policy_rd_rand(struct page_table *pt) {
    int evicted_page;
    int size = dirty_pages.size();
    // Check if no pages dirty pages
    if (size == 0) {
        evicted_page = policy_rand(pt);
    }
    // Check if all pages are dirty
    else if (size == nframes) {
        evicted_page = policy_rand(pt);
        remove_page(dirty_pages, evicted_page);
    }
    // Otherwise find all pages that are read-only
    else {
        std::vector<int> rdonly;
        // Iterate through all used pages
        for (int i = 0; i < nframes; i++) {
            int frame, bits;
            page_table_get_entry(pt, used_pages[i], &frame, &bits);
            // Check if page is not dirty
            if (!(bits & PROT_WRITE)) {
                rdonly.push_back(i);
            }
        }
        // Choose a random read-only page
        int index = rdonly[rand() % rdonly.size()];
        evicted_page = used_pages[index];
        used_pages.erase(used_pages.begin() + index);
    }
    return evicted_page;
}

// Choose a random dirty page
int policy_wr_rand(struct page_table *pt) {
    // Check if there are dirty pages
    if (!dirty_pages.empty()) {
        int index = rand() % dirty_pages.size();
        int evicted_page = dirty_pages[index];
        dirty_pages.erase(dirty_pages.begin() + index);
        remove_page(used_pages, evicted_page);
        return evicted_page;
    }
    // Otherwise choose a random page
    return policy_rand(pt);
}

// Newest page that was written to
int policy_mrw(struct page_table *pt) {
    // Check if there are dirty pages
    if (!dirty_pages.empty()) {
        int evicted_page = dirty_pages.back();
        dirty_pages.pop_back();
        remove_page(used_pages, evicted_page);
        return evicted_page;
    }
    // Otherwise fifo
    return policy_fifo(pt);
}

// Oldest page that was written to
int policy_lrw(struct page_table *pt) {
    // Check if there are dirty pages
    if (!dirty_pages.empty()) {
        int evicted_page = dirty_pages.front();
        dirty_pages.pop_front();
        remove_page(used_pages, evicted_page);
        return evicted_page;
    }
    // Otherwise fifo
    return policy_fifo(pt);
}

// Clock policy
int policy_clock(struct page_table *pt) {
    // Search for a frame with unset use bit
    while (true) {
        // Get page table entry pointed by clock hand
        int frame, bits;
        int page = used_pages[clock_hand];
        page_table_get_entry(pt, page, &frame, &bits);
        // Check if use bit is unset
        if (use_bits[frame] == 0) {
            used_pages.erase(used_pages.begin() + clock_hand);
            return page;
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
            int evicted_page = policy(pt);
            int evicted_frame, evicted_bits;
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
        used_pages.push_back(page);
        use_bits[new_frame] = 1;
        // Update counters
        page_faults++;
        num_reads++;
        PRINT("Handler: Added page %d/frame %d into page table\n", page, new_frame);
    }
    // Check if read-only page was written to
    else if ((bits & PROT_READ) && !(bits & PROT_WRITE)) {
        page_table_set_entry(pt, page, frame, PROT_READ | PROT_WRITE);
        use_bits[frame] = 1;
        dirty_pages.push_back(page);
        PRINT("Handler: PROT_WRITE added to page %d/frame %d\n", page, frame);
    }

    PT_PRINT("After", pt);
}

int main(int argc, char *argv[]) {
    // Check argument count
    if (argc != 5) {
        std::cerr << "Usage: virtmem <npages> <nframes> "
                  << "<rand|fifo|lifo|rd_rand|wr_rand|mrw|lrw|clock> "
                  << "<sort|scan|focus>" << std::endl;
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
    } else if (strcmp(algorithm, "wr_rand") == 0) {
        policy = policy_wr_rand;
    } else if (strcmp(algorithm, "mrw") == 0) {
        policy = policy_mrw;
    } else if (strcmp(algorithm, "lrw") == 0) {
        policy = policy_lrw;
    } else if (strcmp(algorithm, "clock") == 0) {
        policy = policy_clock;
    } else if (strcmp(algorithm, "custom") == 0) {
        policy = policy_lrw;
    } else {
        std::cerr << "ERROR: Unknown algorithm: " << algorithm << std::endl;
        exit(1);
    }

    // Validate the program specified
    program_f program = NULL;
    if (!strcmp(program_name, "sort")) {
        if (nframes < 2) {
            std::cerr << "ERROR: nFrames >= 2 for sort program" << std::endl;
            exit(1);
        } else if (strcmp(algorithm, "lifo") == 0) {
            std::cerr << "lifo does not work with sort\n";
            return 0;
        }
        program = sort_program;
    } else if (!strcmp(program_name, "scan")) {
        program = scan_program;
    } else if (!strcmp(program_name, "focus")) {
        program = focus_program;
    } else {
        std::cerr << "ERROR: Unknown program: " << program_name << std::endl;
        exit(1);
    }

    // Initialize free frames queue
    for (int i = 0; i < nframes; ++i) {
        free_frames.push(i);
    }
    // Initialize and use bits for clock policy
    use_bits = new int[nframes];

    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk) {
        std::cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << std::endl;
        return 1;
    }

    // Create a page table
    struct page_table *pt = page_table_create(npages, nframes, page_fault_handler);
    if (!pt) {
        std::cerr << "ERROR: Couldn't create page table: " << strerror(errno) << std::endl;
        return 1;
    }

    // fprintf(stderr, "%d %d %s %s\n", npages, nframes, algorithm, program_name);

    // Run the specified program
    char *virtmem = page_table_get_virtmem(pt);
    program(virtmem, npages * PAGE_SIZE);

    // Clean up the page table and disk
    page_table_delete(pt);
    disk_close(disk);

    // Print results
    std::cout << "Page faults: " << page_faults;
    std::cout << " | Disk reads: " << num_reads;
    std::cout << " | Disk writes: " << num_writes << std::endl;

#ifdef DATA
    if (DATA == 1) {
        std::cerr << algorithm << " pf " << page_faults << std::endl;
    } else if (DATA == 2) {
        std::cerr << algorithm << " dr " << num_reads << std::endl;
    } else if (DATA == 3) {
        std::cerr << algorithm << " dw " << num_writes << std::endl;
    }
#endif
    return 0;
}
