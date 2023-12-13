#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP
	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");

	loadAVG = 0;
	num_of_ready_queues = numOfLevels;


//	*quantums = quantum;
//
//	struct Env_Queue newQueue[numOfLevels];
//
//	for(int i = 0; i < numOfLevels; i++)
//		init_queue(&(newQueue[i]));
//
//	env_ready_queues = newQueue;


	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue) * numOfLevels);
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
	kclock_set_quantum(quantum);

	for(int i = 0; i < numOfLevels; i++)
	{
		quantums[i] = quantum;
		init_queue(&(env_ready_queues[i]));
	}

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}


//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");

	int maxPriroty = -1;
	struct Env* returnedEnv;
	struct Env* newEnv;

	for(uint8 i = 0; i < num_of_ready_queues; i++)
	{

		if(env_ready_queues[i].size != 0)
		{
			returnedEnv = LIST_FIRST(&env_ready_queues[i]); // delete process from queue?
			return returnedEnv;
		}
	}


	return NULL;
}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	{


		if(curenv != NULL)
			curenv->recentCPU++;
		// hl value el loadAVG & recentCPU htkoon fixedPoint wla int?
		int64 currentTicks = timer_ticks(); // in ms
		// Update Priority
		if(currentTicks % 4 == 0) // ngeeb el recent abl wla b3d el priority?
		{
			struct Env* newEnv;
			//do at ticks = 0?
			for(int i = 0; i < num_of_ready_queues; i++)
			{
				LIST_FOREACH(newEnv, &(env_ready_queues[i]))
				{
					fixed_point_t CPUTime = fix_int(newEnv->recentCPU);
					fixed_point_t niceVal = fix_int(newEnv->nice);
					fixed_point_t maxPriority = fix_int(PRI_MAX);

					CPUTime = fix_div(CPUTime, 4); // recent / 4
					niceVal = fix_mul(niceVal, 2); // nice * 2

					maxPriority = fix_sub(maxPriority, CPUTime); // PRI_MAX - recent/4
					maxPriority = fix_sub(maxPriority, niceVal); // PRI_MAX - recent/4 - nice*2


					newEnv->priority = fix_trunc(maxPriority);

				}
			}

			// Update Priority for curenv
			fixed_point_t CPUTime = fix_int(curenv->recentCPU);
			fixed_point_t niceVal = fix_int(curenv->nice);
			fixed_point_t maxPriority = fix_int(PRI_MAX);

			CPUTime = fix_div(CPUTime, 4); // recent / 4
			niceVal = fix_mul(niceVal, 2); // nice * 2

			maxPriority = fix_sub(maxPriority, CPUTime); // PRI_MAX - recent/4
			maxPriority = fix_sub(maxPriority, niceVal); // PRI_MAX - recent/4 - nice*2


			curenv->priority = fix_trunc(maxPriority);
		}


		if(currentTicks % 1000 == 0)
		{
			struct Env* newEnv;

			// Update load AVG
			int numOfReadyProcesses = 0;
			for(int i = 0; i < num_of_ready_queues; i++)
			{
				numOfReadyProcesses += queue_size(num_of_ready_queues[i]);
			}

			if(curenv != NULL)
				numOfReadyProcesses++;

			fixed_point_t oldLoadAVG_FP = fix_int(loadAVG);
			fixed_point_t readyProcesses_FP = fix_int(numOfReadyProcesses);

			fixed_point_t coeff_1 = fix_div(fix_int(59), fix_int(60));
			fixed_point_t coeff_2 = fix_div(fix_int(1), fix_int(60));

			coeff_1 = fix_mul(coeff_1, oldLoadAVG_FP);
			coeff_2 = fix_mul(coeff_2, readyProcesses_FP);

			fixed_point_t newLoadAVG_FP = fix_add(coeff_1, coeff_2);

			loadAVG = fix_round(newLoadAVG_FP); // round or trunc?

			// Update recent CPU
			for(int i = 0; i < num_of_ready_queues; i++)
			{
				LIST_FOREACH(newEnv, &(env_ready_queues[i]))
				{
					fixed_point_t loadAVG_FP = fix_int(loadAVG);
					fixed_point_t recentCPU_FP = fix_int(newEnv->recentCPU);
					fixed_point_t nice_FP = fix_int(newEnv->nice);
					fixed_point_t loadAVG_FP_double = fix_mul(fix_int(2), loadAVG_FP); // multiply loadAVG by 2
					fixed_point_t loadConstant = fix_div(loadAVG_FP_double, fix_add(loadAVG_FP_double, fix_int(1)));
					loadConstant = fix_mul(loadConstant, recentCPU_FP); // multiply loadConst by old recent
					recentCPU_FP = fix_add(loadConstant, nice_FP); // add loadConst x old recent to nice value

					newEnv->recentCPU = fix_round(recentCPU_FP);
				}
			}
			// Update for curenv
			fixed_point_t loadAVG_FP = fix_int(loadAVG);
			fixed_point_t recentCPU_FP = fix_int(curenv->recentCPU);
			fixed_point_t nice_FP = fix_int(curenv->nice);
			fixed_point_t loadAVG_FP_double = fix_mul(fix_int(2), loadAVG_FP); // multiply loadAVG by 2
			fixed_point_t loadConstant = fix_div(loadAVG_FP_double, fix_add(loadAVG_FP_double, fix_int(1)));
			loadConstant = fix_mul(loadConstant, recentCPU_FP); // multiply loadConst by old recent
			recentCPU_FP = fix_add(loadConstant, nice_FP); // add loadConst x old recent to nice value

			curenv->recentCPU = fix_round(recentCPU_FP);
}
	}


	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}

