#include <iostream>
using namespace std;

class CustomRand 
{
public:
    CustomRand()
    {
        _seed = 1;
    }

    CustomRand(int seed)
    {
        _seed = seed;
    }

    int operator()()
    {
        int random; 
        random = _seed * 0x343FD;
        random += 0x269EC3;
        _seed = random;
        random >>= 0x10;
        random &= 0x7FFF;

        return random;
    }

private:
    int _seed;
};

void OriginRandom()
{
    srand(time(NULL));
    for(int i= 0; i < 10; i++)
        printf("origin: %d\n", rand());

}
void CustomRandom()
{
    CustomRand customRand(time(NULL));

    for (int i = 0; i < 10; i++)
        printf("custom: %d\n", customRand());      
}

int main()
{
    OriginRandom();
    printf("\n");
    CustomRandom();
}

