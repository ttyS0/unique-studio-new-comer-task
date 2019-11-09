#include <iostream>
#include <vector>
#include <map>
#include <cstring>

class cli
{
private:
	int argument_count;
	char** argument_values;
	std::vector<const char*> argument_seq;
	std::vector<const char*> argument_opt;
	std::map<const char*, bool> argument_used;
	std::map<const char*, bool> argument_string_concat;
	std::map<const char*, bool> argument_switch;
	std::map<const char*, const char*> argument_string;
	std::map<const char*, const char*> argument_description;
public:
	cli(int argc, char** argv)
	{
		argument_count = argc;
		argument_values = argv;
	}
	void add_switch(const char* key, const char* description)
	{
		argument_switch[key] = false;
		argument_description[key] = description;
	}
	void add_option(const char* key, const char* description, const char* default_value)
	{
		argument_opt.push_back(key);
		argument_string[key] = default_value;
		argument_description[key] = description;
		if(description[0] == ' ')
		{
			argument_string_concat[key] = false;
		}
		else
		{
			argument_string_concat[key] = true;
		}
		
	}
	void add_argument(const char* key, const char* description, const char* default_value)
	{
		argument_seq.push_back(key);
		argument_string[key] = default_value;
		argument_used[key] = false;
		argument_description[key] = description;
	}
	bool get_switch(const char* key)
	{
		return argument_switch[key];
	}
	const char* get_option(const char* key)
	{
		return argument_string[key];
	}
	const char* get_argument(const char* key)
	{
		return argument_string[key];
	}
	void parse()
	{
		for(int i = 1; i < argument_count; i++)
		{
			bool found = false;
			for(auto p : argument_switch)
			{
				if(strcmp(argument_values[i], p.first) == 0)
				{
					argument_switch[p.first] = true;
					found = true;
					break;
				}
			}
			if(found) continue;
			for(auto p : argument_opt)
			{
				if(!argument_string_concat[p])
				{
					if(strcmp(argument_values[i], p) == 0)
					{
						if(i + 1 < argument_count)
						{
							argument_string[p] = argument_values[i + 1];
							found = true;
							break;
						}
					}
				}
				else
				{
					const char *s1 = p, *s2 = argument_values[i];
					bool eq = true;
					bool null = true;
					while((*s1 != '\0') && (*s2 != '\0'))
					{
						null = false;
						if(*s1++ != *s2++)
						{
							eq = false;
							break;
						}
					}
					if(eq && !null)
					{
						argument_string[p] = strtok(argument_values[i], p);
						found = true;
						break;
					}
				}
			}
			if(found) continue;
			for(auto p : argument_seq)
			{
				if(argument_used[p] == false)
				{
					argument_string[p] = argument_values[i];
					argument_used[p] = true;
					break;
				}
			}
		}
	}
	void help()
	{
		std::cout << "Usage: " << argument_values[0];
		for(auto p : argument_switch)
		{
			std::cout << " [" << p.first << "]";
		}
		for(auto p : argument_opt)
		{
			std::cout << " [" << p << (argument_string_concat[p] ? "" : " ") << "...]";
		}
		for(auto p : argument_seq)
		{
			std::cout << " " << p;
		}
		std::cout << std::endl;
		for(auto p : argument_switch)
		{
			std::cout << "\t" << p.first << "\t" << argument_description[p.first] << std::endl;
		}
		for(auto p : argument_opt)
		{
			std::cout << "\t" << p << "\t" << (argument_string_concat[p] ? argument_description[p] : argument_description[p] + 1) << std::endl;
		}
		for(auto p : argument_seq)
		{
			std::cout << "\t" << p << "\t" << argument_description[p] << std::endl;
		}
	}
};
