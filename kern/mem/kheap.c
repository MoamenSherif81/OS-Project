#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

// todo call unmap frame
// we need to know the acutal limits correctly
// check if mo2men added the updated test
// to use those arrays first map the virtual address to page num we start from kheap limit + pagesize as page num 0
const uint32 HEAP_PAGES = 10000000;
//((KERNEL_HEAP_MAX - (kheap_limit + PAGE_SIZE)) / PAGE_SIZE) + 5;
uint32 reserved[10000000];
uint32 last_size[10000000];

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...
	//panic("not implemented yet");

	if(daStart + initSizeToAllocate > daLimit) return E_NO_MEM;

	int numberOfPages = ROUNDUP(initSizeToAllocate / PAGE_SIZE, PAGE_SIZE);

	kheap_start = daStart;
	kheap_break = ROUNDUP(daStart + initSizeToAllocate, PAGE_SIZE);
	kheap_limit = daLimit;

	for(int i = 0; i < numberOfPages; i++){
		uint32 currAddress = daStart + PAGE_SIZE * i;
		struct FrameInfo *newFrameInfoPtr = NULL;
		int returnValue = allocate_frame(&newFrameInfoPtr);
		if(returnValue == E_NO_MEM) return E_NO_MEM;
		else {
			uint32 physical = to_physical_address(newFrameInfoPtr);
			map_frame(ptr_page_directory, newFrameInfoPtr, currAddress, PERM_WRITEABLE);
		}
	}

	initialize_dynamic_allocator(daStart, initSizeToAllocate);

	return 0;
}

void* sbrk(int increment)
{
	//kiss
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */


	uint32 currentBreak = kheap_break;

	if(increment > 0)
	{
		if(ROUNDUP(kheap_break + increment, PAGE_SIZE) > kheap_limit)
			panic("Memory limit exceeded!!!");
		else
		{
			kheap_break = ROUNDUP(kheap_break + increment, PAGE_SIZE);

			for(uint32 i = currentBreak; i <= kheap_break; i += PAGE_SIZE)
			{
				// Start from current sbrk up until the new break, allocating pages all the way
				struct FrameInfo* frameInfoPtr = NULL;
				allocate_frame(&frameInfoPtr);
				map_frame(ptr_page_directory, frameInfoPtr, kheap_break, PERM_WRITEABLE);
			}

			return (void *)currentBreak;
		}
    
	}
	else if(increment == 0)
		return (void *)currentBreak;
	else
	{
		kheap_break = currentBreak + increment; // Subtract increment from current break
		if(kheap_break < kheap_start)
			panic("Memory limit exceeded!!!"); // Case: going lower than start of heap

		if(kheap_break <= currentBreak - PAGE_SIZE)
		{
			// If previous page boundary is crossed, loop over pages
			// until the page with current break
			uint32 decrementedBreak = ROUNDUP(kheap_break, PAGE_SIZE);
			for(uint32 i = currentBreak; i > decrementedBreak; i -= PAGE_SIZE)
			{
				unmap_frame(ptr_page_directory, i);
			}
		}
		currentBreak = kheap_break;
		return (void *)currentBreak;
	}
	//MS2: COMMENT THIS LINE BEFORE START CODING====
	return (void*)-1 ;
	//panic("not implemented yet");
}


void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
		if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
			return alloc_block_FF(size);
		} else {
			unsigned int num_of_pages = ROUNDUP(size ,PAGE_SIZE) / PAGE_SIZE;

			uint32 count = 0 , which = -1;
			bool found = 0;

			for (uint32 page = 0 ; page < 10000000 ; page++) {

				// out of range because HEAP_PAGES isn't so accurate
				if(page * PAGE_SIZE + kheap_limit + PAGE_SIZE >=  KERNEL_HEAP_MAX)
					break;

				if (!reserved[page]) {
					count ++;
					if(which == -1)
						which = page;
				} else {
					count = 0 , which = -1;
				}

				if(count == num_of_pages){
					found = 1;
					break;
				}
			}

			if (found) {
				last_size[which] = num_of_pages;

				for (uint32 i = which ; i < which + num_of_pages ; i++){
					reserved[i] = 1;
					struct FrameInfo *newFrameInfoPtr = NULL;
					uint32 currAddress = kheap_limit + PAGE_SIZE + PAGE_SIZE * i;

					int returnValue = allocate_frame(&newFrameInfoPtr);

					if (returnValue == E_NO_MEM)
						return NULL;
					else {
						map_frame(ptr_page_directory, newFrameInfoPtr, currAddress, PERM_WRITEABLE);
					}
				}

				uint32 address = (kheap_limit + PAGE_SIZE) + PAGE_SIZE * which;

				return (void*) address;
			} else {
				return NULL;
			}
		}
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");

	// no idea what happens if he wants to free before kheap limit
	// no idea what happens if he wants to free before kheap limit
	// no idea what happens if he wants to free before kheap limit
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer
	return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
