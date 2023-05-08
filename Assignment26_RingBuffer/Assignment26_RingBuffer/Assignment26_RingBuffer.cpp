#include "RingBuffer.h"
#include <iostream>
#include <windows.h>

#define MAX 121 //console width

int main()
{
	RingBuffer buf;
	char enqueue[MAX] = "1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 123456";
	char dequeue[MAX] = { '\0', };
	char peek[MAX] = { '\0', };
	
	int cnt = 0;
	int enqueueRet;
	int dequeueRet;
	int peekRet;

	srand(65532);
	int enqueueIdx = 0;
	int enqueueSize = rand() % (MAX - 1);
	int dequeueSize = rand() % (MAX - 1);

	while (1)
	{	
		enqueueRet = buf.Enqueue(&enqueue[enqueueIdx], enqueueSize);
		if (enqueueRet < 0)
		{
			printf("\n\n========Enqueue Error========");
			break;
		}
		
		if (enqueueIdx + enqueueSize == MAX - 1)
			enqueueIdx = 0;
		else
			enqueueIdx += enqueueSize;

		enqueueSize = rand() % (MAX - 1);
		if (enqueueIdx + enqueueSize > MAX - 1)
			enqueueSize = MAX - enqueueIdx - 1;
		
		if (dequeueSize < buf.GetUseSize())
		{
			if (buf.Peek(peek, dequeueSize) < 0)
			{
				printf("\n\n========Peek Error========");
				break;
			}

			if (buf.Dequeue(dequeue, dequeueSize) < 0)
			{
				printf("\n\n========Dequeue Error========");
				break;
			}

			if (memcmp(peek, dequeue, dequeueSize) != 0)
			{
				printf("\n\n========Peek != Dequeue========");
				break;
			}

			printf("%s", dequeue);
			memset(peek, '\0', MAX);
			memset(dequeue, '\0', MAX);
		}

		dequeueSize = rand() % (MAX - 1);
		cnt++;

		if (GetAsyncKeyState(VK_RETURN))
			break;
	}

	printf("\n");
	printf("\n%d", cnt);
	//printf("\nWrite Pos: %d", buf._writePos);
	//printf("\nRead Pos: %d", buf._readPos);
	printf("\nBuffer Size: %d", buf.GetBufferSize());
	printf("\nUse Size: %d", buf.GetUseSize());
	printf("\nFree Size: %d", buf.GetFreeSize());
	printf("\nDirect Dequeue Size: %d", buf.DirectDequeueSize());
	printf("\nDirect Enqueue Size: %d", buf.DirectEnqueueSize());
	printf("\n");
}

