/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{

#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
	fault_va = ROUNDDOWN(fault_va,PAGE_SIZE);
	if(isPageReplacmentAlgorithmFIFO())
	{
		if(wsSize < (curenv->page_WS_max_size))
		{
			struct FrameInfo  *ptrFrameInfo ;
			int perms = pt_get_page_permissions(curenv->env_page_directory, fault_va);
			int ret = allocate_frame(&ptrFrameInfo);
			if (ret == E_NO_MEM){
				panic("ana m4 3arfni");
			}
			map_frame(curenv->env_page_directory,ptrFrameInfo,fault_va,PERM_PRESENT | PERM_WRITEABLE | PERM_USER);
			ptrFrameInfo->va = fault_va;
			int ret_read  =  pf_read_env_page(curenv ,(void *)fault_va ) ;
			int ok  = 0 ;
			ok |=( fault_va >= USTACKBOTTOM && fault_va < USTACKTOP) ;
			ok |=( fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) ;
			if (ret_read == E_PAGE_NOT_EXIST_IN_PF && !ok ){
				// ANA LESA M5LST4 ELBA2E bta3t el heap
				sched_kill_env(curenv->env_id) ;
			}

			struct   WorkingSetElement* WS1 =  env_page_ws_list_create_element(curenv,fault_va) ;

			LIST_INSERT_TAIL(&(curenv->page_WS_list ), WS1) ;

			curenv->page_last_WS_element =  WS1 ;

			if (LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
			   (curenv->page_last_WS_element) = LIST_FIRST(&(curenv->page_WS_list));
			} else {
			   (curenv->page_last_WS_element) =  NULL ;
			}
		}
		else
		{
			//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
			//refer to the project presentation and documentation for details

				//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
				uint32 *ptr_page_table;
				struct WorkingSetElement* victim = LIST_FIRST(&(curenv->page_WS_list));
				struct FrameInfo* ptr_frame_info = get_frame_info(curenv->env_page_directory, victim->virtual_address, &ptr_page_table);
				uint32  perm =  pt_get_page_permissions(curenv->env_page_directory,victim->virtual_address)&PERM_MODIFIED;
				if(perm){
					pf_update_env_page(curenv,victim->virtual_address,ptr_frame_info);
				}
				unmap_frame(curenv->env_page_directory,victim->virtual_address);
				env_page_ws_invalidate(curenv,victim->virtual_address);
	//		    LIST_REMOVE(&(curenv->page_WS_list),victim);
	//			cprintf("shit shit %d\n",LIST_SIZE(&free_frame_list));
				page_fault_handler(curenv , fault_va );
	//			cprintf("shit shit shit %d\n",LIST_SIZE(&free_frame_list));

		}
	}
	else
	{
		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		bool found = 0;
		struct WorkingSetElement* ws;
		LIST_FOREACH(ws,&(curenv->SecondList))
		{
			if(ws->virtual_address == fault_va)
			{
				found = 1;
				break;
			}
		}
		if(found)
		{
			LIST_REMOVE(&(curenv->SecondList),ws);
			if(LIST_SIZE(&(curenv->ActiveList)) == (curenv->ActiveListSize))
			{
				struct WorkingSetElement* overflow_victim = LIST_LAST(&(curenv->ActiveList));
				pt_set_page_permissions(curenv->env_page_directory,overflow_victim->virtual_address,0,PERM_PRESENT);
				LIST_REMOVE(&(curenv->ActiveList),overflow_victim);
				LIST_INSERT_HEAD(&(curenv->SecondList),overflow_victim);
			}
			pt_set_page_permissions(curenv->env_page_directory,fault_va,PERM_PRESENT,0);
			LIST_INSERT_HEAD(&(curenv->ActiveList),ws);
			return;
		}
		if(LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)) < (curenv->ActiveListSize)+(curenv->SecondListSize))
		{
			if(LIST_SIZE(&(curenv->ActiveList)) == (curenv->ActiveListSize))
			{
				struct WorkingSetElement* overflow_victim = LIST_LAST(&(curenv->ActiveList));
				pt_set_page_permissions(curenv->env_page_directory,overflow_victim->virtual_address,0,PERM_PRESENT);
				LIST_REMOVE(&(curenv->ActiveList),overflow_victim);
				LIST_INSERT_HEAD(&(curenv->SecondList),overflow_victim);
			}
			struct FrameInfo  *ptrFrameInfo;
			int ret = allocate_frame(&ptrFrameInfo);
			if (ret == E_NO_MEM){
				panic("ana m4 3arfni 2.0");
			}
			map_frame(curenv->env_page_directory,ptrFrameInfo,fault_va,PERM_PRESENT | PERM_WRITEABLE | PERM_USER);
			ptrFrameInfo->va = fault_va;
			int ret_read  =  pf_read_env_page(curenv ,(void *)fault_va ) ;
			int ok  = 0 ;
			ok |=( fault_va >= USTACKBOTTOM && fault_va < USTACKTOP) ;
			ok |=( fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) ;
			if (ret_read == E_PAGE_NOT_EXIST_IN_PF && !ok ){
				// ANA LESA M5LST4 ELBA2E bta3t el heap
				sched_kill_env(curenv->env_id) ;
			}
			struct   WorkingSetElement* WS1 =  env_page_ws_list_create_element(curenv,fault_va) ;
			LIST_INSERT_HEAD(&(curenv->ActiveList ), WS1);
		}
		else
		{
			// Write your code here, remove the panic and write your code
//			panic("page_fault_handler() LRU Replacement is not implemented yet...!!");

			//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
			uint32 *ptr_page_table;
			struct WorkingSetElement* victim = LIST_LAST(&(curenv->SecondList));
			struct FrameInfo* ptr_frame_info = get_frame_info(curenv->env_page_directory, victim->virtual_address, &ptr_page_table);
			uint32  perm =  pt_get_page_permissions(curenv->env_page_directory,victim->virtual_address)&PERM_MODIFIED;
			if(perm){
				pf_update_env_page(curenv,victim->virtual_address,ptr_frame_info);
			}
			unmap_frame(curenv->env_page_directory,victim->virtual_address);
			env_page_ws_invalidate(curenv,victim->virtual_address);
			page_fault_handler(curenv , fault_va );
		}
	}
}


void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



