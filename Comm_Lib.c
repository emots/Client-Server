#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <datacomm.h>

S32 chid;   // channel id of client
S32 attach; // attach point of the server

void cli_init(void)
{
	S32 init = API_ERROR;

	init = rct_name_open(ATTACH_POINT, EOK);

	if(API_SUCCESS <= init)
	{
		chid = init;
	}
	else if(API_ERROR == init)
	{
		fprintf(stderr,"Problem with channel!\n");
	}
	else if(PROBLEM_PARAMETER == init)
	{
		fprintf(stderr,"Problem with parameters!\n");
	}
	else
	{
		fprintf(stderr,"Unknown problem!\n");
	}
}

void cli_deinit(void)
{
	rct_name_close(chid);
}

void cli_send_request(send_reply_data_t *data)
{
	S32 result_send = API_ERROR;
	if(NULL!= data)
	{
		if((API_ERROR != chid) && (PROBLEM_PARAMETER != chid))
		{
			result_send =(rct_MsgSend(chid, data, sizeof(send_reply_data_t), data, sizeof(send_reply_data_t)));
			if(API_SUCCESS == result_send)
			{
				//everything is ok
			}
			else
			{
				data->result = RES_ERR;
				strncpy(data->reply_str,"Sending the message: FAILED!",STR_RESULT_LENGTH);
			}
		}
		else
		{
			data->result = RES_ERR;
			strncpy(data->reply_str,"Problem with channel id!",STR_RESULT_LENGTH);
		}
	}
	else
	{
		fprintf(stderr,"Send: The argument points to NULL!\n");
	}


}

void cli_check_reply(send_reply_data_t *data)
{

	if(NULL != data)
	{
		cli_send_request(data);
	}
	else
	{
		fprintf(stderr,"Check reply: The argument points to NULL!\n");
	}
}

int ser_init(void)
{
	S8 ret_val = API_ERROR;
	attach = rct_name_attach(NULL,ATTACH_POINT,EOK);
	if(API_SUCCESS <= attach)
	{
		ret_val = API_SUCCESS;
	}
	else
	{
		fprintf(stdout,"Problem with channel!\n");
	}
	return ret_val;
}

void ser_deinit(void)
{
	rct_name_detach(attach,EOK);
}

void ser_receive(send_reply_data_t *rec_data)
{
	if(NULL != rec_data)
	{
		rec_data->handle = rct_MsgReceive(attach, rec_data, sizeof(send_reply_data_t),NULL);

		if(API_SUCCESS <= rec_data->handle)
		{
			//everything is ok
		}
		else
		{
			printf("Have a problem with receive\n");
		}
	}
	else
	{
		fprintf(stderr,"Receive: The argument points to NULL!\n");
	}
}

void ser_reply(send_reply_data_t *rec_data)
{
	S32 check_reply = API_ERROR;
	if(NULL != rec_data)
	{
		check_reply = rct_MsgReply(rec_data->handle, EOK, rec_data, sizeof(send_reply_data_t));

		if(API_SUCCESS == check_reply)
		{
			//everything is ok
		}
		else
		{
			printf("Have a problem with reply\n");
		}
	}
	else
	{
		fprintf(stderr,"Check reply: The argument points to NULL!\n");
	}
}
