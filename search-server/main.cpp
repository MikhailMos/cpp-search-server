// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271
#include <string>
#include <iostream>

int main(int nNumberofArgs, char *pszArgs[])
{
    using namespace std;

    int count_result = 0;

    for (int i = 1; i <= 1000; i++)
    {
        if (i < 3)
        {
            continue;
        }
        string strNumber = to_string(i);
        for (char tmpChar : strNumber)
        {
            if (tmpChar == '3')
            {
                ++count_result;
                break;
            }
        }
    }

    cout << "Result is "s << count_result << endl;

    return 0;
}

// Закомитьте изменения и отправьте их в свой репозиторий.
