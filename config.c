#include "config.h"
#include <stdio.h>

int load_config(char * cfg_name)
{
	FILE *fp;
	char line[100];
	fp = fopen(cfg_name, "r");
	if (NULL == fp)
	{
		exit(1);
	}
	while (!feof(fp))
	{
		while (fgets(line, 100, fp) != NULL)
		{
			if (strcmp(line, "") == 0)
			{
				continue;
			}
			if (!strstr(line, "="))
			{
				continue;
			}
			char * p = strtok(line, " =\n");
			if (NULL != p)
			{
				if (strcmp(p, "page_size") == 0)
				{
					p = strtok(NULL, " =\n");
					buffer_size = atoi(p);
		//			printf("buffer_size: %d\n", buffer_size);
				}
				else if (strcmp(p, "num_pages") == 0)
				{
					p = strtok(NULL, " =\n");
					num_buffer_pages = atoi(p);
				}
				else if (strcmp(p, "alpha") == 0)
				{
					p = strtok(NULL, " =\n");
					alpha = atof(p);
				}
				else if (strcmp(p, "num_array") == 0)
				{
					p = strtok(NULL, " =\n");
					num_array = atoi(p);
					//				printf("num_arr: %d\n",num_arr);
				}
				else if (strcmp(p, "type_array") == 0)
				{
					int i = 0;
					type_array = malloc(sizeof(char)*num_array);
					p = strtok(NULL, " =,\n");
					while(p!=NULL && strlen(p)!=0)
					{
						strcpy(&type_array[i],p);
						i++;
						p = strtok(NULL, " =,\n");

					}
					//				printf("len_arr: %d\n",len_arr);
				}
				else if (strcmp(p, "len_array") == 0)
				{
					p = strtok(NULL, " =\n");
					len_array = atoi(p);
					//				printf("len_arr: %d\n",len_arr);
				}
				else if (strcmp(p, "io_size") == 0)
				{
					p = strtok(NULL, " =\n");
					io_size = atoi(p);
					//				printf("method: %d\n",method);
				}
				else if (strcmp(p, "method") == 0)
				{
					p = strtok(NULL, " =\n");
					method = atoi(p);
					//				printf("method: %d\n",method);
				}
			}
		}
	}

	fclose(fp);
	return 0;
}
