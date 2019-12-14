#include <stdlib.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include<time.h>
#include <signal.h>
#include "../CommLib/datacomm.h"

typedef struct node{		// the linked list of request to process
	int code;				// unique code that every request has, for check
	send_reply_data_t data; // the request
	struct node *next;		// pointer to next request in the list
}node_t;

U32 count = 0;   											// unique code foe every request in the list
bool if_term = false; 										// if is different from 0 the server is stopped
static node_t *root = NULL;									// the pointer to first element in the list
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;   // mutex to lock the data when it is in processing

static void server (void); 										// start the server - start communication type and wait to receive data to process it
static int find_subs(char*str1 , char*str2);					// find substring into string
static void process_data(send_reply_data_t *rec_data);			// in this function the data is adjust for sync or async processing
static node_t* add_request(send_reply_data_t *rec_data);		// add request in linked list if it is async operation
static void check_cli_request(send_reply_data_t *rec_data);		// check the linked list if there has a request for the client
static void operations(send_reply_data_t *rec_data);			// do the operation with the operands in request
static void sig_handler(int signo);								// handle with signals for stopping the server

int main(int argc, char *argv[])
{
	printf("Welcome to the intro_server\n");
	server();

	return EXIT_SUCCESS;
}

static void server (void)
{
	signal(SIGINT, sig_handler);  //wait for signal Ctrl + C
	signal(SIGTSTP, sig_handler); // wait for signal Ctr + Z

	if(API_SUCCESS == ser_init())
	{
		send_reply_data_t *rec_data;
		pthread_mutex_init(&mutex,NULL);
		while(false == if_term)
		{
			rec_data = malloc(sizeof(send_reply_data_t));

			ser_receive(rec_data);

			if(API_SUCCESS <= rec_data->handle)
			{
				pthread_attr_t attr;
				pthread_attr_init( &attr );
				pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED );

				if(false == rec_data->flag_wait) // if the flag_wait is false we start a thread which will process the data that we receive from client
				{
					count++;
					rec_data->code_check = count;
					pthread_create( NULL, &attr,(void*)&process_data, (void*)rec_data );
				}
				else							// if the flag_wait is true we start a thread which will check the linked list
				{								// about if the reply is done for this specific client
					pthread_create( NULL, &attr,(void*)&check_cli_request, (void*)rec_data );
				}
			}
			else
			{
				break;
			}
		}
	}
	ser_deinit();
}

static int find_subs(char*str, char*sub)
{
	S32 i = 0;
	S32 j = 0;
	S32 ret_val = API_ERROR;
	bool sub_exist = false;
	bool found = false;

	if(strlen(str) >= strlen(sub))
	{
		for(i =0; i<strlen(str); i++)
		{
			//found = true;
			for(j = 0; j < strlen(sub); j++)
			{
				if(str[j+i] != sub[j])
				{
					found = false;
					break;
				}
				else
				{
					found = true;
				}
				sub_exist = true;
			}
			if(true == found)
			{
				ret_val = i;
				break;
			}
		}
		if(false == sub_exist)
		{
			ret_val = NOTHING_FOUND;
		}
	}
	else
	{
		ret_val = API_ERROR;
	}
	return ret_val;
}

static void process_data(send_reply_data_t *rec_data)
{
	if(NULL != rec_data)
	{
		node_t *request_p;
		switch(rec_data->op_th)
		{
			case OP_TH_SYNC:
			{
				operations(rec_data);
				ser_reply(rec_data);
				break;
			}
			case OP_TH_ASYNC:
			{
				rec_data->result = RES_WAIT;
				rec_data->flag_wait = true;
				request_p = add_request(rec_data);
				ser_reply(rec_data);
				operations(&request_p->data);
				break;
			}
			default:
				break;
		}
		free(rec_data);
	}
	else
	{
		fprintf(stderr,"NULL pointer in process data!\n");
	}
}

static node_t* add_request(send_reply_data_t *rec_data)
{
	node_t *current = NULL;
	if(NULL != rec_data)
	{
		pthread_mutex_lock(&mutex);

		current = malloc(sizeof(node_t));
		current->next = root;
		root = current;
		root->data = *rec_data;
		root->code = rec_data->code_check;

		pthread_mutex_unlock(&mutex);
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}

	return current;
}

static void check_cli_request(send_reply_data_t *rec_data)
{
	node_t *previous = NULL;
	bool find = false;
	if(NULL != rec_data)
	{
		pthread_mutex_lock(&mutex);
		node_t *current = root;
		if(NULL != root)
		{
			S32 c = 0; // this is only for check whether the list comes in cycles
			while(c<100000)
			{
				c++;
				if(current->code == rec_data->code_check)
				{
					find = true;
					break;
				}
				else if(NULL != current->next)
				{
					previous = current;
					current = current->next;
				}
				else
				{
					break;
				}
			 }

			if ((c >= 100000) && (false == find)) // this is only for check whether the list comes in cycles
				{
					fprintf(stderr,"Internal server error 1\n");
				}

			if(true == find)
			{
				current->data.handle = rec_data->handle;
				ser_reply(&current->data);

				if(RES_DONE == current->data.result)
				{
					if(NULL == previous)
					{
						root = current->next;
					}
					else
					{
						previous->next = current->next;
					}
					free(current);
				}
			}
			else
			{
				rec_data->result = RES_ERR;
				strncpy(rec_data->reply_str, "Invalid request! 1", STR_RESULT_LENGTH);
				ser_reply(rec_data);
			}
		}
		else
		{
			rec_data->result = RES_ERR;
			strncpy(rec_data->reply_str, "Invalid request! 2", STR_RESULT_LENGTH);
			ser_reply(rec_data);
		}
		pthread_mutex_unlock(&mutex);
		free(rec_data);
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}
}

static void operations(send_reply_data_t *rec_data)
{
	if(NULL != rec_data)
	{
		char result[STR_BUFF_RESULT_LENGTH] = {0};
		if(OP_ADD == rec_data->op_id)
		{
			int result = rec_data->number1 + rec_data->number2;
			rec_data->reply_int = result;
			rec_data->result = RES_DONE;
		}
		else if(OP_MULT == rec_data->op_id)
		{
			int result = rec_data->number1 * rec_data->number2;
			rec_data->reply_int = result;
			rec_data->result = RES_DONE;
		}
		else if(OP_CONC == rec_data->op_id)
		{
			strncpy(result, rec_data->str1, STR_LENGTH);
			strncat(result, rec_data->str2, STR_LENGTH);
			strncpy(rec_data->reply_str, result, STR_RESULT_LENGTH);
			rec_data->result = RES_DONE;
		}
		else if(OP_SUB == rec_data->op_id)
		{
			int result = rec_data->number1 - rec_data->number2;
			rec_data->reply_int = result;
			rec_data->result = RES_DONE;
		}
		else if(OP_DIV == rec_data->op_id)
		{
			int result;

			if(0 == rec_data->number2)
			{
				strncpy(rec_data->reply_str,"Can't divide!",STR_RESULT_LENGTH);
				rec_data->result = RES_ERR;
			}
			else
			{
				result = rec_data->number1 / rec_data->number2;
				rec_data->reply_int = result;
				rec_data->result = RES_DONE;
			}
		}
		else if(OP_FSUBSS == rec_data->op_id)
		{
			int result = find_subs(rec_data->str1, rec_data->str2);
			rec_data->reply_int = result;
			rec_data->result = RES_DONE;
		}
	}
	else
	{
		fprintf(stderr,"Can't do operation with NULL pointer!\n");
	}
}

static void sig_handler(int signo)
{
	if ((SIGINT == signo) || (SIGTSTP == signo))
	{
		if_term = true;
	}
	else
	{

	}
}
