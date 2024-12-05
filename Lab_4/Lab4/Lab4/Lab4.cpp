#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

int philosopher_count;
HANDLE* forks;
CRITICAL_SECTION* forkCriticalSections;
int eating_time;        // Время еды (в мс)
int thinking_time;      // Время размышлений (в мс)
int test_duration;      // Длительность теста (в с)
int timeout_time;       // Время тайм-аута для второй стратегии
int retry_time;         // Время ожидания перед повторной попыткой


bool running = true;


vector<long long> wait_times;
vector<bool> is_wait;
vector<int> eat_counts;
vector<int> think_counts;

enum SyncMode {
    MUTEX,
    SEMAPHORE,
    CRITICAL_SECTION_MODE
};

enum Strategy {
    NORMAL,
    TIMEOUT,
    FORK_ACTOR
};

SyncMode mode;
Strategy strategy;
double variance = 0.5;

int get_random_time(int base_time) {
    int min_time = base_time * (1.0 + variance);
    int max_time = base_time * (1.0 - variance);
    return min_time + rand() % (max_time - min_time + 1);
}

void eat(int philosopher) {
    cout << "Philosopher " << philosopher << " is trying to eat." << endl;

    auto wait_start = chrono::high_resolution_clock::now();

    if (strategy == NORMAL) {
        if (mode == MUTEX) {
            WaitForSingleObject(forks[philosopher], INFINITE);
            WaitForSingleObject(forks[(philosopher + 1) % philosopher_count], INFINITE);
        }
        else if (mode == SEMAPHORE) {
            WaitForSingleObject(forks[philosopher], INFINITE);
            WaitForSingleObject(forks[(philosopher + 1) % philosopher_count], INFINITE);
        }
        else if (mode == CRITICAL_SECTION_MODE) {
            EnterCriticalSection(&forkCriticalSections[philosopher]);
            EnterCriticalSection(&forkCriticalSections[(philosopher + 1) % philosopher_count]);
        }
    }
    else if (strategy == TIMEOUT) {
        bool success = false;
        while (!success) {
            // Захват левой вилки
            if (mode == MUTEX) {
                WaitForSingleObject(forks[philosopher], INFINITE);
            }
            else if (mode == SEMAPHORE) {
                WaitForSingleObject(forks[philosopher], INFINITE);
            }
            else if (mode == CRITICAL_SECTION_MODE) {
                EnterCriticalSection(&forkCriticalSections[philosopher]);
            }

            // Попытка захвата правой вилки с тайм-аутом
            if (mode == MUTEX) {
                success = (WaitForSingleObject(forks[(philosopher + 1) % philosopher_count], timeout_time) == WAIT_OBJECT_0);
            }
            else if (mode == SEMAPHORE) {
                success = (WaitForSingleObject(forks[(philosopher + 1) % philosopher_count], timeout_time) == WAIT_OBJECT_0);
            }
            else if (mode == CRITICAL_SECTION_MODE) {
                success = TryEnterCriticalSection(&forkCriticalSections[(philosopher + 1) % philosopher_count]);
            }

            if (!success) {
                // Если не удалось захватить правую вилку, освобождаем левую и ждём
                if (mode == MUTEX) {
                    ReleaseMutex(forks[philosopher]);
                }
                else if (mode == SEMAPHORE) {
                    ReleaseSemaphore(forks[philosopher], 1, NULL);
                }
                else if (mode == CRITICAL_SECTION_MODE) {
                    LeaveCriticalSection(&forkCriticalSections[philosopher]);
                }

                // Ждем перед повторной попыткой
                Sleep(retry_time);
            }
        }
    }
    else if (strategy == FORK_ACTOR) {
        // Стратегия "мудрый-философ"

        // Захватываем левую вилку
        if (mode == MUTEX) {
            WaitForSingleObject(forks[philosopher], INFINITE);
        }
        else if (mode == SEMAPHORE) {
            WaitForSingleObject(forks[philosopher], INFINITE);
        }
        else if (mode == CRITICAL_SECTION_MODE) {
            EnterCriticalSection(&forkCriticalSections[philosopher]);
        }

        bool success = false;

        while (!success)
        {
            // Попытка захватить правую вилку
            if (mode == MUTEX) {
                success = (WaitForSingleObject(forks[(philosopher + 1) % philosopher_count], 10) == WAIT_OBJECT_0);
            }
            else if (mode == SEMAPHORE) {
                success = (WaitForSingleObject(forks[(philosopher + 1) % philosopher_count], 10) == WAIT_OBJECT_0);
            }
            else if (mode == CRITICAL_SECTION_MODE) {
                success = TryEnterCriticalSection(&forkCriticalSections[(philosopher + 1) % philosopher_count]);
            }

            // Если правую вилку не удается взять и левый сосед ждет, передаем ему вилку
            if (!success) {
                is_wait[philosopher] = true;
                if (wait_times[(philosopher_count + philosopher - 1) % philosopher_count]) {
                    // Левый сосед ждет
                    // Освобождаем левую вилку и передаем её соседу
                    is_wait[philosopher] = false;
                    if (mode == MUTEX) {
                        ReleaseMutex(forks[philosopher]);
                        Sleep(10);
                        WaitForSingleObject(forks[philosopher], INFINITE);
                    }
                    else if (mode == SEMAPHORE) {
                        ReleaseSemaphore(forks[philosopher], 1, NULL);
                        Sleep(10);
                        WaitForSingleObject(forks[philosopher], INFINITE);
                    }
                    else if (mode == CRITICAL_SECTION_MODE) {
                        LeaveCriticalSection(&forkCriticalSections[philosopher]);
                        Sleep(10);
                        EnterCriticalSection(&forkCriticalSections[philosopher]);
                    }
                }
            }
            else {
                is_wait[philosopher] = false;
            }
        }
    }

    // Считаем время ожидания
    auto wait_end = chrono::high_resolution_clock::now();
    long long wait_duration = chrono::duration_cast<chrono::milliseconds>(wait_end - wait_start).count();
    wait_times[philosopher] += wait_duration;

    // Философ ест
    cout << "Philosopher " << philosopher << " is eating." << endl;
    Sleep(get_random_time(eating_time));
    eat_counts[philosopher]++;

    // Освобождаем вилки
    if (mode == MUTEX) {
        ReleaseMutex(forks[philosopher]);
        ReleaseMutex(forks[(philosopher + 1) % philosopher_count]);
    }
    else if (mode == SEMAPHORE) {
        ReleaseSemaphore(forks[philosopher], 1, NULL);
        ReleaseSemaphore(forks[(philosopher + 1) % philosopher_count], 1, NULL);
    }
    else if (mode == CRITICAL_SECTION_MODE) {
        LeaveCriticalSection(&forkCriticalSections[philosopher]);
        LeaveCriticalSection(&forkCriticalSections[(philosopher + 1) % philosopher_count]);
    }

    cout << "Philosopher " << philosopher << " finished eating." << endl;
}

DWORD WINAPI philosopher_thread(LPVOID param) {
    int philosopher = *(int*)param;

    while (running) {
        eat(philosopher);
        cout << "Philosopher " << philosopher << " is thinking." << endl;
        Sleep(get_random_time(thinking_time));
        think_counts[philosopher]++;
    }

    return 0;
}

void setup(SyncMode syncMode, int count) {
    philosopher_count = count;
    mode = syncMode;

    forks = new HANDLE[philosopher_count];
    forkCriticalSections = new CRITICAL_SECTION[philosopher_count];
    wait_times.resize(philosopher_count, 0);
    is_wait.resize(philosopher_count, false);
    eat_counts.resize(philosopher_count, 0);
    think_counts.resize(philosopher_count, 0);

    if (mode == MUTEX) {
        for (int i = 0; i < philosopher_count; ++i) {
            forks[i] = CreateMutex(NULL, FALSE, NULL);
        }
    }
    else if (mode == SEMAPHORE) {
        for (int i = 0; i < philosopher_count; ++i) {
            forks[i] = CreateSemaphore(NULL, 1, 1, NULL);
        }
    }
    else if (mode == CRITICAL_SECTION_MODE) {
        for (int i = 0; i < philosopher_count; ++i) {
            InitializeCriticalSection(&forkCriticalSections[i]);
        }
    }
}

void cleanup() {
    if (mode == CRITICAL_SECTION_MODE) {
        for (int i = 0; i < philosopher_count; ++i) {
            DeleteCriticalSection(&forkCriticalSections[i]);
        }
        delete[] forkCriticalSections;
    }
    else {
        for (int i = 0; i < philosopher_count; ++i) {
            CloseHandle(forks[i]);
        }
        delete[] forks;
    }
}

void stop_after_duration(int duration) {
    Sleep(duration * 1000);
    running = false;
}

int main() {
    int syncChoice, strategyChoice;

    cout << "Choose synchronization method (0: Mutex, 1: Semaphore, 2: Critical Section): ";
    cin >> syncChoice;
    SyncMode syncMode = static_cast<SyncMode>(syncChoice);

    cout << "Choose strategy (0: Normal, 1: Timeout, 2: Philosopher): ";
    cin >> strategyChoice;
    strategy = static_cast<Strategy>(strategyChoice);

    cout << "Enter number of philosophers (minimum 2): ";
    cin >> philosopher_count;
    if (philosopher_count < 2) {
        cout << "Invalid number of philosophers. Minimum is 2." << endl;
        return 0;
    }

    cout << "Enter time for eating (in milliseconds): ";
    cin >> eating_time;
    cout << "Enter time for thinking (in milliseconds): ";
    cin >> thinking_time;

    if (strategy == TIMEOUT) {
        cout << "Enter timeout duration (timeout_time) for waiting the second fork (in milliseconds): ";
        cin >> timeout_time;
        if (timeout_time == -1) {
            timeout_time = eating_time;
        }
        cout << "Enter retry wait time (retry_time) after failing to get the second fork (in milliseconds): ";
        cin >> retry_time;
        if (retry_time == -1) {
            retry_time = eating_time * (0.1 + 0.5 * variance);
        }
    }

    cout << "Enter test duration (in seconds): ";
    cin >> test_duration;

    setup(syncMode, philosopher_count);

    vector<HANDLE> philosopherThreads(philosopher_count);
    vector<int> philosopherIds(philosopher_count);

    for (int i = 0; i < philosopher_count; ++i) {
        philosopherIds[i] = i;
        philosopherThreads[i] = CreateThread(NULL, 0, philosopher_thread, &philosopherIds[i], 0, NULL);
    }

    thread timer_thread(stop_after_duration, test_duration);

    WaitForMultipleObjects(philosopher_count, philosopherThreads.data(), TRUE, INFINITE);
    timer_thread.join();

    cout << "\nTotal wait times, eat counts, and think counts for each philosopher:" << endl;
    for (int i = 0; i < philosopher_count; ++i) {
        cout << "Philosopher " << i
            << " waited for " << wait_times[i] << " milliseconds, "
            << "ate " << eat_counts[i] << " times, "
            << "and thought " << think_counts[i] << " times." << endl;
    }

    cleanup();

    return 0;
}
