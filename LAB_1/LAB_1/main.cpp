#include <windows.h>
#include <process.h>
#include <iostream>
#include <regex>
#include <fstream>

struct Data
{
	std::string cron;
	std::string exe_path;
};

struct MTime
{
	int hour = 0;
	int min = 0;
	int sec = 0;
};

DWORD WINAPI Cron(void* arg)
{
	Data* data = static_cast<Data*>(arg);
	std::string cron = data->cron;
	std::string exe_path = data->exe_path;

	std::regex reg(R"(((\d\d?-\d\d?)|(\d\d?)|(\*))\/?(\d\d?)?)");
	std::smatch sm;

	int sp_1 = cron.find(" ", 0);
	int sp_2 = cron.find(" ", sp_1 + 1);

	std::string sec = cron.substr(0, sp_1);
	std::string min = cron.substr(sp_1 + 1, sp_2);
	std::string hour = cron.substr(sp_2 + 1);

	int sec_min = 0;
	int sec_max = 0;
	int sec_step = 1;
	int min_min = 0;
	int min_max = 0;
	int min_step = 1;
	int hour_min = 0;
	int hour_max = 0;
	int hour_step = 1;

	if (regex_search(sec, sm, reg)) {
		if (sm[2] != "")
		{
			sec_min = std::stoi(sm[2].str().substr(0, sm[2].str().find('-')));
			sec_max = std::stoi(sm[2].str().substr(sm[2].str().find('-') + 1));
		}
		else if (sm[3] != "")
		{
			sec_min = std::stoi(sm[3].str());
			sec_max = sec_min;
			sec_step = 60;
		}
		else if (sm[4] != "")
		{
			sec_min = 0;
			sec_max = 59;
		}
		if (sm[5] != "")
		{
			sec_step = std::stoi(sm[5]);
			if (sec_min == sec_max)
				sec_max = 59;
		}
	}
	if (regex_search(min, sm, reg)) {
		if (sm[2] != "")
		{
			min_min = std::stoi(sm[2].str().substr(0, sm[2].str().find('-')));
			min_max = std::stoi(sm[2].str().substr(sm[2].str().find('-') + 1));
		}
		else if (sm[3] != "")
		{
			min_min = std::stoi(sm[3].str());
			min_max = min_min;
			min_step = 60;
		}
		else if (sm[4] != "")
		{
			min_min = 0;
			min_max = 59;
		}
		if (sm[5] != "")
		{
			min_step = std::stoi(sm[5]);
			if (min_min == min_max)
				min_max = 59;
		}
	}
	if (regex_search(hour, sm, reg)) {
		if (sm[2] != "")
		{
			hour_min = std::stoi(sm[2].str().substr(0, sm[2].str().find('-')));
			hour_max = std::stoi(sm[2].str().substr(sm[2].str().find('-') + 1));
		}
		else if (sm[3] != "")
		{
			hour_min = std::stoi(sm[3].str());
			hour_max = hour_min;
			hour_step = 24;
		}
		else if (sm[4] != "")
		{
			hour_min = 0;
			hour_max = 23;
		}
		if (sm[5] != "")
		{
			hour_step = std::stoi(sm[5]);
			if (hour_min == hour_max)
				hour_max = 23;
		}
	}

	std::vector<MTime> vect;
	int hour_curr = hour_min;
	int min_curr = min_min;
	int sec_curr = sec_min;
	while (hour_curr <= hour_max)
	{
		while (min_curr <= min_max)
		{
			while (sec_curr <= sec_max)
			{
				MTime time;
				time.sec = sec_curr;
				time.min = min_curr;
				time.hour = hour_curr;
				vect.push_back(time);
				sec_curr += sec_step;
			}
			min_curr += min_step;
			sec_curr = sec_min;
		}
		hour_curr += hour_step;
		min_curr = min_min;
		sec_curr = sec_min;
	}

	/*for (int i = 0; i < vect.size(); i++)
	{
		std::cout << vect[i].hour << ":" << vect[i].min << ":" << vect[i].sec << "\n";
	}*/
	std::string prefix;
	std::string sufix;
	std::string out;

	prefix = "--------------------\nThreadId : " + std::to_string(static_cast<int>(GetCurrentThreadId())) + " ";
	sufix = "--------------------\n";

	SYSTEMTIME time;
	GetLocalTime(&time);
	int f_hour = static_cast<int>(time.wHour);
	int f_min = static_cast<int>(time.wMinute);
	int f_sec = static_cast<int>(time.wSecond);
	int index = 0;
	int size = vect.size();

	out = prefix + "CURR_TIME : " + std::to_string(f_hour) + ":" + std::to_string(f_min) + ":" + std::to_string(f_sec) + "\n" + sufix;
	std::cout << out;

	for (int i = 0; i < size; i++)
	{
		if (vect[i].hour >= f_hour && vect[i].min >= f_min && vect[i].sec > f_sec)
		{
			index = i;
			break;
		}
		else if (vect[i].hour >= f_hour && vect[i].min > f_min)
		{
			index = i;
			break;
		}
		else if (vect[i].hour > f_hour)
		{
			index = i;
			break;
		}
	}

	out = prefix + "NEXT_TIME : " + std::to_string(vect[index].hour) + ":" + std::to_string(vect[index].min) + ":" + std::to_string(vect[index].sec) + "\n" + sufix;
	std::cout << out;

	while (true)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		if (vect[index].hour == static_cast<int>(time.wHour) && vect[index].min == static_cast<int>(time.wMinute) && vect[index].sec == static_cast<int>(time.wSecond))
		{
			STARTUPINFOA startup_info = { sizeof(STARTUPINFOA) };
			PROCESS_INFORMATION process_information;
			char command_line[] = "D:\\Games\\Snake.exe";
			if (!CreateProcessA(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_information))
			{
				std::cout << "Error Create Process\n";
				std::cout << GetLastError();
			}
			else
			{
				std::string info;
				info = "--------------------\nProcessId : " + std::to_string(static_cast<int>(GetProcessId(process_information.hProcess))) + " Priority : " + std::to_string(static_cast<int>(GetPriorityClass(process_information.hProcess))) + "\n" + sufix;
				std::cout << info;
			}
			index = (index + 1) % size;
			out = prefix + "NEXT_TIME : " + std::to_string(vect[index].hour) + ":" + std::to_string(vect[index].min) + ":" + std::to_string(vect[index].sec) + "\n" + sufix;
			std::cout << out;
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	const int max_threads = 10;
	std::fstream file("C:\\sem_5\\OSISP\\LAB_1\\Task.txt");
	std::string input;
	std::string out;
	int curr_threads = 0;
	
	DWORD   dwThreadIdArray[max_threads];
	HANDLE  hThreadArray[max_threads];
	
	for (int i = 0; i < max_threads && std::getline(file, input); i++)
	{
		Data* data = new Data();
		data->cron = input.substr(0, input.find(','));
		data->exe_path = input.substr(input.find(',') + 1);
		
		hThreadArray[i] = CreateThread(NULL, 0, Cron, data, 0, &dwThreadIdArray[i]);
		if (hThreadArray[i] == NULL)
		{
			std::cout << "Error Create Thread\n";
			--i;
			//ExitProcess(3);
		}
		int ThreadId = GetThreadId(hThreadArray[i]);
		int Priority = GetThreadPriority(hThreadArray[i]);
		out = "ThreadId : " + std::to_string(ThreadId) + " Priotity : " + std::to_string(Priority) + "\n";
		std::cout << out;
		++curr_threads;
	}

	WaitForMultipleObjects(curr_threads, hThreadArray, TRUE, INFINITE);

	for (int i = 0; i < curr_threads; i++)
	{
		CloseHandle(hThreadArray[i]);
	}
	return 0;
}