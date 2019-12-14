#include <stdlib.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include "../CommLib/datacomm.h"

#define SIZE_INPUT_VALUE 10 // size of the input number from the client
#define MAX_SIZE_BUFF 50 	// max size of buffer

// user case input
#define ADD 1 			// operation add (+)
#define MULT 2 			// operation multiply (*)
#define CONC 3 			// operation concatenate (string A + string B)
#define EXIT 4			// Exit from the application
#define AUTO_TEST 9		// Test with automatic requests with operations from 1 to 3

#define FIRST_OPERAND 1   // The first operand that the client has to enter
#define SECOND_OPERAND 2  // The second operand that the client has to enter

#define RANDOM_LETTER_GEN ((rand()%(122-97 +1)) + 97) // generates random letter using ASCII codes between 97('a') - 122('z')
#define RANDOM_NUMBER_GEN (rand()%1000)				  // generates random number between 0 to 1000
#define RANDOM_OPERATION_GEN ((rand()%3) + 1)		  // generates random operation between 1 to 3
#define RANDOM_STRING_LENGTH_GEN ((rand()%16) + 1)	  // generates random string length between 1 to 16

static void client(void);								// start the client application/start the connection and print the menu and wait to client input
static void operation(operation_t op);					// prepare the request for the operands
static void client_send(send_reply_data_t mssg);		// send the request and if the operation is ASYNC start a threat to wait for reply
static void automatic_test();							// sending automatically requests with operations from 1 to 3
static void wait_for_reply(send_reply_data_t *mssg);	// wait for reply from the server
static void print_reply(send_reply_data_t *mssg);		// print the result

int main(int argc, char *argv[])
{

	fprintf(stdout,"!!! Welcome to the intro_client !!!\n\n");
	client();

	return EXIT_SUCCESS;
}

static void client(void)
{
	cli_init(); //////////////////////client init

	U8 num_choice = 0;
	char input_opr[SIZE_INPUT_VALUE];
	bool is_input_ok = true;
	char buff_format[MAX_SIZE_BUFF];
	S32 i = 0;
	sprintf(buff_format, "%%%ds" , SIZE_INPUT_VALUE);

	while(1)
	{
		fprintf(stdout,"(1) Add 2 numbers\n");
		fprintf(stdout,"(2) Multiply 2 numbers\n");
		fprintf(stdout,"(3) Concatenate 2 strings\n");
		fprintf(stdout,"(4) Exit\n");
		fprintf(stdout,"(9) Automatic test!\n");
		fprintf(stdout,"Enter command:\n ");
		fscanf(stdin, buff_format, input_opr);
		fprintf(stdout,"\n\n");
		is_input_ok = true;

		for(i = 0; i<strlen(input_opr); i++)
		{
			if(!isdigit(input_opr[i]))
			{
				is_input_ok = false;
				break;
			}
		}
		if(true == is_input_ok)
		{
			sscanf(input_opr, "%hhd", &num_choice);
			if(EXIT == num_choice)
			{
				fprintf(stdout,"Goodbye!\n");
				break;
			}
			else if(((num_choice > EXIT ) || (num_choice < ADD)) && (num_choice != AUTO_TEST))
			{
				fprintf(stdout,"Wrong Entry!!!\n");
			}
			else
			{
				switch(num_choice)
				{
					case ADD:
					{
						operation(OP_ADD);
						break;
					}

					case MULT:
					{
						operation(OP_MULT);
						break;
					}

					case CONC:
					{
						operation(OP_CONC);
						break;
					}

					case AUTO_TEST:
					{
						automatic_test();
						break;
					}

					default:
					{
						break;
					}

				};
			}
		}
		else
		{
			printf("Wrong entry!!!\n");
		}
	}
	cli_deinit();/////////////// client deinit
}

static void operation(operation_t op)
{
	send_reply_data_t mssg;
	bool is_operation_ok = true;
	char input_val[SIZE_INPUT_VALUE];
	bool is_input_ok = true;
	U8 opr_num = FIRST_OPERAND;
	S32 i;
	char buff_format[MAX_SIZE_BUFF];
	sprintf(buff_format, "%%%ds" , SIZE_INPUT_VALUE);

	if((OP_ADD == op) || (OP_MULT == op))
	{
		while(1)
		{
			is_input_ok = true;
			if(FIRST_OPERAND == opr_num)
			{
				fprintf(stdout,"Enter operand 1: \n");
			}
			else if(SECOND_OPERAND == opr_num)
			{
				fprintf(stdout,"Enter operand 2: \n");
			}
			else
			{

			}
			fscanf(stdin, buff_format, input_val);

			for(i = 0; i<strlen(input_val); i++)
			{
				if(!isdigit(input_val[i]))
				{
					is_input_ok = false;
					break;
				}
			}
			if(true == is_input_ok)
			{
				if(FIRST_OPERAND == opr_num)
				{
					sscanf(input_val, "%d",(int*) &mssg.number1);
					opr_num = SECOND_OPERAND;
				}
				else if(SECOND_OPERAND == opr_num)
				{
					sscanf(input_val, "%d",(int*) &mssg.number2);
					break;
				}
			}
			else
			{
				fprintf(stdout,"The operand has be an integer! \n");
			}
		}

		if(OP_ADD == op)
		{
			mssg.op_th = OP_TH_SYNC;
		}
		else
		{
			mssg.op_th = OP_TH_ASYNC;
		}
	}
	else if(OP_CONC == op)
	{
		sprintf(buff_format, "%%%ds" , STR_LENGTH);
		fprintf(stdout,"Enter string 1: \n");
		fscanf(stdin, buff_format, mssg.str1);
		fprintf(stdout,"Enter string 2: \n");
		fscanf(stdin, buff_format, mssg.str2);
		mssg.op_th = OP_TH_ASYNC;
	}
	else
	{
		is_operation_ok = false;
	}

	if(is_operation_ok)
	{
		mssg.handle = 0;
		mssg.op_id = op;
		mssg.flag_wait = false;
		client_send(mssg);
	}
	else
	{
		fprintf(stdout,"Unknown operation \n");
	}
}

static void client_send(send_reply_data_t mssg)
{
	fprintf(stdout,"Sending request...\n");
	cli_send_request(&mssg);

	if(RES_DONE == mssg.result)
	{
		print_reply(&mssg);

	}
	else if(RES_WAIT == mssg.result)
	{
		send_reply_data_t *mssg_p;
		mssg_p = malloc(sizeof(send_reply_data_t));
		memcpy(mssg_p, &mssg, sizeof(send_reply_data_t));

		// if the result is in wait state we create and start a threat to wait for the reply with done state
		pthread_attr_t attr;
		pthread_attr_init( &attr );
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED );
		pthread_create( NULL, &attr,(void*)&wait_for_reply, (void*)mssg_p );
	}
	else if(RES_ERR == mssg.result)
	{
		fprintf(stderr,"Problem with request...\n");
		fprintf(stderr,"%s\n",mssg.reply_str);
	}
	else
	{
		fprintf(stderr,"Unknown status ...\n");
	}
}

static void wait_for_reply(send_reply_data_t *mssg)
{
	if(NULL != mssg)
	{
		while(mssg->result != RES_DONE)
		{
			if(RES_WAIT == mssg->result)
			{
				delay(1);// this delay is use to not load the processor
				cli_check_reply(mssg);
			}
			else if(RES_ERR == mssg->result)
			{
				fprintf(stderr,"Problem with request...\n");
				fprintf(stderr,"%s\n",mssg->reply_str);
				break;
			}
			else
			{
				fprintf(stderr,"Unknown status ...\n");
				break;
			}
		}
		print_reply(mssg);
		free(mssg);
	}
	else
	{
		fprintf(stderr,"Wait for reply: argument points to NULL!\n");
	}
}

static void print_reply(send_reply_data_t *mssg)
{
	if(NULL != mssg)
	{
		if(RES_DONE == mssg->result)
		{
			if((OP_ADD == mssg->op_id) || (OP_MULT == mssg->op_id) )
			{
				char opr = '&';
				if(OP_ADD == mssg->op_id)
				{
					opr = '+';
				}
				else if(OP_MULT == mssg->op_id)
				{
					opr = '*';
				}
				fprintf(stdout,"Result from (%d %c %d): %d\n",(int)mssg->number1,opr,(int)mssg->number2,(int)mssg->reply_int);

			}
			else if(OP_CONC == mssg->op_id)
			{
				fprintf(stdout,"Result from conc(a,b): %s\n",mssg->reply_str);
			}
		}
	}
	else
	{
		fprintf(stderr,"Nothing to print from NULL pointer!\n");
	}
}

// this test is only for developer
static void automatic_test()
{
	S8 num_op = 0;
	S32 result = 0;
	bool problem = false;
	S32 j;
	U32 str1_length;
	U32 str2_length;
	char str1[STR_BUFF_LENGTH];
	char str2[STR_BUFF_LENGTH];
	char result_str[STR_BUFF_RESULT_LENGTH];
	srand(time(NULL));

	while(problem == false)
	{
		num_op = RANDOM_OPERATION_GEN;

		send_reply_data_t mssg;
		mssg.handle = 0;
		mssg.op_id = num_op-1;// because the enum start with 0
		mssg.flag_wait = false;

		if(OP_ADD == mssg.op_id)
		{
			mssg.op_th = OP_TH_SYNC;
		}
		else
		{
			mssg.op_th = OP_TH_ASYNC;
		}

		if((ADD == num_op) ||(MULT == num_op))
		{
			mssg.number1 = RANDOM_NUMBER_GEN;
			mssg.number2 = RANDOM_NUMBER_GEN;

			if(ADD == num_op)
			{
				result = mssg.number1 + mssg.number2;
			}
			else if(MULT == num_op)
			{
				result = mssg.number1 * mssg.number2;
			}
		}
		else if(CONC == num_op)
		{
			str1_length = RANDOM_STRING_LENGTH_GEN;

			for(j = 0; j<str1_length; j++)
			{
				str1[j] = RANDOM_LETTER_GEN;
			}
			str1[j] = 0;

			str2_length = RANDOM_STRING_LENGTH_GEN;

			for(j = 0; j<str2_length; j++)
			{
				str2[j] = RANDOM_LETTER_GEN;
			}
			str2[j] = 0;

			strncpy(mssg.str1, str1, STR_BUFF_LENGTH);
			strncpy(mssg.str2, str2, STR_BUFF_LENGTH);

			strncpy(result_str, str1, STR_BUFF_RESULT_LENGTH);
			strncat(result_str, str2, STR_BUFF_RESULT_LENGTH);

			memset(str1, 0, sizeof(str1));
			memset(str2, 0, sizeof(str2));

		}

		fprintf(stderr,"Sending request...%d\n",(int)mssg.code_check);
		cli_send_request(&mssg);

		while(RES_WAIT == mssg.result)
		{
			delay(1); // this delay is use to not load the processor
			cli_check_reply(&mssg);
		}

		if((RES_DONE == mssg.result) && (false == problem))
		{
			if((OP_ADD == mssg.op_id) || (OP_MULT == mssg.op_id) )
			{
				char opr = '&';
				if(OP_ADD == mssg.op_id)
				{
					opr = '+';
				}
				else if(OP_MULT == mssg.op_id)
				{
					opr = '*';
				}
				if(mssg.reply_int == result)
				{
					fprintf(stdout,"Result from (%d %c %d): %d\n\n",(int)mssg.number1,opr,(int)mssg.number2,(int)mssg.reply_int);
				}
				else
				{
					fprintf(stdout,"Result from server is (%d %c %d): %d ,  but the expected value is %d\n\n",(int)mssg.number1, opr, (int)mssg.number2, (int)mssg.reply_int, (int)result);
				}
			}
			else if(OP_CONC == mssg.op_id)
			{

				if(API_SUCCESS == strcmp(mssg.reply_str, result_str))
				{
					fprintf(stdout,"Result from conc(a,b): %s\n\n",mssg.reply_str);
				}
				else
				{
					fprintf(stdout,"Different result!\n\n");
				}
				memset(result_str, 0, STR_BUFF_RESULT_LENGTH);
			}
		}
		else if(RES_ERR == mssg.result)
		{
			fprintf(stderr,"Problem with request...%d\n",(int)mssg.code_check);
			fprintf(stderr,"%s\n",mssg.reply_str);
			problem = true;
		}
		else
		{
			fprintf(stderr,"Unknown status\n");
		}
	}
}


