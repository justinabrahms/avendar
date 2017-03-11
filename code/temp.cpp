#include <cstdlib>
#include <ctime>
#include <iostream>

int dice(int count, int max)
{
    --max;
    int total = 0;
    for (int i = 0; i < count; ++i)
        total += 1 + (rand() % max);

    return total;
}

int main()
{
    srand(time(0));
    int total = 0;
    int runs = 10000;
    int level = 51;
    int multiplier = 2;
    for (int i(0); i < runs; ++i)
    {
        int amt = dice(6, multiplier * level) - dice(6, multiplier * level);
        if (amt <= 1) amt = dice(2, 8);
        std::cout << amt << std::endl;
        total += amt;
    }
    std::cout << (total / runs) << std::endl;
    return 0;
}
