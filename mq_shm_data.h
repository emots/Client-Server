#pragma once
// mq
#define RW_PERMISSION_GRANTED (0666) // read and write operation are granted
#define MQ_PRIO (1)

#define NAME_SIZE 20 //the max size of the name of mq
#define ID_SIZE 10  // the max size of unique id which is append to the CLIENT


//shared memory
#define NUM_REQ 50 			// the count of blocks in the shared memory that can store request
#define BUFF_MAX_SIZE 512	// the max size of buffer that store the request

typedef enum status // status of each block in the shared memory
{
	STAT_FREE,		// the block is free and can store data
	STAT_REQ,		// in the block already has data
	STAT_BUSY,		// in the block has data which is in progress
	STAT_DONE,		// in the block has data that everyone is done working with it
	STAT_ERR,		// if there have problem with the data or the size in the block
	STAT_EXX		// this status is use only when the server is stopped, to notify the clients
}status_t;

typedef struct data // the data which can put in shared memory block
{
	status_t status;			// the state of block
	S32 size_of_data_in_buffer;	// the size of data in buffer
	char buffer[BUFF_MAX_SIZE]; // the buffer which stores data
}data_t;

typedef struct shrm_data
{
	pthread_mutex_t mutex;		// mutex
	pthread_cond_t cond;		// conditional variable
	data_t array_data[NUM_REQ]; // array of block that can store data

}shrm_data_t;
