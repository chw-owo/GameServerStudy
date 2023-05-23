#include <iostream>

void DecimalToBinary(unsigned char input)
{
	unsigned char flag = 0;
	printf("%u 의 바이너리 : ", input);

	for (short i = (sizeof(input) * 8) - 1; i >= 0; i--)
	{
		flag = 1 << i;

		if (input & flag)
			printf("%u", 1);
		else
			printf("%u", 0);
	}
	printf("\n");
}

void BitController()
{
	unsigned short value = 0;
	unsigned short flag = 0;
	unsigned char index, input;

	while (true)
	{
		printf("비트위치: ");
		scanf_s("%u", &index);

		printf("OFF/ON [0,1] : ");
		scanf_s("%u", &input);

		if (index < 1 || index > sizeof(value) * 8)
		{
			printf("비트 범위를 초과하였습니다.\n\n");
			continue;
		}

		if (input == 0)
			value &= ~(1 << (index - 1));
		else
			value |= 1 << (index - 1);

		for (char i = 0; i < sizeof(value) * 8; i++)
		{
			flag = 1 << i;

			if (value & flag)
				printf("%u 번 Bit : ON\n", i + 1);
			else
				printf("%u 번 Bit : OFF\n", i + 1);
		}
		printf("\n");
	}
}

void ByteController()
{
	unsigned int value = 0;
	unsigned char index, input, tmp;

	while (true)
	{
		input = 0;

		printf("위치 (1~4) : ");
		scanf_s("%u", &index);

		printf("값 [0~255] : ");
		scanf_s("%u", &input);

		if (index < 1 || index > sizeof(value))
		{
			printf("위치 범위를 초과하였습니다.\n\n");
			continue;
		}

		if (input < 0 || input > 0xff)
		{
			printf("데이터 범위를 초과하였습니다.\n\n");
			continue;
		}


		value &= ~(0xff << ((index - 1) * 8));
		value |= input << ((index - 1) * 8);

		for (char i = 0; i < sizeof(value); i++)
		{
			tmp = value >> (i * 8);
			printf("%u 번째 바이트 값 : %u \n", i + 1, tmp);
		}

		printf("\n전체 4바이트 값 : 0x%p \n\n", value);
	}
}

int main()
{
	DecimalToBinary(40);
	//BitController();
	//ByteController();
}