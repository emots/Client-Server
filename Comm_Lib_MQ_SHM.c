#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <datacomm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mq_shm_data.h>

#define MQ1

#ifdef MQ

int rct_name_attach(dispatch_t * dpp, const char * name, unsigned flags)
{
	mqd_t queue_server = API_ERROR;
	S8 ret_val = API_ERROR;

	if((NULL == dpp) && (API_SUCCESS == flags))
	{
		queue_server = mq_open(name, O_CREAT, RW_PERMISSION_GRANTED, NULL);
		if(API_SUCCESS <= queue_server)
		{
			ret_val = queue_server;
		}
		else
		{
			printf("Problem with create mq!\n");
			ret_val = API_ERROR;
		}
		mq_close(queue_server);
	}
	else
	{
		ret_val = PROBLEM_PARAMETER;
	}
	return ret_val;
}

int rct_name_detach(int attach, unsigned flags)
{
	S32 ret_val = API_ERROR;

	ret_val = mq_unlink(ATTACH_POINT);

	return ret_val;
}

int rct_MsgReceive(int chid, void * msg, size_t msg_len, unsigned int* msg_prio)
{
	mqd_t mqdes = API_ERROR;
	S8 ret_val = API_ERROR;
	struct mq_attr attr1;

	if(NULL != msg)
	{
		if(NULL == msg_prio)
		{
			mqdes = mq_open(ATTACH_POINT, O_RDWR, RW_PERMISSION_GRANTED, NULL);
			if(API_SUCCESS <= mqdes)
			{
				mq_getattr(mqdes, &attr1);
				if(API_SUCCESS <= (mq_receive(mqdes,(char*) msg, attr1.mq_msgsize, NULL)))
				{
					ret_val = API_SUCCESS;
				}
				else
				{
					perror("Problem with receiving:");
				}
			}
			else
			{
				perror("Problem with opening mq:");
			}
			mq_close(mqdes);
		}
		else
		{
			fprintf(stderr,"Problem with msg_prio!\n");
		}
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}

	return ret_val;
}

int rct_MsgReply(int rcvid, int status, void *msg, int size_of_buffer)
{
	S8 ret_val = API_ERROR;
	mqd_t mqdes = API_ERROR;

	if(NULL != msg)
	{
		char name_mq[NAME_SIZE];
		char unique_num_mq[ID_SIZE];
		strcpy(name_mq,CLIENT);
		itoa(((send_reply_data_t*)msg)->client_id, unique_num_mq, ID_SIZE);
		strcat(name_mq, unique_num_mq);
		mqdes = mq_open(name_mq, O_RDWR, RW_PERMISSION_GRANTED, NULL);
		if(API_SUCCESS <= mqdes)
		{
			if( API_SUCCESS <= (mq_send(mqdes,(char*)msg, size_of_buffer, MQ_PRIO)))
			{
				ret_val = API_SUCCESS;
			}
			else
			{
				perror("Problem with reply:");
			}
		}
		else
		{
			perror("Problem with opening mq:");
		}
		mq_close(mqdes);
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}

	return ret_val;
}

int rct_MsgSend( int chid, void *smsg, int sbytes, void *rmsg, int rbytes )
{
	mqd_t queue_server = API_ERROR;
	mqd_t queue_client = API_ERROR;
	S8 ret_val = API_ERROR;
	struct mq_attr attr1;

	if((NULL != smsg) && (NULL != rmsg))
	{
		queue_server = mq_open(ATTACH_POINT, O_RDWR, RW_PERMISSION_GRANTED, NULL);
		if(API_SUCCESS <= queue_server)
		{
			((send_reply_data_t*)smsg)->client_id = chid;

			if(API_SUCCESS <= (mq_send(queue_server, (char*)smsg, sbytes, MQ_PRIO)))
			{
				ret_val = API_SUCCESS;
			}
			else
			{
				perror("Problem with send:");
			}
		}
		else
		{
			perror("Problem with open queue_server:");
		}
		mq_close(queue_server);


		if( API_SUCCESS == ret_val)
		{
			char name_mq[NAME_SIZE];
			char unique_num_mq[ID_SIZE];
			strcpy(name_mq,CLIENT);
			itoa(chid,unique_num_mq, ID_SIZE);
			strcat(name_mq, unique_num_mq);

			queue_client = mq_open(name_mq, O_RDWR, RW_PERMISSION_GRANTED, NULL);
			if(API_SUCCESS <= queue_client)
			{

				mq_getattr(queue_client, &attr1);
				if((API_SUCCESS <= mq_receive(queue_client,(char*)rmsg, attr1.mq_msgsize, NULL)))
				{
					//everything is ok
				}
				else
				{
					printf("Problem with receiving!\n");
					ret_val = API_ERROR;
				}
			}
			else
			{
				printf("Problem with open queue_client!\n");
				ret_val = API_ERROR;
			}
			mq_close(queue_client);
		}
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}

	return ret_val;
}

int rct_name_open(const char * name_t, unsigned flags)
{
	S32 ret_val = API_ERROR;
	ret_val = getpid();

	if(API_SUCCESS == flags)
	{
		mqd_t queue_client;
		char name_mq[NAME_SIZE];
		char unique_num_mq[ID_SIZE];
		strcpy(name_mq,CLIENT);
		itoa(ret_val, unique_num_mq, ID_SIZE);
		strcat(name_mq, unique_num_mq);

		queue_client = mq_open(name_mq, O_CREAT, RW_PERMISSION_GRANTED, NULL);
		if(API_SUCCESS <= queue_client)
		{
			//everything is ok
		}
		else
		{
			perror("Problem with create mq:");
			ret_val = API_ERROR;
		}
		mq_close(queue_client);
	}
	else
	{
		ret_val = PROBLEM_PARAMETER;
	}
	return ret_val;
}

int rct_name_close(int chid)
{
	S8 ret_val = API_ERROR;

	char name_mq[NAME_SIZE];
	char unique_num_mq[ID_SIZE];
	strcpy(name_mq,CLIENT);
	itoa(chid, unique_num_mq, ID_SIZE);
	strcat(name_mq, unique_num_mq);

	if(API_SUCCESS <= mq_unlink(name_mq))
	{
		ret_val = API_SUCCESS;
	}
	else
	{
		perror("Problem with close mq:");
	}
	return ret_val;
}

#else // SHARED MEMORY

shrm_data_t *shr_p; // the shared memory pointer

int rct_name_attach(dispatch_t * dpp, const char * name, unsigned flags)
{
	S32 file_desc = API_ERROR;
	S32 res_ftrunc = API_ERROR;
	S8 ret_val = API_ERROR;

	if((NULL == dpp) && (API_SUCCESS == flags))
	{
		file_desc = shm_open(name, O_RDWR | O_CREAT | O_TRUNC, RW_PERMISSION_GRANTED);
		if(API_SUCCESS <= file_desc)
		{
			res_ftrunc = ftruncate(file_desc,sizeof(shrm_data_t));

			if(API_SUCCESS <= res_ftrunc)
			{
				shr_p = mmap (NULL, sizeof(shrm_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, file_desc, 0);
				if(MAP_FAILED != shr_p)
				{
					pthread_mutexattr_t attr;
					pthread_mutexattr_init(&attr);
					pthread_mutexattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);
					pthread_mutex_init(&shr_p->mutex,&attr);

					pthread_condattr_t attrc;
					pthread_condattr_init(&attrc);
					pthread_condattr_setpshared(&attrc,PTHREAD_PROCESS_SHARED);
					pthread_cond_init(&shr_p->cond,&attrc);

					ret_val = API_SUCCESS;
				}
				else
				{
					perror("mmap:");
				}
			}
			else
			{
				perror("ftruncate:");
			}
		}
		else
		{
			perror("Shm open:");
		}
	}
	else
	{
		fprintf(stderr,"Problem with arguments!\n");
	}
	return ret_val;
}

int rct_name_detach(int attach, unsigned flags)
{
	U32 i = 0;
	S32 res = API_ERROR;

	for(i = 0; i <= NUM_REQ; i++)
	{
		shr_p->array_data[i].status = STAT_EXX;
	}

	res = shm_unlink(ATTACH_POINT);
	if(API_SUCCESS <= res)
	{
		//everything is ok
	}
	else
	{
		perror("Unlink:");
	}

	return res;

}

int rct_MsgReceive(int chid, void * msg, size_t msg_len, unsigned int* msg_prio )
{

	static S32 i = NUM_REQ;
	S32 ret_val = API_ERROR;
	bool if_data_received = false;
	S32 j = 0;

	if(NULL != msg)
	{
		pthread_mutex_lock(&shr_p->mutex);
		while(false == if_data_received)
		{
			for(j = 0; j < NUM_REQ; j++)
			{
				i++;
				if (i >= NUM_REQ)
				{
					i=0;
				}
				if(STAT_REQ == shr_p->array_data[i].status)
				{
					if(shr_p->array_data[i].size_of_data_in_buffer <= msg_len)
					{
						memcpy(msg, &shr_p->array_data[i].buffer, msg_len);
						shr_p->array_data[i].status= STAT_BUSY;
						ret_val = i;
						if_data_received = true;
						break;
					}
					else
					{
						printf("The receive data is too large!\n");
						if_data_received = true;
						break;
					}
				}
			}
			if(false == if_data_received)
			{
				pthread_cond_wait( &shr_p->cond, &shr_p->mutex);
			}
		}
		pthread_mutex_unlock(&shr_p->mutex);
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}

	return ret_val;
}

int rct_MsgReply(int rcvid, int status, void *msg, int size)
{
	S8 ret_val = API_ERROR;

		if(NULL != msg)
		{
			pthread_mutex_lock(&shr_p->mutex);
			if(BUFF_MAX_SIZE >= size)
			{
				shr_p->array_data[rcvid].size_of_data_in_buffer = size;
				//memset(&shr_p->array_data[rcvid].buffer,0xFF,BUFF_MAX_SIZE); // this is only for test
				memcpy(&shr_p->array_data[rcvid].buffer,msg,size);
				shr_p->array_data[rcvid].status = STAT_DONE;
				ret_val = API_SUCCESS;
			}
			else
			{
				shr_p->array_data[rcvid].status = STAT_ERR;
				printf("The reply data is too large!\n");
			}
			pthread_cond_broadcast(&shr_p->cond);
			pthread_mutex_unlock(&shr_p->mutex);
		}
		else
		{
			fprintf(stderr,"The argument points to NULL!\n");
		}

	return ret_val;
}

int rct_MsgSend( int chid, void *smsg, int sbytes, void *rmsg, int rbytes )
{

	S32 i = 0;
	S8 ret_val = API_ERROR;
	bool is_shrm_full = true;

	if((NULL!= smsg) && (NULL != rmsg))
	{
		pthread_mutex_lock(&shr_p->mutex);
		while(is_shrm_full != false)
		{
			for(i = 0; i < NUM_REQ; i++)
			{
				if(STAT_FREE == shr_p->array_data[i].status)
				{
					if(BUFF_MAX_SIZE >= sbytes)
					{
						shr_p->array_data[i].size_of_data_in_buffer = sbytes;
						//memset(&shr_p->array_data[i].buffer,0xFF,BUFF_MAX_SIZE); //this is only for test
						memcpy(&shr_p->array_data[i].buffer,smsg,sbytes);
						shr_p->array_data[i].status = STAT_REQ;
						is_shrm_full = false;
						break;
					}
					else
					{
						fprintf(stderr,"Too large message to send!\n");
						is_shrm_full = false;
						break;
					}
				}
			}
			if(NUM_REQ == i)
			{
				i--;
			}
			if(true == is_shrm_full && STAT_EXX != shr_p->array_data[i].status)
			{
				pthread_cond_wait( &shr_p->cond, &shr_p->mutex);
			}
			else
			{
				pthread_cond_broadcast(&shr_p->cond);
				if(STAT_EXX == shr_p->array_data[i].status)
				{
					break;
				}
			}
		}

		while(shr_p->array_data[i].status < STAT_DONE)
		{
			pthread_cond_wait( &shr_p->cond, &shr_p->mutex);
		}

		if(STAT_DONE == shr_p->array_data[i].status)
		{
			if(shr_p->array_data[i].size_of_data_in_buffer <= rbytes)
			{
				memcpy(rmsg,&shr_p->array_data[i].buffer,rbytes);
				shr_p->array_data[i].status = STAT_FREE;
				ret_val = API_SUCCESS;
			}
			else
			{
				fprintf(stderr,"Too large message to receive!\n");
			}
		}
		else if(STAT_FREE == shr_p->array_data[i].status)
		{
			fprintf(stderr,"Problem with send! The status stay free!\n");
		}
		else if(STAT_ERR == shr_p->array_data[i].status)
		{
			shr_p->array_data[i].status = STAT_FREE;
			fprintf(stderr,"Problem with receive! The server send ERR!\n");
		}
		else if(STAT_EXX == shr_p->array_data[i].status)
		{
			fprintf(stderr,"The server is not found!\n");
		}
		else
		{
			fprintf(stderr,"Unknown status!\n");
		}
		pthread_mutex_unlock(&shr_p->mutex);
	}
	else
	{
		fprintf(stderr,"The argument points to NULL!\n");
	}

	return ret_val;
}

int rct_name_open(const char * name_t, unsigned flags)
{
	S32 ret_val = API_ERROR;
	S32 file_desc = API_ERROR;
	file_desc = shm_open(name_t, O_RDWR, RW_PERMISSION_GRANTED);
	if(API_SUCCESS <= file_desc)
	{
		shr_p = mmap(NULL, sizeof(shrm_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, file_desc, 0);
		if(MAP_FAILED != shr_p)
		{
			ret_val = file_desc;
		}
		else
		{
			perror("mmap:");
		}
	}
	else
	{
		perror("Shm open:");
	}
	return ret_val;
}

int rct_name_close(int chid)
{
	S32 ret_val = API_ERROR;
	if(API_SUCCESS <= close(chid))
	{
		ret_val = API_SUCCESS;
	}
	else
	{
		fprintf(stderr,"Can't close!\n");
	}

	return ret_val;
}
#endif
