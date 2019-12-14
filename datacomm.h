#pragma once

#define ATTACH_POINT "/emilTask"
#define CLIENT "/client"

#define ZERO_TERMIN_SIZE (1)
#define STR_LENGTH (16)
#define STR_BUFF_LENGTH (STR_LENGTH + ZERO_TERMIN_SIZE) // the length of string operators
#define STR_RESULT_LENGTH (STR_LENGTH*2)
#define STR_BUFF_RESULT_LENGTH (STR_LENGTH*2 + ZERO_TERMIN_SIZE) // the length of string result


#define API_ERROR (-1)  		// if the function has error
#define API_SUCCESS (0) 		// if the function was success
#define PROBLEM_PARAMETER (-2)	// if in the function has problem with parameters
#define NOTHING_FOUND (-3)		// this is only for Finding substring functionality (OP_FSUBSS)

//8-bit Types
typedef unsigned char         U8;
typedef signed char           S8;
//16-bit Types
typedef unsigned short		 U16;
typedef signed short		 S16;
//32-bit Types
typedef unsigned long		 U32;
typedef signed long		 	 S32;

typedef enum { false, true } bool;

typedef enum operations
{
	OP_ADD,   // operation add (+)
	OP_MULT,  // operation multiply (*)
	OP_CONC,  // operation concatenate (string A + string B)
	OP_SUB,   // operation subtract (-)
	OP_DIV,   // operation divide (/)
	OP_FSUBSS // operation find substring in a string (if string a is in string b)
}operation_t;

typedef enum result
{
	RES_WAIT, // the server is not done with the reply and return to the client to wait some time and ask again for his reply
	RES_ERR,  // the server has problem with the reply, or internal problem
	RES_DONE  // the server is done with the reply
}ser_result_t;

typedef enum opp_threads
{
	OP_TH_SYNC, // synchronous execution
	OP_TH_ASYNC // asynchronous execution
}opp_threads_t;

typedef struct send_reply_data
{
	S16 handle; 		      		  // this helps to return the right reply for each client
	operation_t op_id;  			  // operation

	// if is mathematical operation
	S32 number1; 					  // operand 1
	S32 number2;		              // operand 2
	S32 reply_int;					  // the result from mathematical operation with operand 1 and operand 2

	// if is string operation
	char str1[STR_BUFF_LENGTH];		  // string operand 1
	char str2[STR_BUFF_LENGTH];		  // string operand 2
	char reply_str[STR_BUFF_RESULT_LENGTH]; // the result from string operation with operand 1 and operand 2

	ser_result_t result;			  // the state from the server for his reply
	opp_threads_t op_th;			  // how to execute the operation
	S32 code_check;					  // the unique code that server gives to client, if the execute of operation is asynchronous, to check if his reply is ready
	bool flag_wait;					  // with this flag the server understand if the incoming request is new or if is checking request
	S32 client_id;					  // this is use only when the communication is with mq, it is generate with getpid() and helps to create unique name for the clients mq
}send_reply_data_t;


#ifndef EOK
#define EOK 0	// flag - everything is ok
#endif

/////////////////////////Comm_lib
void cli_init(void); 								// call function to start communication type and check if everything is ok

void cli_deinit(void);								// call function to end communication and stop the client and check if everything is ok

void cli_send_request(send_reply_data_t *data);		// call function to send request to the server and check if everything is ok

void cli_check_reply(send_reply_data_t *data);		// call cli_send_request

int ser_init(void);									// call function to start communication type and check if everything is ok

void ser_deinit(void);								// call function to end communication and stop the server and check if everything is ok

void ser_receive(send_reply_data_t *rec_data);		// call function to receive the data from client and check if everything is ok

void ser_reply(send_reply_data_t *rec_data);		// call function to reply processed data to client and check if everything is ok


/////////////////////////Comm_lib_MQ_SHM
int rct_name_attach(dispatch_t * dpp, const char * name, unsigned flags); 			// start communication type on server

int rct_name_detach(int attach, unsigned flags);									// end communication on server

int rct_MsgReceive(int chid, void * msg, size_t msg_len, unsigned int* msg_prio );	// receive the data from client

int rct_MsgReply(int rcvid, int status, void *msg, int size);						// reply processed data to client

int rct_MsgSend( int chid, void *smsg, int sbytes, void *rmsg, int rbytes );		// send request to the server

int rct_name_open(const char * name_t, unsigned flags);								// start communication type on client

int rct_name_close(int chid);														// end communication on client
