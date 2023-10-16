#include <iostream>
#include "openssl/sha.h"
#include <sstream>
#include <iomanip>
#include <array>
#include <list>
#include <chrono> // для измерения времени и выполнения операций
#include <thread>
#include <vector>

using namespace std;


#define LENGTH 5
#define FIRST_DIGIT 'a'
#define LAST_DIGIT 'z'

const array<const string, 3> targetHashes = 
{ 
	{
        //"3ac16d51671ad766eb7b5b9edf182a245df80159fd9c7086a39a73e8a0b0401a",
        //"3ac16d51671ad766eb7b5b9edf182a245df80159fd9c7086a39a73e8a0b0401a",
        "5ef1b1016a260f0c229c5b24afe87fe24a68b4c80f6f89535b87e0ca72a08623",
        "1d194f061fd453fa9caaa2a8ec9310e358fb5764c38d80c3d8df4b433fd40245",
        "3ac16d51671ad766eb7b5b9edf182a245df80159fd9c7086a39a73e8a0b0401a"
		//"1115dd800feaacefdf481f1f9070374a2a81e27880f187396db67958b207cbad",
		//"3a7bd3e2360a3d29eea436fcfb7e44c735d117c42d1c1835420b6b9942dd4f1b",
		//"74e1bb62f8dabb8125a58852b63bdf6eaef667cb56ac7f7cdba6d7305c50a22f"
	} 
};

// принимает строку в качестве входных данных и возвращает ее хэш 
string sha256(const string str)
{
    // инициализирует массив беззнаковых символов длиной хеша SHA-256 в байтах
    // массив будет хранить полученный хеш
	unsigned char hash[SHA256_DIGEST_LENGTH];

    // создаем экземпляр структуры SHA256_CTX - хранит состояние алгоритма хеширования
	SHA256_CTX sha256;
    // устанавливает начальное значение хеш-функции
	SHA256_Init(&sha256);
    /*
    функция обновляет SHA-256
    - str.c_str() возвращает указатель на первый символ строки
    - str.size() возвращает длину строки в байтах
     */
	SHA256_Update(&sha256, str.c_str(), str.size());
    // завершает вычисление хэша и сохраняет хэш в массиве hash
	SHA256_Final(hash, &sha256);

    // stringstream - позволяет читать и записывать строку
	stringstream ss;

    // изменяем двоичное значения хеш-функции в hash в шестнадцатеричную строку
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
        /*
        - hex - для установки шестнадцатеричной основы вывода
        - setw(2) - для устанавки ширины вывода в 2 символа
        - setfill('0') - для устанавливки символа заполнения - 0, если вывод меньше 2 символов, то будет заполнен нулями
        - static_cast<int> — для изменения значения одного типа в другой,
        тут изменяется значение байта по индексу i массива hash из unsigned char в int 
        */ 
		ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
	}
	return ss.str();
}

/*
функция перебора
- targetIndex — индекс хеш-значения в массиве targetHashes
- start - начальный индекс для поиска 
- end - конечным индексом для поиска 
*/
string bruteForce(int targetIndex, int start, int end)
{
    // получает значение хэша по индексу targetIndex в массиве targetHashes
    string targetHash = targetHashes[targetIndex];
    // массив символов, для хранения текущего слова, сгенерированного в результате поиска 
    char number[LENGTH + 1];
    // задаем массив number первым словом - aaaaa
    for (int i = 0; i < LENGTH; ++i)
        number[i] = FIRST_DIGIT;
    // для завершения строки
    number[LENGTH] = '\0';

    // перебирает диапазон start, end и генерирует хэш для каждого слова 
    for (int i = start; i < end; ++i)
    {
        // строковая переменная word и содержимым массива number
        string word(number);
        // вычисляет и присваивает хэш слова word переменной hash
        string hash = sha256(word);

        // проверяет совпадает ли хеш текущей word хешу targetHash
        if (hash == targetHash)
        {
            cout << "Found match: " << word << endl;
            return word;
        }
        //cout << word << " - is not" << endl;

        // для увеличения последней цифры текущего word и продолжения поиска
        // задает индекс последней цифры массива number, а после уменьшается на 1 
        int pos = LENGTH;
        // задает значение последней цифры массива number
        char digit = number[--pos];

        // проверяет, равна ли переменная digit константе LAST_DIGIT
        // если переменная digit равна LAST_DIGIT, то digit присваивается значение предыдущей цифры в массиве number
        for (; digit == LAST_DIGIT; digit = number[--pos])
        {
            // если последнюю цифру word невозможно увеличить дальше
            if (pos == 0)
                return "err";
            // если digit достигает последнего символа увеличивает последнюю цифру word и продолжает поиск 
            number[pos] = FIRST_DIGIT;
        }
        // если digit равна LAST_DIGIT, то digit увеличивается, это увеличивает последнюю цифру word
        number[pos] = ++digit;
    }
}

/*
поиск с использованием нескольких потоков
 - numThreads — количество потоков
 - targetIndex - индекс хеша в массиве targetHashes
*/
double runBruteForce(int numThreads, int targetIndex)
{
    /*
    - high_solve_lock — класс часов
    - now - метод для получения текущего времени
    - auto - для автоматического определения типа переменной start на основе типа возвращаемого значения метода now
    */
    auto start = chrono::high_resolution_clock::now();

    // если количество потоков 1
    if (numThreads == 1)
    {
        // от 0 до pow(LAST_DIGIT - FIRST_DIGIT + 1, LENGTH) - занчит, что все пространство поиска просматривается одним потоком
        bruteForce(targetIndex, 0, pow(LAST_DIGIT - FIRST_DIGIT + 1, LENGTH));
    }
    else
    {
        // создаем пустой вектор потоков  
        vector<thread> threads;
        // вычисляем размер фрагмента для каждого потока
        int chunkSize = pow(LAST_DIGIT - FIRST_DIGIT + 1, LENGTH) / numThreads;
        // проходим каждый поток
        for (int i = 0; i < numThreads; ++i)
        {
            // вычисляем индексы начала и конца текущего потока
            int start = i * chunkSize;
            // если текущий поток является последним, конечный индекс устанавливается в pow... иначе : ....
            int end = (i == numThreads - 1) ? pow(LAST_DIGIT - FIRST_DIGIT + 1, LENGTH) : (i + 1) * chunkSize;
            // Создаем новый поток и добавляем его в конец вектора
            threads.emplace_back(bruteForce, i, start, end);
        }

        // Ждем завершения всех потоков
        for (auto& thread : threads)
        {
            // метод join блокирует вызывающий поток до тех пор, пока присоединяемый поток не завершит выполнение
            thread.join();
        }
    }

    auto end = chrono::high_resolution_clock::now();
    // выдает продолжительность времени в секундах 
    chrono::duration<double> elapsed = end - start;
    cout << "Thread " << targetIndex << " took " << elapsed.count() << " seconds" << endl;
    return elapsed.count();
}



int main()
{
    // для хранения найденных слов
    list<string> answers;
 
    int num_threads;
    cout << "Enter num thread: ";
    cin >> num_threads;
    
    // выполняет перебор каждого хеша
    for (int i = 0; i < targetHashes.size(); ++i)
    {
        double totalTime = runBruteForce(num_threads, i);
        answers.push_back("Target " + to_string(i) + " took " + to_string(totalTime) + " seconds");
    }

    cout << '\n';
    for (const auto& answer : answers)
    {
        cout << answer << endl;
    }
    return 0;
}


