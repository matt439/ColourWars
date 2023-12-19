#include "pch.h"
#include "Save.h"

std::vector<std::string> Save::read_raw_save_file()
{
    std::string temp, path;
    path = directory_consts::SAVE_FILE_PATH;
    //path = "C:/Users/matsi/source/repos/ColourWars/save_data.txt";
    std::ifstream infile(path);
    std::vector<std::string> lines;
    while (std::getline(infile, temp))
    {
        lines.push_back(temp);
    }
    return lines;
}
bool Save::check_if_save_file_exists()
{
    std::ifstream infile(directory_consts::SAVE_FILE_PATH);
	return infile.good();
}
void Save::load_save_file()
{
    if (this->check_if_save_file_exists())
    {
		std::cout << "Save file exists.\n";
	}
    else
    {
        std::cout << "Save file does not exist.\n";
        this->save_to_file();
    }
    std::vector<std::string> save_file = read_raw_save_file();
    this->_save_data = decode_save_file(save_file);
}

save_data Save::decode_save_file(const std::vector<std::string>& save_file)
{
    // set save file to default
    save_data result = save_data();
    if (save_file.size() < 2)
    {
        return result;
    }
    try
    {
	    result.resolution = save_file[0];
        result.full_screen = save_file[1];
	}
    catch (const std::exception&)
    {
        return result;
    }
    //std::string line = save_file[0];
    //result.resolution = line;
    return result;
}
bool Save::write_save_file(const save_data& data)
{
    std::ofstream save_file(directory_consts::SAVE_FILE_PATH);
    if (save_file.is_open())
    {
        save_file << this->_save_data.resolution + "\n";
        save_file << this->_save_data.full_screen + "\n";
        //do other save items 

        save_file.close();
        return true;
    }
    return false;
}

void Save::save_to_file()
{
    auto start = std::chrono::system_clock::now();
    auto legacyStart = std::chrono::system_clock::to_time_t(start);
    char buffer[30];
    std::cout << ctime_s(buffer, sizeof(buffer), &legacyStart) << " Saving...\n";
    if (write_save_file(this->_save_data))
    {
		std::cout << "Save was successful.\n";
	}
    else
    {
		std::cout << "Save was unsuccessful.\n";
	}
}

void Save::set_resolution_and_save(const std::string& resolution)
{
    this->_save_data.resolution = resolution;
    this->save_to_file();
}

void Save::set_full_screen_and_save(bool full_screen)
{
    if (full_screen)
    {
        this->_save_data.full_screen = "true";
    }
    else
    {
		this->_save_data.full_screen = "false";
	}
	this->save_to_file();
}