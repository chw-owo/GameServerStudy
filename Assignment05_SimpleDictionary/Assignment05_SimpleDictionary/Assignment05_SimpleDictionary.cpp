#include <iostream>

#define DICT_MAX 10
#define WORD_MAX 10

#define IN_MAX 200
#define OUT_MAX 400

enum LANG
{
	ENG,
	KOR,
	LANG_MAX
};

char dict[DICT_MAX][LANG_MAX][WORD_MAX] =
{
	{"i", "나"},
	{"you", "너"},
	{"am", "은(는)"},
	{"is", "은(는)"},
	{"are", "은(는)"},
	{"boy", "소년"},
	{"girl", "소녀"},
	{"a", "하나의"},
	{"man", "남자"},
	{"woman", "여자"}
};

void Translate()
{
	bool found = false;
	int buffer_idx = 0;

	while(1)
	{
		char input[IN_MAX] = "";
		char buffer[IN_MAX] = "";
		char output[OUT_MAX] = "";

		// input section
		printf("100자 이내 영어 문장을 입력해주세요.\n");
		scanf_s("%[^\n]s", &input, IN_MAX);
		_strlwr_s(input);
		rewind(stdin);

		for (int i = 0; i <= (int)strlen(input); i++)
		{
			// logic section
			// 1. tokenize
			if ( input[i] != ' ' && input[i] != '\0')
			{
				buffer[buffer_idx] = input[i];
				buffer_idx++;
			}
			else
			{
				buffer[buffer_idx] = '\0';

				//// 2. translate
				for (int dict_idx = 0; dict_idx < DICT_MAX; dict_idx++)
				{
					if (strcmp(dict[dict_idx][ENG], buffer) == 0)
					{
						strcat_s(output, dict[dict_idx][KOR]);
						strcat_s(output, " ");
						found = true;
						break;
					}
				}

				if (found == false)
				{
					strcat_s(output, buffer);
					strcat_s(output, " ");
				}

				buffer_idx = 0;
				found = false;
			}
		}
		
		// render section
		printf("%s\n\n", output);
		buffer_idx = 0;
	}
}

int main()
{
	Translate();
}
